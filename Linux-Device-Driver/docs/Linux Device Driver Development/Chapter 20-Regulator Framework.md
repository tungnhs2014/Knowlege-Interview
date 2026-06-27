```bash
# Chapter 20 - Regulator Framework
```
A regulator is an electronic device that supplies power to other devices. Devices powered by regulators are called consumers. They consume power provided by regulators. Most regulators can enable and disable their output and some can also control their output voltage or current. The driver should expose those capabilities to consumers by means of specific functions and data structures, which we will discuss in this chapter.
The chip that physically provides regulators is called a Power Management Integrated
Circuit (PMIC):
The Linux regulator framework has been designed to interface and control voltage and current regulators. It is divided into four separate interfaces, as follows:
A regulator drivers interface for regulator PMIC drivers. The structure of this interface can be found in include/linux/regulator/driver.h.
A consumer interface for device drivers.
A machine interface for board configuration.
A sysfs interface for user space.
In this chapter, we will cover the following topics:
Introduction to the PMIC/producer driver interface, driver methods, and data structures
A case study with the ISL6271A MIC driver, as well as a dummy regulator for testing purposes
A regulator consumer interface along with its API
Regulator (producer/consumer) binding in DT
## PMIC/producer driver interface
The producer is the device generating the regulated voltage or current. The name of such a device is PMIC and it can be used for power sequencing, battery management, DC-to-DC
conversion, or simple power switches (on/off). It regulates the output power from the input power, with the help of (and under) software control.
It deals with regulator drivers, and especially the producer PMIC side, which requires a few headers:
```c
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
```
## Driver data structures
We will start with a short walkthrough of data structures used by the regulator framework.
Only the producer interface is described in this section.
## Description structure
The kernel describes every regulator provided by a PMIC by means of a struct regulator_desc structure, which characterizes a regulator. By regulator, I mean any independent regulated output. For example, the ISL6271A from Intersil is a PMIC with three independent regulated outputs. There should then be three instances of regulator_desc in its driver. This structure, which contains the fixed properties of a regulator, looks like the following:
```c
struct regulator_desc {
```
const char *name;
const char *of_match;
```c
int id;
unsigned n_voltages;
```
const struct regulator_ops *ops;
```c
int irq;
enum regulator_type type;
struct module *owner;
unsigned int min_uV;
unsigned int uV_step;
};
```
Let's omit some fields for the sake of simplicity. The full structure definition is available in include/linux/regulator/driver.h:
name holds the name of the regulator.
of_match holds the name used to identify the regulator in DT.
id is a numerical identifier for the regulator.
owner represents the module providing the regulator. Set this field to
THIS_MODULE.
type indicates whether the regulator is a voltage regulator or a current regulator.
It can either be REGULATOR_VOLTAGE or REGULATOR_CURRENT. Any other value will result in a regulator registering failure.
n_voltages indicates the number of selectors available for this regulator. It represents the numerical value that the regulator can output. For fixed output voltage, n_voltages should be set to 1.
min_uV indicates the minimum voltage value this regulator can provide. It is the voltage given by the lowest selector.
uV_step represents the voltage increase with each selector.
ops represents the regulator operations table. It is a structure pointing to a set of operation callbacks that the regulator can support. This field will be discussed later.
irq is the interrupt number of the regulator.
## Constraints structure
When a PMIC exposes a regulator to consumers, it has to impose some nominal limits for this regulator with the help of the struct regulation_constraints structure. It is a structure that gathers security limits of the regulator and defines boundaries the consumers cannot cross. It is a kind of a contract between the regulator driver and the consumer driver:
```c
struct regulation_constraints {
```
const char *name;
/* voltage output range (inclusive) - for voltage control */
```c
int min_uV;
int max_uV;
int uV_offset;
```
/* current output range (inclusive) - for current control */
```c
int min_uA;
int max_uA;
```
/* valid regulator operating modes for this machine */
```c
unsigned int valid_modes_mask;
```
/* valid operations for regulator on this machine */
```c
unsigned int valid_ops_mask;
struct regulator_state state_disk;
struct regulator_state state_mem;
struct regulator_state state_standby;
```
suspend_state_t initial_state; /* suspend state to set at init */
/* mode to set on startup */
```c
unsigned int initial_mode;
```
/* constraint flags */
```c
unsigned always_on:1; /* regulator never off when system is on */
unsigned boot_on:1; /* bootloader/firmware enabled regulator */
unsigned apply_uV:1; /* apply uV constraint if min == max */
};
```
Let's describe each element in the structure:
min_uV, min_uA, max_uA, and max_uV are the smallest voltage/current values that the consumers may set.
uV_offset is the offset applied to voltages from the consumer to compensate for voltage drops.
valid_modes_mask and valid_ops_mask, are masks of modes and operations,
respectively which may be configured/performed by consumers.
always_on should be set if the regulator should never be disabled.
boot_on should be set if the regulator is enabled when the system is initially started. If the regulator is not enabled by the hardware or bootloader, then it will be enabled when the constraints are applied.
name is a descriptive name for the constraints used for display purposes.
apply_uV applies the voltage constraint when initializing.
input_uV represents the input voltage for this regulator when it is supplied by another regulator.
state_disk, state_mem, and state_standby define the state for the regulator when the system is suspended in disk mode, mem mode, or in standby.
initial_state indicates the suspended state is set by default.
initial_mode is the mode to set at startup.
## init data structure
There are two ways to pass regulator_init_data to a driver; this can be done by platform data in the board initialization file or by a node in the device tree using the of_get_regulator_init_data function:
```c
struct regulator_init_data {
struct regulation_constraints constraints;
```
/* optional regulator machine specific init */
```c
int (*regulator_init)(void *driver_data);
void *driver_data; /* core does not touch this */
};
```
The following are the meanings of elements in the structure:
constraints represents the regulator constraints regulator_init is an optional callback invoked at a given moment when the core registers the regulator driver_data represents the data passed to regulator_init
As you can see, the struct constraints structure is part of init data. This is explained by the fact that at the initialization of the regulator, its constraint is directly applied to it, way before any consumer can use it.
## Feeding init data into a board file
This method consists of filling an array of constraints, either from within the driver, or in the board file, and using it as part of the platform data. The following is the sample based on the device from a case study, the ISL6271A from Intersil:
```c
static struct regulator_init_data isl_init_data[] = {
[0] = {
.constraints = {
```
.name = "Core Buck",
.min_uV = 850000,
.max_uV = 1600000,
.valid_modes_mask = REGULATOR_MODE_NORMAL
| REGULATOR_MODE_STANDBY,
.valid_ops_mask = REGULATOR_CHANGE_MODE
| REGULATOR_CHANGE_STATUS,
```c
},
},
[1] = {
.constraints = {
```
.name = "LDO1",
.min_uV = 1100000,
.max_uV = 1100000,
.always_on = true,
.valid_modes_mask = REGULATOR_MODE_NORMAL
| REGULATOR_MODE_STANDBY,
.valid_ops_mask = REGULATOR_CHANGE_MODE
| REGULATOR_CHANGE_STATUS,
```c
},
},
[2] = {
.constraints = {
```
.name = "LDO2",
.min_uV = 1300000,
.max_uV = 1300000,
.always_on = true,
.valid_modes_mask = REGULATOR_MODE_NORMAL
| REGULATOR_MODE_STANDBY,
.valid_ops_mask = REGULATOR_CHANGE_MODE
| REGULATOR_CHANGE_STATUS,
```c
},
},
};
```
This method is now depreciated, though it is presented here for your information. The new and recommended approach is the DT, which is described in the next section.
## Feeding init data into the DT
In order to extract init data passed from within the DT, there is a new data type that we need to introduce, struct of_regulator_match, which looks like this:
```c
struct of_regulator_match {
```
const char *name;
```c
void *driver_data;
struct regulator_init_data *init_data;
struct device_node *of_node;
```
const struct regulator_desc *desc;
```c
};
```
Prior to making any use of this data structure, we need to figure out how to achieve the regulator binding of a DT file.
Every PMIC node in the DT should have a sub-node named regulators, in which we have to declare each of the regulators this PMIC provides as a dedicated sub-node. In other words, every regulator of a PMIC is defined as a sub-node of the regulators node, which in turn is a child of the PMIC node in the DT.
There are standardized properties you can define in a regulator node:
```dts
regulator-name: This is a string used as a descriptive name for regulator outputs regulator-min-microvolt: This is the smallest voltage that consumers may set regulator-max-microvolt: This is the largest voltage consumers may set regulator-microvolt-offset: This is the offset applied to voltages to compensate for voltage drops regulator-min-microamp: This is the smallest current consumers may set regulator-max-microamp: This is the largest current consumers may set regulator-always-on: This is a Boolean value, indicating that the regulator should never be disabled regulator-boot-on: This is a bootloader/firmware enabled regulator
<name>-supply: This is a phandle to the parent supply/regulator node regulator-ramp-delay: This is the ramp delay for the regulator (in uV/uS)
```
Those properties really look like fields in struct regulator_init_data. Back with the
ISL6271A driver, its DT entry should look like this:
```c
isl6271a@3c {
dts
compatible = "isl6271a";
reg = <0x3c>;
interrupts = <0 86 0x4>;
```
/* supposing our regulator is powered by another regulator */
```dts
in-v1-supply = <&some_reg>;
```
[...]
```c
regulators {
reg1: core_buck {
dts
regulator-name = "Core Buck";
regulator-min-microvolt = <850000>;
regulator-max-microvolt = <1600000>;
c
};
reg2: ldo1 {
dts
regulator-name = "LDO1";
regulator-min-microvolt = <1100000>;
regulator-max-microvolt = <1100000>;
regulator-always-on;
c
};
reg3: ldo2 {
dts
regulator-name = "LDO2";
regulator-min-microvolt = <1300000>;
regulator-max-microvolt = <1300000>;
regulator-always-on;
c
};
};
};
```
Using the kernel helper function, of_regulator_match(), given the regulators subnode as the parameter, the function will walk through each regulator device node and build a struct init_data structure for each of them. There is an example in the probe()
function, discussed in the Driver methods section.
## Configuration structure
Regulator devices are configured by means of the struct regulator_config structure,
which holds variable elements of the regulator description. This structure is passed to the framework when it comes to registering a regulator with the core:
```c
struct regulator_config {
struct device *dev;
```
const struct regulator_init_data *init_data;
```c
void *driver_data;
struct device_node *of_node;
};
```
The preceding code can be explained as follows:
dev represents the struct device structure the regulator belongs to.
init_data is the most important field of the structure, since it contains an element holding the regulator constraints (a machine-specific structure).
driver_data holds the regulator's private data.
of_node is for DT capable drivers. It is the node to parse for DT bindings. It is up to the developer to set this field. It may be NULL also.
## Device operation structure
The struct regulator_ops structure is a list of callbacks representing all operations a regulator can perform. These callbacks are helpers and are wrapped by generic kernel functions:
```c
struct regulator_ops {
```
/* enumerate supported voltages */
```c
int (*list_voltage) (struct regulator_dev *,
unsigned selector);
```
/* get/set regulator voltage */
```c
int (*set_voltage) (struct regulator_dev *,
int min_uV, int max_uV,
unsigned *selector);
int (*map_voltage)(struct regulator_dev *,
int min_uV, int max_uV);
int (*set_voltage_sel) (struct regulator_dev *,
unsigned selector);
int (*get_voltage) (struct regulator_dev *);
int (*get_voltage_sel) (struct regulator_dev *);
```
/* get/set regulator current */
```c
int (*set_current_limit) (struct regulator_dev *,
int min_uA, int max_uA);
int (*get_current_limit) (struct regulator_dev *);
int (*set_input_current_limit) (struct regulator_dev *,
int lim_uA);
int (*set_over_current_protection) (struct regulator_dev *);
int (*set_active_discharge) (struct regulator_dev *,
```
bool enable);
/* enable/disable regulator */
```c
int (*enable) (struct regulator_dev *);
int (*disable) (struct regulator_dev *);
int (*is_enabled) (struct regulator_dev *);
```
/* get/set regulator operating mode (defined in consumer.h) */
```c
int (*set_mode) (struct regulator_dev *, unsigned int mode);
unsigned int (*get_mode) (struct regulator_dev *);
};
```
Callback names explain quite well what they do. There are other callbacks that are not listed here, for which you must enable the appropriate mask in valid_ops_mask or valid_modes_mask of the regulator's constraints before the consumer can use them.
Available operation mask flags are defined in include/linux/regulator/machine.h.
Therefore, given a struct regulator_dev structure, you can get the ID of the corresponding regulator by calling the rdev_get_id() function:
```c
int rdev_get_id(struct regulator_dev *rdev)
```
## Driver methods
Driver methods consist of probe() and remove() functions. Please refer to the preceding data structure if this section seems unclear to you.
## The probe function
The probe function of a PMIC driver can be split into a few steps, enumerated as follows:
1. Define an array of struct regulator_desc objects for all the regulators provided by this PMIC. In this step, you should define a valid struct regulator_ops to be linked to the appropriate regulator_desc. It could be the same regulator_ops for all, assuming they all support the same operations.
2. Now, in the probe function, for each regulator, do the following:
Fetch the appropriate struct regulator_init_data either from the platform data, which must already contain a valid struct regulation_constraints, or build a struct regulation_constraints from the DT, in order to build a new struct regulator_init_data object.
Use the previous struct regulator_init_data to set up a struct regulator_config structure. If the driver supports the
DT, you can make regulator_config.of_node point to the node used to extract the regulator properties.
Call regulator_register() (or the managed version, devm_regulator_register()) to register the regulator with the core, giving the previous regulator_desc and regulator_config as parameters.
A regulator is registered with the kernel using the regulator_register() function, or devm_regulator_register(), which is the managed version:
```c
struct regulator_dev * regulator_register(const struct regulator_desc
```
*regulator_desc, const struct regulator_config *cfg)
This function returns a data type we have not discussed so far: a struct regulator_dev object, defined in include/linux/regulator/driver.h. That structure represents an instance of a regulator device from the producer side (it is different in the consumer side).
Instances of the struct regulator_dev structure should not be used directly by anything except the regulator core and notification injection (which should take the mutex and not other direct access). That being said, to keep track of the registered regulator from within the driver, you should hold references for each regulator_dev object returned by the registering function.
## The remove function
The remove() function is where every operation performed earlier during the probe function. Therefore, the essential function you should keep in mind is regulator_unregister(), when it comes to removing a regulator from the system:
```c
void regulator_unregister(struct regulator_dev *rdev)
```
This function accepts a pointer to a struct regulator_dev structure as a parameter. This is another reason a reference for each registered regulator should be kept. The following is the remove function of the ISL6271A driver:
```c
static int __devexit isl6271a_remove(struct i2c_client *i2c)
{
struct isl_pmic *pmic = i2c_get_clientdata(i2c);
int i;
for (i = 0; i < 3; i++)
regulator_unregister(pmic->rdev[i]);
kfree(pmic);
return 0;
}
```
## Case study – Intersil ISL6271A voltage regulator
This PMIC provides three regulator's devices, among which only one can have its output value changed. The two others provide fixed voltages:
```c
struct isl_pmic {
struct i2c_client *client;
struct regulator_dev *rdev[3];
struct mutex mtx;
};
```
First, we define ops callbacks, to set up a struct regulator_desc:
1. A callback to handle a get_voltage_sel operation:
```c
static int isl6271a_get_voltage_sel(struct regulator_dev *rdev)
{
struct isl_pmic *pmic = rdev_get_drvdata(dev);
int idx = rdev_get_id(rdev);
idx = i2c_smbus_read_byte(pmic->client);
if (idx < 0)
```
[...] /* handle this error */
```c
return idx;
}
```
The following is the callback to handle a set_voltage_sel operation:
```c
static int isl6271a_set_voltage_sel(
struct regulator_dev *dev, unsigned selector)
{
struct isl_pmic *pmic = rdev_get_drvdata(dev);
int err;
err = i2c_smbus_write_byte(pmic->client, selector);
if (err < 0)
```
[...] /* handle this error */
```c
return err;
}
```
2. Since we are done with the callback definition, we can build struct regulator_ops:
```c
static struct regulator_ops isl_core_ops = {
```
.get_voltage_sel = isl6271a_get_voltage_sel,
.set_voltage_sel = isl6271a_set_voltage_sel,
.list_voltage = regulator_list_voltage_linear,
.map_voltage = regulator_map_voltage_linear,
```c
};
static struct regulator_ops isl_fixed_ops = {
```
.list_voltage = regulator_list_voltage_linear,
```c
};
```
You can ask yourself where the regulator_list_voltage_linear and regulator_list_voltage_linear functions come from. As with many other regulator helper functions, they are also defined in drivers/regulator/helpers.c. The kernel provides helper functions for linear output regulators, as is the case for the ISL6271A.
It is time to build an array of struct regulator_desc for all regulators:
```c
static const struct regulator_desc isl_rd[] = {
{
```
.name = "Core Buck",
.id = 0,
.n_voltages = 16,
.ops = &isl_core_ops,
.type = REGULATOR_VOLTAGE,
.owner = THIS_MODULE,
.min_uV = ISL6271A_VOLTAGE_MIN,
.uV_step = ISL6271A_VOLTAGE_STEP,
```c
}, {
```
.name = "LDO1",
.id = 1,
.n_voltages = 1,
.ops = &isl_fixed_ops,
.type = REGULATOR_VOLTAGE,
.owner = THIS_MODULE,
.min_uV = 1100000,
```c
}, {
```
.name = "LDO2",
.id = 2,
.n_voltages = 1,
.ops = &isl_fixed_ops,
.type = REGULATOR_VOLTAGE,
.owner = THIS_MODULE,
.min_uV = 1300000,
```c
},
};
```
LDO1 and LDO2 have a fixed output voltage. It is why their n_voltages properties are set to 1, and their ops only provide regulator_list_voltage_linear mapping.
3. Now we are in the probe function, the place where we need to build our struct init_data structures. If you remember, we will use struct of_regulator_match, introduced previously. We should declare an array of that type, in which we should set the .name property of each regulator, for which we need to fetch init_data:
```c
static struct of_regulator_match isl6271a_matches[] = {
{ .name = "core_buck", },
{ .name = "ldo1", },
{ .name = "ldo2", },
};
```
Looking a bit closer, you will notice that the .name property is set with exactly the same value as the label of the regulator in the device tree. This is a rule you should care about and respect.
Now let us look at the probe function. The ISL6271A provides three regulator outputs,
which means that the regulator_register() function should be called three times:
```c
static int isl6271a_probe(struct i2c_client *i2c,
```
const struct i2c_device_id *id)
```c
{
struct regulator_config config = { };
struct regulator_init_data *init_data =
dev_get_platdata(&i2c->dev);
struct isl_pmic *pmic;
int i, ret;
struct device *dev = &i2c->dev;
struct device_node *np, *parent;
if (!i2c_check_functionality(i2c->adapter,
```
I2C_FUNC_SMBUS_BYTE_DATA))
```c
return -EIO;
pmic = devm_kzalloc(&i2c->dev,
```
sizeof(struct isl_pmic), GFP_KERNEL);
```c
if (!pmic)
return -ENOMEM;
```
/* Get the device (PMIC) node */
```c
np = of_node_get(dev->of_node);
if (!np)
return -EINVAL;
```
/* Get 'regulators' subnode */
parent = of_get_child_by_name(np, "regulators");
```c
if (!parent) {
dev_err(dev, "regulators node not found\n");
return -EINVAL;
}
```
/* fill isl6271a_matches array */
ret = of_regulator_match(dev, parent, isl6271a_matches,
```c
ARRAY_SIZE(isl6271a_matches));
of_node_put(parent);
if (ret < 0) {
```
dev_err(dev, "Error parsing regulator init data: %d\n",
ret);
```c
return ret;
}
pmic->client = i2c;
mutex_init(&pmic->mtx);
for (i = 0; i < 3; i++) {
struct regulator_init_data *init_data;
struct regulator_desc *desc;
int val;
if (pdata)
```
/* Given as platform data */
```c
config.init_data = pdata->init_data[i];
```
else
/* Fetched from device tree */
config.init_data = isl6271a_matches[i].init_data;
```c
config.dev = &i2c->dev;
```
config.of_node = isl6271a_matches[i].of_node;
config.ena_gpio = -EINVAL;
/*
* config is passed by reference because the kernel
* internally duplicate it to create its own copy
* so that it can override some fields
*/
```c
pmic->rdev[i] = devm_regulator_register(&i2c->dev,
```
&isl_rd[i], &config);
```c
if (IS_ERR(pmic->rdev[i])) {
dev_err(&i2c->dev, "failed to register %s\n",
id->name);
return PTR_ERR(pmic->rdev[i]);
}
}
i2c_set_clientdata(i2c, pmic);
return 0;
}
```
init_data can be NULL for a fixed regulator. It means that for the
ISL6271A, only the regulator whose voltage output may change may be assigned an init_data.
/* Only the first regulator actually need it */
```c
if (i == 0)
if(pdata)
```
config.init_data = init_data; /* pdata */
else isl6271a_matches[i].init_data; /* DT */
else config.init_data = NULL;
The preceding driver does not fill every field of struct regulator_desc. It greatly depends on the type of device for which we write a driver. Some drivers leave the whole job to the regulator core, and only provide the chip's register address, which the regulator core needs to work with. Such drivers use the regmap API, which is a generic I2C and SPI
register map library. For instance drivers/regulator/max8649.c is an example.
## Driver example
Let's summarize the things discussed previously in a real driver, for a dummy PMIC with two regulators, where the first one has a voltage range of 850000 µV to 1600000 µV with a step of 50000 µV, and the second regulator has a fixed voltage of 1300000 µV:
```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h> /* For platform devices */
#include <linux/interrupt.h> /* For IRQ */
#include <linux/of.h> /* For DT*/
#include <linux/err.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#define DUMMY_VOLTAGE_MIN 850000
#define DUMMY_VOLTAGE_MAX 1600000
#define DUMMY_VOLTAGE_STEP 50000
struct my_private_data {
int foo;
int bar;
struct mutex lock;
};
static const struct of_device_id regulator_dummy_ids[] = {
dts
{ .compatible = "packt,regulator-dummy", },
c
{ /* sentinel */ }
};
static struct regulator_init_data dummy_initdata[] = {
[0] = {
.constraints = {
```
.always_on = 0,
.min_uV = DUMMY_VOLTAGE_MIN,
.max_uV = DUMMY_VOLTAGE_MAX,
```c
},
},
[1] = {
.constraints = {
```
.always_on = 1,
```c
},
},
};
static int isl6271a_get_voltage_sel(struct regulator_dev *dev)
{
return 0;
}
static int isl6271a_set_voltage_sel(struct regulator_dev *dev,
unsigned selector)
{
return 0;
}
static struct regulator_ops dummy_fixed_ops = {
```
.list_voltage = regulator_list_voltage_linear,
```c
};
static struct regulator_ops dummy_core_ops = {
```
.get_voltage_sel = isl6271a_get_voltage_sel,
.set_voltage_sel = isl6271a_set_voltage_sel,
.list_voltage = regulator_list_voltage_linear,
.map_voltage = regulator_map_voltage_linear,
```c
};
static const struct regulator_desc dummy_desc[] = {
{
```
.name = "Dummy Core",
.id = 0,
.n_voltages = 16,
.ops = &dummy_core_ops,
.type = REGULATOR_VOLTAGE,
.owner = THIS_MODULE,
.min_uV = DUMMY_VOLTAGE_MIN,
.uV_step = DUMMY_VOLTAGE_STEP,
```c
}, {
```
.name = "Dummy Fixed",
.id = 1,
.n_voltages = 1,
.ops = &dummy_fixed_ops,
.type = REGULATOR_VOLTAGE,
.owner = THIS_MODULE,
.min_uV = 1300000,
```c
},
};
static int my_pdrv_probe (struct platform_device *pdev)
{
struct regulator_config config = { };
config.dev = &pdev->dev;
struct regulator_dev *dummy_regulator_rdev[2];
int ret, i;
for (i = 0; i < 2; i++){
```
config.init_data = &dummy_initdata[i];
dummy_regulator_rdev[i] = \
```c
regulator_register(&dummy_desc[i], &config);
if (IS_ERR(dummy_regulator_rdev)) {
```
ret = PTR_ERR(dummy_regulator_rdev);
```c
pr_err("Failed to register regulator: %d\n", ret);
return ret;
}
}
platform_set_drvdata(pdev, dummy_regulator_rdev);
return 0;
}
static void my_pdrv_remove(struct platform_device *pdev)
{
int i;
struct regulator_dev *dummy_regulator_rdev = \
platform_get_drvdata(pdev);
for (i = 0; i < 2; i++)
regulator_unregister(&dummy_regulator_rdev[i]);
}
static struct platform_driver mypdrv = {
```
.probe = my_pdrv_probe,
.remove = my_pdrv_remove,
```c
.driver = {
dts
.name = "regulator-dummy",
```
.of_match_table = of_match_ptr(regulator_dummy_ids),
.owner = THIS_MODULE,
```c
},
};
module_platform_driver(mypdrv);
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_LICENSE("GPL");
```
Once the module is loaded and the device matched, the kernel will print something like this:
Dummy Core: at 850 mV
Dummy Fixed: 1300 mV
You can then check what happened under the hood:
```bash
# ls /sys/class/regulator/
```
regulator.0 regulator.11 regulator.14 regulator.4 regulator.7
regulator.1 regulator.12 regulator.2 regulator.5 regulator.8
regulator.10 regulator.13 regulator.3 regulator.6 regulator.9
regulator.13 and regulator.14 have been added by our driver. Let us now check their properties:
```bash
# cd /sys/class/regulator
# cat regulator.13/name
```
Dummy Core
```bash
# cat regulator.14/name
```
Dummy Fixed
```bash
# cat regulator.14/type voltage
# cat regulator.14/microvolts
```
1300000
```bash
# cat regulator.13/microvolts
```
850000
## Regulators consumer interface
The consumer interface only requires the driver to include one header:
```c
#include <linux/regulator/consumer.h>
```
A consumer can be static or dynamic. A static one requires only a fixed supply, whereas a dynamic one requires active management of the regulator at runtime. From the consumer point side, a regulator device is represented in the kernel as an instance of a struct regulator structure, defined in drivers/regulator/internal.h and shown as follows:
/*
* struct regulator
*
* One for each consumer device.
*/
```c
struct regulator {
struct device *dev;
struct list_head list;
unsigned int always_on:1;
unsigned int bypass:1;
int uA_load;
int min_uV;
int max_uV;
```
char *supply_name;
```c
struct device_attribute dev_attr;
struct regulator_dev *rdev;
struct dentry *debugfs;
};
```
This structure is meaningful enough and does not need us to add any comments. To see how easy it is to consume a regulator, here is a little example of how a consumer acquires a regulator:
[...]
```c
int ret;
struct regulator *reg;
dts
const char *supply = "vdd1";
c
int min_uV, max_uV;
dts
reg = regulator_get(dev, supply);
```
[...]
## Regulator device requesting
Prior to gaining access to a regulator, the consumer has to request the kernel by means of the regulator_get() function. It is also possible to use the managed version, the devm_regulator_get() function:
```c
struct regulator *regulator_get(struct device *dev,
```
const char *id)
An example of using this function is as follows:
```dts
reg = regulator_get(dev, "Vcc");
```
The consumer passes in its struct device pointer and power supply ID. The core will try to find the correct regulator by consulting the DT or a machine-specific lookup table. If we focus only on the device tree, *id should match the <name> pattern of the regulator supply in the device tree. If the lookup is successful then this call will return a pointer to the struct regulator that supplies this consumer.
To release the regulator, the consumer driver should call:
```c
void regulator_put(struct regulator *regulator)
```
Prior to calling this function, the driver should ensure that all regulator_enable() calls made on this regulator source are balanced by regulator_disable() calls.
More than one regulator can supply a consumer; for example, codec consumers with analog and digital supplies:
digital = regulator_get(dev, "Vcc"); /* digital core */
analog = regulator_get(dev, "Avdd"); /* analog */
Consumer probe() and remove() functions are an appropriate place to grab and release regulators.
## Controlling the regulator device
Regulator control consists of enabling, disabling, and setting output values for a regulator.
## Enabling and disabling regulator output
A consumer can enable its power supply by calling the following:
```c
int regulator_enable(regulator);
```
This function returns 0 on success. The reverse operation consists of disabling the power supply, by calling this:
```c
int regulator_disable(regulator);
```
To check whether a regulator is already enabled or not, the consumer should call this:
```c
int regulator_is_enabled(regulator);
```
This function returns a value greater than 0 if the regulator is enabled. Since the regulator may be enabled early by the bootloader or shared with another consumer, you can use the regulator_is_enabled() function to check the regulator state.
Here is an example:
```c
printk (KERN_INFO "Regulator Enabled = %d\n",
regulator_is_enabled(reg));
```
For a shared regulator, regulator_disable() will actually disable the regulator only when the enabled reference count is zero. That said, you can force disabling in case of an emergency; for example, by calling regulator_force_disable():
```c
int regulator_force_disable(regulator);
```
Each of the functions that we will discuss in the sections that follow are actually wrappers around a regulator_ops operation. For example, regulator_set_voltage() internally calls regulator_ops.set_voltage after checking the corresponding mask allowing this operation to be set.
## Voltage control and status
For consumers that need to adapt their power supplies according to their operating modes,
the kernel provides this:
```c
int regulator_set_voltage(regulator, min_uV, max_uV);
```
min_uV and max_uV are the minimum and maximum acceptable voltages in microvolts.
If called when the regulator is disabled, this function will change the voltage configuration so that the voltage is physically set when the regulator is next enabled. That said,
consumers can get the regulator configured voltage output by calling regulator_get_voltage(), which will return the configured output voltage whether the regulator is enabled or not:
```c
int regulator_get_voltage(regulator);
```
Here is an example:
```c
printk (KERN_INFO "Regulator Voltage = %d\n",
regulator_get_voltage(reg));
```
## Current limit control and status
What we have discussed in the voltage section also applies here. For example, USB drivers may want to set the limit to 500 mA when supplying power.
Consumers can control their supply current limit by calling the following:
```c
int regulator_set_current_limit(regulator, min_uA, max_uA);
```
min_uA and max_uA are the minimum and maximum acceptable current limits in microamps.
In the same way, consumers can get the regulator configured to the current limit by calling regulator_get_current_limit(), which will return the current limit whether the regulator is enabled or not:
```c
int regulator_get_current_limit(regulator);
```
## Operating mode control and status
For efficient power management, some consumers may change the operating mode of their supply when their (consumers) operating state changes. Consumer drivers can request a change in their supply regulator operating mode by calling the following:
```c
int regulator_set_optimum_mode(struct regulator *regulator,
int load_uA);
int regulator_set_mode(struct regulator *regulator,
unsigned int mode);
unsigned int regulator_get_mode(struct regulator *regulator);
```
A consumer should use regulator_set_mode() on a regulator only when it knows about the regulator and does not share the regulator with other consumers. This is known as direct mode. Now, regulator_set_uptimum_mode() causes the core to undertake some background work in order to determine what operating mode is best for the requested current. This is called indirect mode.
## Regulator binding
This section only deals with consumer interface binding. Because PMIC binding consists of providing init data for regulators that this PMIC provides, you should refer to the Feeding init data into the DT section to understand producer binding.
A consumer node can reference one or more of its supplies/regulators using the following bindings:
<name>-supply: phandle to the regulator node
It is the same principle as PWM consumer binding. Now, <name> should be meaningful enough, so that the driver can easily refer to it when requesting the regulator. That said,
<name> must match the *id parameter of the regulator_get() function:
```c
twl_reg1: regulator@0 {
```
[...]
```c
};
twl_reg2: regulator@1 {
```
[...]
```c
};
mmc: mmc@0x0 {
```
[...]
```dts
vmmc-supply = <&twl_reg1>;
vmmcaux-supply = <&twl_reg2>;
c
};
```
The consumer code (which is the MMC driver) that actually requests its supplies could look like this:
```c
struct regulator *main_regulator;
struct regulator *aux_regulator;
int ret;
```
main_regulator = devm_regulator_get(dev, "vmmc");
/*
* It is a good practice to apply the config before
* enabling the regulator
*/
```c
if (!IS_ERR(io_regulator)) {
regulator_set_voltage(main_regulator,
```
MMC_VOLTAGE_DIGITAL,
MMC_VOLTAGE_DIGITAL);
ret = regulator_enable(io_regulator);
```c
}
```
[...]
aux_regulator = devm_regulator_get(dev, "vmmcaux");
[...]
## Summary
With the wide range of devices that need to be smartly and smoothly supplied, this chapter can be relied on to take care of their power supply management. PMIC devices usually sit on SPI or I2C buses. Having already dealt with these buses in previous chapters, you should be able to write any PMIC driver. Let's now jump to the next chapter, which deals with framebuffer drivers, which is a completely different and no less interesting topic