```bash
# Chapter 15 - GPIO Controller Drivers - gpio_chip
```
In the previous chapter, we dealt with GPIO lines. Those lines are exposed to the system by means of a special device called the GPIO controller. This chapter will explain step by step how to write drivers for such devices, thus covering the following topics:
GPIO controller driver architecture and data structures
Sysfs interface for GPIO controllers
GPIO controller representation in DT
## Driver architecture and data structures
Drivers for such devices should provide the following:
Methods to establish GPIO direction (input and output).
Methods used to access GPIO values (get and set).
Methods to map a given GPIO to IRQ and return the associated number.
A flag saying whether calls to its methods may sleep. This is very important.
An optional debugfs dump method (showing extra state such as pullup config).
An optional number called a base number, from which GPIO numbering should start. It will be automatically assigned if omitted.
In the kernel, a GPIO controller is represented as an instance of struct gpio_chip,
defined in linux/gpio/driver.h:
```c
struct gpio_chip {
```
const char *label;
```c
struct device *dev;
struct module *owner;
int (*request)(struct gpio_chip *chip, unsigned offset);
void (*free)(struct gpio_chip *chip, unsigned offset);
int (*get_direction)(struct gpio_chip *chip, unsigned offset);
int (*direction_input)(struct gpio_chip *chip, unsigned offset);
int (*direction_output)(struct gpio_chip *chip, unsigned offset,
int value);
int (*get)(struct gpio_chip *chip,unsigned offset);
void (*set)(struct gpio_chip *chip, unsigned offset, int value);
void (*set_multiple)(struct gpio_chip *chip, unsigned long *mask,
unsigned long *bits);
int (*set_debounce)(struct gpio_chip *chip, unsigned offset,
unsigned debounce);
int (*to_irq)(struct gpio_chip *chip, unsigned offset);
int base;
```
u16 ngpio;
const char *const *names;
bool can_sleep;
bool irq_not_threaded;
bool exported;
#ifdef CONFIG_GPIOLIB_IRQCHIP
/*
* With CONFIG_GPIOLIB_IRQCHIP we get an irqchip
* inside the gpiolib to handle IRQs for most practical cases.
*/
```c
struct irq_chip *irqchip;
struct irq_domain *irqdomain;
unsigned int irq_base;
irq_flow_handler_t irq_handler;
unsigned int irq_default_type;
```
#endif
#if defined(CONFIG_OF_GPIO)
/*
* If CONFIG_OF is enabled, then all GPIO controllers described in the
* device tree automatically may have an OF translation
*/
```c
struct device_node *of_node;
int of_gpio_n_cells;
int (*of_xlate)(struct gpio_chip *gc,
```
const struct of_phandle_args *gpiospec, u32 *flags);
```c
}
```
The following is the meaning of each element in the structure:
request is an optional hook for chip-specific activation. If provided, it is executed prior to allocating GPIO whenever you call gpio_request() or gpiod_get().
free is an optional hook for chip-specific deactivation. If provided, it is executed before the GPIO is deallocated whenever you call gpiod_put() or gpio_free().
get_direction is executed whenever you need to know the direction of the
GPIO offset. Return value should be 0 to mean out, and 1 to mean in(the same as GPIOF_DIR_XXX), or negative error.
direction_input configures the signal offset as input, or returns an error.
get returns the value of GPIO offset; for output signals, this returns either the value actually sensed or zero.
set assigns an output value to the GPIO offset.
```c
set_multiple is called when you need to assign output values for multiple signals defined by mask. If not provided, the kernel will install a generic hook that will walk through mask bits and execute chip->set(i) on each bit set.
```
See the following code, which shows how you can implement this function:
```c
static void gpio_chip_set_multiple(struct gpio_chip *chip,
unsigned long *mask, unsigned long *bits)
{
if (chip->set_multiple) {
chip->set_multiple(chip, mask, bits);
} else {
unsigned int i;
```
/* set outputs if the corresponding mask bit is set */
```c
for_each_set_bit(i, mask, chip->ngpio)
chip->set(chip, i, test_bit(i, bits));
}
}
```
set_debounce if supported by the controller, this hook is an optional callback provided to set the debounce time for the specified GPIO.
to_irq is an optional hook to provide GPIO to IRQ mapping. This is called whenever you want to execute the gpio_to_irq() or gpiod_to_irq()
function. This implementation may not sleep.
base identifies the first GPIO number handled by this chip; or, if negative during registration, the kernel will automatically (dynamically) assign one.
ngpio is the number of GPIOs this controller provides; it starts from base to
(base + ngpio - 1).
names, if set, must be an array of strings to use as alternative names for the
GPIOs in this chip. The array must be ngpio sized, and any GPIO that does not need an alias may have its entry set to NULL in the array.
can_sleep is a Boolean flag to be set if the get()/set() method may sleep. It is the case for the GPIO controller (also known as an expander) sitting on a bus,
such as I2C or SPI, whose access may lead to sleep. This implies that, if the chip supports IRQs, these IRQs need to be threaded as the chip access may sleep when, for example, reading out the IRQ status registers. For a GPIO controller mapped to memory (part of SoC), this can be set to false.
```c
irq_not_threaded is a Boolean flag and must be set if can_sleep is set, but the IRQs don't need to be threaded.
```
Each chip exposes a number of signals, identified in method calls by offset values in the range 0 (ngpio - 1). When those signals are referenced through calls such as gpio_get_value(gpio), the offset is calculated by subtracting the base from the GPIO number.
After every callback has been defined and other fields set, you should call gpiochip_add() on the configured struct gpio_chip structure in order to register the controller with the kernel. When it comes to unregistering, use gpiochip_remove(). That is all. You can see how easy it is to write your own GPIO controller driver. In the book sources repository, you will find a working GPIO controller driver for the MCP23016 I2C
I/O expander from a microchip, whose data sheet is available at http://ww1.microchip.com/downloads/en/DeviceDoc/20090C.pdf.
To write such drivers, you should include:
```c
#include <linux/gpio.h>
```
The following is an excerpt from the driver we have written for our controller, just to show you how easy the task of writing a GPIO controller driver is:
```c
#define GPIO_NUM 16
struct mcp23016 {
struct i2c_client *client;
struct gpio_chip chip;
};
static int mcp23016_probe(struct i2c_client *client,
```
const struct i2c_device_id *id)
```c
{
struct mcp23016 *mcp;
if (!i2c_check_functionality(client->adapter,
```
I2C_FUNC_SMBUS_BYTE_DATA))
```c
return -EIO;
mcp = devm_kzalloc(&client->dev, sizeof(*mcp), GFP_KERNEL);
if (!mcp)
return -ENOMEM;
mcp->chip.label = client->name;
mcp->chip.base = -1;
mcp->chip.dev = &client->dev;
mcp->chip.owner = THIS_MODULE;
mcp->chip.ngpio = GPIO_NUM; /* 16 */
mcp->chip.can_sleep = 1; /* may not be accessed from atomic context */
mcp->chip.get = mcp23016_get_value;
mcp->chip.set = mcp23016_set_value;
mcp->chip.direction_output = mcp23016_direction_output;
mcp->chip.direction_input = mcp23016_direction_input;
mcp->client = client;
i2c_set_clientdata(client, mcp);
return gpiochip_add(&mcp->chip);
}
```
To request a self-owned GPIO from within the controller driver, you should not use gpio_request(). A GPIO driver can use the following functions instead to request and free descriptors without being pinned to the kernel forever:
```c
struct gpio_desc *gpiochip_request_own_desc(struct gpio_desc *desc, const char *label)
void gpiochip_free_own_desc(struct gpio_desc *desc)
```
Descriptors requested with gpiochip_request_own_desc() must be released with gpiochip_free_own_desc().
## Pin controller guidelines
Depending on the controller you write the driver for, you may need to implement a pin control operation to handle pin multiplexing, configuration, and so on:
For a pin controller that can only do simple GPIO, a simple struct gpio_chip will be sufficient to handle it. There is no need to set up a struct pinctrl_desc structure; just write the GPIO controller driver as it.
If the controller can generate interrupts on top of the GPIO functionality, a struct irq_chip must be set up and registered to the IRQ subsystem.
For a controller that has pin multiplexing, advanced pin driver strength, and complex biasing, you should set up the following three interfaces :
```c
struct gpio_chip, discussed earlier in this chapter struct irq_chip, discussed in the next chapter (Chapter 16,
```
Advanced IRQ Management)
```c
struct pinctrl_desc, not discussed in the book, but well explained in the kernel documentation in
```
Documentation/pinctrl.txt
## Sysfs interface for GPIO controller
On successful gpiochip_add(), a directory entry with a path such as /sys/class/gpio/gpiochipX/ will be created, where X is the GPIO controller base
(the controller providing GPIOs starting at #X), with the following attributes:
base, whose value is the same as X, and which corresponds to gpio_chip.base
(if assigned statically) and is the first GPIO managed by this chip.
label, which is provided for diagnostics (not always unique).
ngpio, which tells how many GPIOs this controller provides (N to N + ngpio
- 1). This is the same as defined in gpio_chip.ngpios.
All of the preceding attributes are read-only.
## GPIO controllers and the DT
Every GPIO controller declared in the DT must have the Boolean property gpiocontroller set. Some controllers provide IRQs mapped to the GPIO. In that case, the interrupt-cells property should be set too; usually you use 2, but it depends on the need. The first cell is the pin number, and the second represents the interrupt flag.
gpio-cells should be set to identify how many cells are used to describe a GPIO specifier.
You usually use <2>, the first cell to identify the GPIO number, and the second for flags.
Actually, most non-memory mapped GPIO controllers do not use flags:
```c
expander_1: mcp23016@27 {
dts
compatible = "microchip,mcp23016";
```
interrupt-controller;
```dts
gpio-controller;
#gpio-cells = <2>;
interrupt-parent = <&gpio6>;
interrupts = <31 IRQ_TYPE_LEVEL_LOW>;
reg = <0x27>;
#interrupt-cells=<2>;
c
};
```
The preceding sample is the node of our GPIO-controller device, and the complete device driver is provided with the sources book.
## Summary
This chapter was much more than a basis for writing the driver for any GPIO controller that you may encounter; it explains the main structure of such devices. The next chapter deals with advanced IRQ management, in which we will see how to manage an interrupt controller and thus add such functionality in the driver of the MCP23016 expander from a microchip