```bash
# Chapter 14 - Pin Control and GPIO Subsystem
```
Most embedded Linux driver and kernel engineers write using GPIOs or play with pin multiplexing. By pins, I mean outgoing line of component. SoC does multiplex pins,
meaning that a pin may have several functions; for example, MX6QDL_PAD_SD3_DAT1 in arch/arm/boot/dts/imx6dl-pinfunc.h can be either an SD3 data line 1, UART1's cts/rts, Flexcan2's Rx, or a normal GPIO.
The mechanism by which you choose the mode a pin should work in is called pin muxing.
The system responsible for this is called the pin controller. In the second part of the chapter,
we will discuss General Purpose Input Output (GPIO), which is a special function (mode)
in which a pin can operate.
In this chapter, we will:
Walk through the pin control subsystem and see how you can declare their nodes in the DT
Explore both legacy integer-based GPIO interfaces and the new descriptor-based interface API
Deal with GPIO mapped to IRQ
Handle sysfs interfaces dedicated to GPIOs
## The pin control subsystem
The pin control (pinctrl) subsystem allows managing pin muxing. In the DT, devices that need pins to be multiplexed in a certain way must declare the pin control configuration they need.
The pinctrl subsystem provides:
Pin multiplexing, which allows for reusing the same pin for different purposes,
such as one pin being a UART TX pin, GPIO line, or HSI data line. Multiplexing can affect groups of pins or individual pins.
Pin configuration, applying electronic properties of pins such as pull-up, pulldown, driver strength, debounce period, and so on.
The purpose of this book is limited to using functions exported by the pin controller driver,
and does not cover how to write a pin controller driver.
## Pinctrl and the device tree
The pinctrl subsystem is nothing but a way to gather pins (not only GPIO), and pass them to the driver. The pin controller driver is responsible for parsing pin descriptions in the DT
and applying their configuration in the chip. The driver usually needs a set of two nested nodes to describe a group of pins configurations. The first node describes the function of the group (what purpose the group will be used for), the second holds the pin's configuration.
How pin groups are assigned in the DT heavily depends on the platform, and thus the pin controller driver. Every pin control state is given an integer ID starting at 0 and contiguous.
You can use a name property, which will be mapped on top of IDs, so that the same name always points to the same ID.
Each client device's own binding determines the set of states that must be defined in its DT
node, and whether to define the set of state IDs that must be provided or whether to define the set of state names that must be provided. In any case, a pin configuration node can be assigned to a device by means of two properties:
```dts
pinctrl-<ID>: This allows us to give the list of pinctrl configurations needed for a certain state of the device. It is a list of phandles, each of which points to a pin configuration node. These referenced pin configuration nodes must be child nodes of the pin controller that they configure. Multiple entries may exist in this list so that multiple pin controllers may be configured, or so that a state may be built from multiple nodes for a single pin controller, each contributing part of the overall configuration.
pinctrl-name: This allows for giving a name to each state in a list. List entry 0
defines the name for integer state ID 0, list entry 1 for state ID 1, and so on. The state ID 0 is commonly given the name default. A list of standardized states can be found in include/linux/pinctrl/pinctrl-state.h.
```
The following is an excerpt of DT, showing some device nodes along with their pin control nodes:
usdhc@0219c000 { /* uSDHC4 */
non-removable;
vmmc-supply = <&reg_3p3v>;
```dts
status = "okay";
pinctrl-names = "default";
pinctrl-0 = <&pinctrl_usdhc4_1>;
c
};
gpio-keys {
dts
compatible = "gpio-keys";
pinctrl-names = "default";
pinctrl-0 = <&pinctrl_io_foo &pinctrl_io_bar>;
c
};
iomuxc@020e0000 {
dts
compatible = "fsl,imx6q-iomuxc";
reg = <0x020e0000 0x4000>;
```
/* shared pinctrl settings */
usdhc4 { /* first node describing the function */
```c
pinctrl_usdhc4_1: usdhc4grp-1 { /* second node */
```
fsl,pins = <
MX6QDL_PAD_SD4_CMD__SD4_CMD 0x17059
MX6QDL_PAD_SD4_CLK__SD4_CLK 0x10059
MX6QDL_PAD_SD4_DAT0__SD4_DATA0 0x17059
MX6QDL_PAD_SD4_DAT1__SD4_DATA1 0x17059
MX6QDL_PAD_SD4_DAT2__SD4_DATA2 0x17059
MX6QDL_PAD_SD4_DAT3__SD4_DATA3 0x17059
MX6QDL_PAD_SD4_DAT4__SD4_DATA4 0x17059
MX6QDL_PAD_SD4_DAT5__SD4_DATA5 0x17059
MX6QDL_PAD_SD4_DAT6__SD4_DATA6 0x17059
MX6QDL_PAD_SD4_DAT7__SD4_DATA7 0x17059
>;
```c
};
};
```
[...]
```c
uart3 {
pinctrl_uart3_1: uart3grp-1 {
```
fsl,pins = <
MX6QDL_PAD_EIM_D24__UART3_TX_DATA 0x1b0b1
MX6QDL_PAD_EIM_D25__UART3_RX_DATA 0x1b0b1
>;
```c
};
};
```
// GPIOs (Inputs)
```c
gpios {
pinctrl_io_foo: pinctrl_io_foo {
```
fsl,pins = <
MX6QDL_PAD_DISP0_DAT15__GPIO5_IO09 0x1f059
MX6QDL_PAD_DISP0_DAT13__GPIO5_IO07 0x1f059
>;
```c
};
pinctrl_io_bar: pinctrl_io_bar {
```
fsl,pins = <
MX6QDL_PAD_DISP0_DAT11__GPIO5_IO05 0x1f059
MX6QDL_PAD_DISP0_DAT9__GPIO4_IO30 0x1f059
MX6QDL_PAD_DISP0_DAT7__GPIO4_IO28 0x1f059
MX6QDL_PAD_DISP0_DAT5__GPIO4_IO26 0x1f059
>;
```c
};
};
};
```
In the preceding example, a pin configuration is given in the form <PIN_FUNCTION>
<PIN_SETTING>, for example:
MX6QDL_PAD_DISP0_DAT15__GPIO5_IO09 0x80000000
MX6QDL_PAD_DISP0_DAT15__GPIO5_IO09 represents the pin function, which is GPIO in this case, and 0x80000000 represents the pin settings.
For this line:
MX6QDL_PAD_EIM_D25__UART3_RX_DATA 0x1b0b1
MX6QDL_PAD_EIM_D25__UART3_RX_DATA represents the pin function, which is the RX line of UART3, and 0x1b0b1 represents the settings.
The pin function is a macro whose value is meaningful for the pin controller driver only.
These are generally defined in header files located in arch/<arch>/boot/dts/. If you use a UDOO quad, for example, which has an i.MX6 quad core (ARM), the pin function header will be arch/arm/boot/dts/imx6q-pinfunc.h. The following is the macro corresponding to the fifth line of the GPIO5 controller:
```c
#define MX6QDL_PAD_DISP0_DAT11__GPIO5_IO05 0x19c 0x4b0 0x000 0x5 0x0
```
<PIN_SETTING> can be used to set up things such as pull-ups, pull-downs, keepers, drive strength, and so on. How it should be specified depends on the pin controller binding, and the meaning of its value depends on the SoC data sheet, generally in the IOMUX section.
On i.MX6 IOMUXC, fewer than 17 bits are exclusively used for this purpose.
These preceding nodes are called from the corresponding driver-specific node. Moreover,
these pins are configured during the corresponding driver initialization. Prior to selecting a pin group state, you must get the pin control first using the pinctrl_get() function, call pinctrl_lookup_state() in order to check whether the requested state exists or not, and finally pinctrl_select_state() to apply the state.
The following sample shows how to get a pincontrol and apply its default configuration:
```c
struct pinctrl *p;
struct pinctrl_state *s;
int ret;
```
p = pinctrl_get(dev);
```c
if (IS_ERR(p))
return p;
```
s = pinctrl_lookup_state(p, name);
```c
if (IS_ERR(s)) {
devm_pinctrl_put(p);
return ERR_PTR(PTR_ERR(s));
}
```
ret = pinctrl_select_state(p, s);
```c
if (ret < 0) {
devm_pinctrl_put(p);
return ERR_PTR(ret);
}
```
You usually perform such steps during driver initialization. A suitable place for this code could be within the probe() function.
```c
pinctrl_select_state() internally calls pinmux_enable_setting(),
```
which in turn calls the pin_request() on each pin in the pin control node.
A pin control can be released with the pinctrl_put() function. You can use the resourcemanaged version of the API. That said, you can use pinctrl_get_select(), given the name of the state to select, in order to configure pinmux. The function is defined in include/linux/pinctrl/consumer.h as follows:
```c
static struct pinctrl *pinctrl_get_select(struct device *dev,
```
const char *name)
```dts
Here, *name is the state name as written in the pinctrl-name property. If the name of the state is default, you can just call the pinctr_get_select_default() function, which is a wrapper around pinctl_get_select():
c
static struct pinctrl * pinctrl_get_select_default(
struct device *dev)
{
return pinctrl_get_select(dev, PINCTRL_STATE_DEFAULT);
}
```
Let's see a real example in a board-specific DTS file (am335x-evm.dts):
```c
dcan1: d_can@481d0000 {
dts
status = "okay";
pinctrl-names = "default";
pinctrl-0 = <&d_can1_pins>;
c
};
```
And the following is an example in the corresponding driver:
```c
pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
if (IS_ERR(pinctrl))
dev_warn(&pdev->dev,"pins are not configured from the driver\n");
```
The pin control core will automatically claim the default pinctrl state for us when the device is probed. If you define an init state, the pinctrl core will automatically set pinctrl to this state before the probe() function,
and then switch to the default state after probe() (unless the driver has explicitly changed states already).
## The GPIO subsystem
From a hardware point of view, a GPIO is a functionality, a mode in which a pin can operate. From a software point of view, a GPIO is nothing but a digital line, which can operate as an input or output, and can have only two values: (1 for high or 0 for low).
Kernel GPIO subsystems provide every function you can imagine to set up and handle
GPIO lines from within your driver:
Prior to using a GPIO from within the driver, you should claim it to the kernel.
This is a way to take the ownership of the GPIO, preventing other drivers from accessing the same GPIO. After taking the ownership of the GPIO, you can:
Set the direction.
Toggle its output state (driving line high or low) if used as output.
Set the debounce-interval and read the state, if used as input. For
GPIO lines mapped to IRQ, you can define at what edge/level the interrupt should be triggered, and register a handler that will be run whenever the interrupt occurs.
There are actually two different ways to deal with GPIO in the kernel, as follows:
The legacy and depreciated integer-based interface, where GPIOs are represented by integers
The new and recommended descriptor-based interface, where a GPIO is represented and described by an opaque structure with a dedicated API
## The integer-based GPIO interface – legacy
The integer-based interface is the most well-known. The GPIO is identified by an integer,
which is used for every operation that needs to be performed on the GPIO. The following is the header that contains legacy GPIO access functions:
```c
#include <linux/gpio.h>
```
There are well known functions to handle GPIO in the kernel.
## Claiming and configuring the GPIO
You can allocate and take ownership of a GPIO using the gpio_request() function:
```c
static int gpio_request(unsigned gpio, const char *label)
```
gpio represents the GPIO number we are interested in, and label is the label used by the kernel for the GPIO in sysfs, as we can see in /sys/kernel/debug/gpio. You have to check the value returned, where 0 mean success and a negative error code is produced on an error. Once you've done this with the GPIO, it should be set free with the gpio_free()
function:
```c
void gpio_free(unsigned int gpio)
```
If in doubt, you can use the gpio_is_valid() function to check whether this GPIO
number is valid on the system prior to allocating it:
```c
static bool gpio_is_valid(int number)
```
Once we own the GPIO, we can change its direction, depending on the need, and whether it should be an input or output, using the gpio_direction_input() or gpio_direction_output() functions:
```c
static int gpio_direction_input(unsigned gpio)
static int gpio_direction_output(unsigned gpio, int value)
```
gpio is the GPIO number we need to set the direction. There is a second parameter when it comes to configuring the GPIO as output, value, which is the state the GPIO should be in once the output direction is effective. Here again, the return value is zero or a negative error number. These functions are internally mapped on top of lower-level callback functions exposed by the driver of the GPIO controller that provides the GPIO we use. In the next Chapter 15, GPIO Controller Drivers – gpio_chip, dealing with GPIO controller drivers, we will see that a GPIO controller, through its struct gpio_chip structure, must expose a generic set of callback functions to use its GPIOs.
Some GPIO controllers offer the possibility to change the GPIO debounce-interval (this is only useful when the GPIO line is configured as input). This feature is platform-dependent.
You can use int gpio_set_debounce() to achieve that:
```c
static int gpio_set_debounce(unsigned gpio, unsigned debounce)
```
Here, debounce is the debounce time in ms.
All the preceding functions should be called in a context that may sleep. It is a good practice to claim and configure GPIOs from within the driver's probe function.
## Accessing the GPIO – getting/setting the value
You should pay attention when accessing GPIO. In an atomic context, especially in an interrupt handler, you have to be sure the GPIO controller callback functions will not sleep.
```c
A well-designed controller driver should be able to inform other drivers (actually clients)
```
whether calls to its methods may sleep or not. This can be checked with the gpio_cansleep() function.
None of the functions used to access GPIO return an error code. That is why you should pay attention and check return values during GPIO
allocation and configuration.
## In atomic context
There are GPIO controllers that can be accessed and managed through simple memory read/write operations. These are generally embedded in the SoC, and do not need to sleep.
```c
gpio_cansleep() will always return false for those controllers. For such GPIOs, you can get/set their value from within an IRQ handler, using the well-known gpio_get_value()
```
or gpio_set_value(), depending on the GPIO line being configured as input or output:
```c
static int gpio_get_value(unsigned gpio)
void gpio_set_value(unsigned int gpio, int value);
gpio_get_value() should be used when the GPIO is configured as input (using gpio_direction_input()), and return the actual value (state) of the GPIO. On the other hand, gpio_set_value() will affect the value of the GPIO, which should have been configured as an output using gpio_direction_output(). For both functions, value can be considered as Boolean, where zero means low, and non-zero values mean high.
```
## In a non-atomic context (that may sleep)
On the other hand, there are GPIO controllers wired on buses such as SPI and I2C. Since functions accessing those buses may lead to sleep, the gpio_cansleep() function should always return true (it is up to the GPIO controller to take of returning true). In this case,
you should not access those GPIOs from within the IRQ handles, at least not in the top half
(the hard IRQ). Moreover, the accessors you have to use as your general-purpose access should be suffixed with _cansleep:
```c
static int gpio_get_value_cansleep(unsigned gpio);
void gpio_set_value_cansleep(unsigned gpio, int value);
```
They behave exactly like accessors without the _cansleep() name suffix, with the only difference being that they prevent the kernel from printing warnings when the GPIOs are accessed.
## GPIOs mapped to IRQ
Input GPIOs can often be used as IRQ signals. Such IRQs can be edge-triggered or leveltriggered. The configuration depends on your needs. The GPIO controller is responsible for providing the mapping between the GPIO and its IRQ. You can use gpio_to_irq() to map a given GPIO number to its IRQ number:
```c
int gpio_to_irq(unsigned gpio);
```
The return value is the IRQ number, on which you can call request_irq() (or the threaded version request_threaded_irq()) in order to register a handler for this IRQ:
```c
static irqreturn_t my_interrupt_handler(int irq, void *dev_id)
{
```
[...]
```c
return IRQ_HANDLED;
}
```
[...]
```c
int gpio_int = of_get_gpio(np, 0);
int irq_num = gpio_to_irq(gpio_int);
int error = devm_request_threaded_irq(&client->dev, irq_num,
```
NULL, my_interrupt_handler,
IRQF_TRIGGER_RISING | IRQF_ONESHOT,
```c
input_dev->name, my_data_struct);
if (error) {
dev_err(&client->dev, "irq %d requested failed, %d\n",
client->irq, error);
return error;
}
```
## Putting it all together
The following code is a summary, putting into practice all the concepts discussed regarding integer-based interfaces. This driver manages four GPIOs: two buttons (btn1 and btn2) and two LEDs (green and red). Btn1 is mapped to an IRQ, and whenever its state changes to
LOW, the state of btn2 is applied to the LEDs. For example, if the state of btn1 goes LOW
while btn2 is high, the GREEN and RED LEDs will be driven to HIGH:
```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h> /* For Legacy integer based GPIO */
#include <linux/interrupt.h> /* For IRQ */
static unsigned int GPIO_LED_RED = 49;
static unsigned int GPIO_BTN1 = 115;
static unsigned int GPIO_BTN2 = 116;
static unsigned int GPIO_LED_GREEN = 120;
static int irq;
static irqreturn_t btn1_pushed_irq_handler(int irq, void *dev_id)
{
int state;
```
/* read BTN2 value and change the led state */
state = gpio_get_value(GPIO_BTN2);
```c
gpio_set_value(GPIO_LED_RED, state);
gpio_set_value(GPIO_LED_GREEN, state);
pr_info("GPIO_BTN1 interrupt: Interrupt! GPIO_BTN2 state is %d)\n",
```
state);
```c
return IRQ_HANDLED;
}
static int __init helloworld_init(void)
{
int retval;
```
/*
* One could have checked whether the GPIO is valid on the controller or not,
* using gpio_is_valid() function.
* Ex:
```c
* if (!gpio_is_valid(GPIO_LED_RED)) {
```
* pr_infor("Invalid Red LED\n");
* return -ENODEV;
* }
*/
```c
gpio_request(GPIO_LED_GREEN, "green-led");
gpio_request(GPIO_LED_RED, "red-led");
gpio_request(GPIO_BTN1, "button-1");
gpio_request(GPIO_BTN2, "button-2");
```
/*
* Configure Button GPIOs as input
*
* After this, one can call gpio_set_debounce()
* only if the controller has the feature
*
* For example, to debounce a button with a delay of 200ms
* gpio_set_debounce(GPIO_BTN1, 200);
*/
```c
gpio_direction_input(GPIO_BTN1);
gpio_direction_input(GPIO_BTN2);
```
/*
* Set LED GPIOs as output, with their initial values set to 0
*/
```c
gpio_direction_output(GPIO_LED_RED, 0);
gpio_direction_output(GPIO_LED_GREEN, 0);
```
irq = gpio_to_irq(GPIO_BTN1);
retval = request_threaded_irq(irq, NULL,\
btn1_pushed_irq_handler, \
IRQF_TRIGGER_LOW | IRQF_ONESHOT, \
"device-name", NULL);
```c
pr_info("Hello world!\n");
return 0;
}
static void __exit hellowolrd_exit(void)
{
free_irq(irq, NULL);
gpio_free(GPIO_LED_RED);
gpio_free(GPIO_LED_GREEN);
gpio_free(GPIO_BTN1);
gpio_free(GPIO_BTN2);
pr_info("End of the world\n");
}
module_init(hellowolrd_init);
module_exit(hellowolrd_exit);
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_LICENSE("GPL");
```
The descriptor-based GPIO interface – the new and recommended way
With the new descriptor-based GPIO interface, a GPIO is characterized by a coherent struct gpio_desc structure:
```c
struct gpio_desc {
struct gpio_chip *chip;
unsigned long flags;
```
const char *label;
```c
};
```
You should use the following header to be able to use the new interface:
```c
#include <linux/gpio/consumer.h>
```
With the descriptor-based interface, prior to allocating and taking the ownership of GPIOs,
those GPIOs must have been mapped somewhere. By mapped, I mean they should be assigned to your device, whereas, with the legacy integer-based interface, you just have to fetch a number anywhere and request it as a GPIO. Actually, there are three kinds of mapping in the kernel:
Platform data mapping: The mapping is done in the board file.
Device tree: The mapping is done in DT style, as discussed in the preceding sections. This is the mapping we will discuss in this book.
Advanced Configuration and Power Interface mapping (ACPI): The mapping is done in ACPI style. Generally used on x86-based systems.
## GPIO descriptor mapping - the device tree
GPIO descriptor mappings are defined in the consumer device's node. The property that contains a GPIO descriptor mapping must be named <name>-gpios or <name>-gpio,
where <name> is meaningful enough to describe the function for which those GPIOs will be used.
You should always suffix the property name with either -gpio or -gpios because every descriptor-based interface function relies on the gpio_suffixes[] variable, defined in drivers/gpio/gpiolib.h and shown as follows:
/* gpio suffixes used for ACPI and device tree lookup */
```c
static const char * const gpio_suffixes[] = { "gpios", "gpio" };
```
Let's see how to do this by having a look at the function used to look for GPIO descriptor mappings in devices in the DT:
```c
static struct gpio_desc *of_find_gpio(struct device *dev,
```
const char *con_id,
```c
unsigned int idx,
enum gpio_lookup_flags *flags)
{
```
char prop_name[32]; /* 32 is max size of property name */
```c
enum of_gpio_flags of_flags;
struct gpio_desc *desc;
unsigned int i;
for (i = 0; i < ARRAY_SIZE(gpio_suffixes); i++) {
if (con_id)
```
snprintf(prop_name, sizeof(prop_name), "%s-%s",
con_id,
```c
gpio_suffixes[i]);
```
else snprintf(prop_name, sizeof(prop_name), "%s",
```c
gpio_suffixes[i]);
desc = of_get_named_gpiod_flags(dev->of_node,
```
prop_name, idx,
&of_flags);
```c
if (!IS_ERR(desc) || (PTR_ERR(desc) == -EPROBE_DEFER))
```
break;
```c
}
if (IS_ERR(desc))
return desc;
if (of_flags & OF_GPIO_ACTIVE_LOW)
```
*flags |= GPIO_ACTIVE_LOW;
```c
return desc;
}
```
Now, let's consider the following node, which is an excerpt from Documentation/gpio/board.txt:
```c
foo_device {
dts
compatible = "acme,foo";
```
[...]
```dts
led-gpios = <&gpio 15 GPIO_ACTIVE_HIGH>, /* red */
```
<&gpio 16 GPIO_ACTIVE_HIGH>, /* green */
<&gpio 17 GPIO_ACTIVE_HIGH>; /* blue */
```dts
power-gpios = <&gpio 1 GPIO_ACTIVE_LOW>;
reset-gpios = <&gpio 1 GPIO_ACTIVE_LOW>;
c
};
```
This is what a mapping should look like, with a meaningful name.
## Allocating and using GPIO
You can use either gpiog_get() or gpiod_get_index() to allocate a GPIO descriptor:
```c
struct gpio_desc *gpiod_get_index(struct device *dev,
```
const char *con_id,
```c
unsigned int idx,
enum gpiod_flags flags)
struct gpio_desc *gpiod_get(struct device *dev,
```
const char *con_id,
```c
enum gpiod_flags flags)
```
On error, these functions will return -ENOENT if no GPIO with the given function is assigned, or another error on which you can use the IS_ERR() macro. The first function returns the GPIO descriptor structure that corresponds to the GPIO at a given index,
whereas the second function returns the GPIO at index 0 (useful for one-GPIO mapping).
dev is the device to which the GPIO descriptor will belong. It is your device. con_id is the function within the GPIO consumer. It corresponds to the <name> prefix of the property name in the DT. idx is the index (starting from 0) of the GPIO for which you need a descriptor. flags is an optional parameter that determines GPIO initialization flags, to configure direction and/or output value. It is an instance of enum gpiod_flags, defined in include/linux/gpio/consumer.h:
```c
enum gpiod_flags {
```
GPIOD_ASIS = 0,
GPIOD_IN = GPIOD_FLAGS_BIT_DIR_SET,
GPIOD_OUT_LOW = GPIOD_FLAGS_BIT_DIR_SET |
GPIOD_FLAGS_BIT_DIR_OUT,
GPIOD_OUT_HIGH = GPIOD_FLAGS_BIT_DIR_SET |
GPIOD_FLAGS_BIT_DIR_OUT |
GPIOD_FLAGS_BIT_DIR_VAL,
```c
};
```
Now, let's allocate GPIO descriptors for mappings defined in the preceding DT:
```c
struct gpio_desc *red, *green, *blue, *power;
```
red = gpiod_get_index(dev, "led", 0, GPIOD_OUT_HIGH);
green = gpiod_get_index(dev, "led", 1, GPIOD_OUT_HIGH);
blue = gpiod_get_index(dev, "led", 2, GPIOD_OUT_HIGH);
power = gpiod_get(dev, "power", GPIOD_OUT_HIGH);
The LED GPIOs will be active-high, while the power GPIO will be active-low (that is,
```c
gpiod_is_active_low(power) will be true). The reverse operation to allocation is done with the gpiod_put() function:
gpiod_put(struct gpio_desc *desc);
```
Let's see how you can release red and blue GPIO LEDs:
```c
gpiod_put(blue);
gpiod_put(red);
```
Before we go further, keep in mind that, apart from the gpiod_get()/gpiod_get_index() and gpio_put() functions, which completely differ from gpio_request() and gpio_free(), you can perform API translation from integerbased interfaces to descriptor-based ones just by changing the gpio_ prefix to gpiod_.
To change direction, you should use the gpiod_direction_input() and gpiod_direction_output() functions:
```c
int gpiod_direction_input(struct gpio_desc *desc);
int gpiod_direction_output(struct gpio_desc *desc, int value);
```
value is the state to apply to the GPIO once the direction is set to output. If the GPIO
controller has this feature, you can set the debounce timeout of a given GPIO using its descriptor:
```c
int gpiod_set_debounce(struct gpio_desc *desc, unsigned debounce);
```
In order to access a GPIO given its descriptor, the same attention must be paid as with the integer-based interface. In other words, you should take note of whether you are in an atomic (cannot sleep) or non-atomic context, and then use the appropriate function:
```c
int gpiod_cansleep(const struct gpio_desc *desc);
```
/* Value get/set from sleeping context */
```c
int gpiod_get_value_cansleep(const struct gpio_desc *desc);
void gpiod_set_value_cansleep(struct gpio_desc *desc, int value);
```
/* Value get/set from non-sleeping context */
```c
int gpiod_get_value(const struct gpio_desc *desc);
void gpiod_set_value(struct gpio_desc *desc, int value);
```
For a GPIO descriptor mapped to IRQ, you can use gpiod_to_irq() in order to get the
IRQ number that corresponds to the given GPIO descriptor, which can be used with the request_irq() function:
```c
int gpiod_to_irq(const struct gpio_desc *desc);
```
At any given point in the code, you can switch from the descriptor-based interface to the legacy integer-based interface and vice versa, using the desc_to_gpio() or gpio_to_desc() functions:
/* Convert between the old gpio_ and new gpiod_ interfaces */
```c
struct gpio_desc *gpio_to_desc(unsigned gpio);
int desc_to_gpio(const struct gpio_desc *desc);
```
## Putting it all together
The driver here summarizes the concepts introduced in descriptor-based interfaces. The principle is the same, as are the GPIOs:
```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h> /* For platform devices */
#include <linux/gpio/consumer.h> /* For GPIO Descriptor */
#include <linux/interrupt.h> /* For IRQ */
#include <linux/of.h> /* For DT*/
```
/*
* Let us consider the below mapping in device tree:
*
```c
* foo_device {
dts
* compatible = "packt,gpio-descriptor-sample";
* led-gpios = <&gpio2 15 GPIO_ACTIVE_HIGH>, // red
```
* <&gpio2 16 GPIO_ACTIVE_HIGH>, // green
*
```dts
* btn1-gpios = <&gpio2 1 GPIO_ACTIVE_LOW>;
* btn2-gpios = <&gpio2 31 GPIO_ACTIVE_LOW>;
c
* };
```
*/
```c
static struct gpio_desc *red, *green, *btn1, *btn2;
static int irq;
static irqreturn_t btn1_pushed_irq_handler(int irq, void *dev_id)
{
int state;
```
/* read the button value and change the led state */
state = gpiod_get_value(btn2);
```c
gpiod_set_value(red, state);
gpiod_set_value(green, state);
pr_info("btn1 interrupt: Interrupt! btn2 state is %d)\n",
```
state);
```c
return IRQ_HANDLED;
}
static const struct of_device_id gpiod_dt_ids[] = {
dts
{ .compatible = "packt,gpio-descriptor-sample", },
c
{ /* sentinel */ }
};
static int my_pdrv_probe (struct platform_device *pdev)
{
int retval;
struct device *dev = &pdev->dev;
```
/*
* We use gpiod_get/gpiod_get_index() along with the flags
* in order to configure the GPIO direction and an initial
* value in a single function call.
*
* One could have used:
* red = gpiod_get_index(dev, "led", 0);
* gpiod_direction_output(red, 0);
*/
red = gpiod_get_index(dev, "led", 0, GPIOD_OUT_LOW);
green = gpiod_get_index(dev, "led", 1, GPIOD_OUT_LOW);
/*
* Configure GPIO Buttons as input
*
* After this, one can call gpiod_set_debounce()
* only if the controller has the feature
* For example, to debounce a button with a delay of 200ms
* gpiod_set_debounce(btn1, 200);
*/
btn1 = gpiod_get(dev, "led", 0, GPIOD_IN);
btn2 = gpiod_get(dev, "led", 1, GPIOD_IN);
irq = gpiod_to_irq(btn1);
retval = request_threaded_irq(irq, NULL,\
btn1_pushed_irq_handler, \
IRQF_TRIGGER_LOW | IRQF_ONESHOT, \
"gpio-descriptor-sample", NULL);
```c
pr_info("Hello! device probed!\n");
return 0;
}
static void my_pdrv_remove(struct platform_device *pdev)
{
free_irq(irq, NULL);
gpiod_put(red);
gpiod_put(green);
gpiod_put(btn1);
gpiod_put(btn2);
pr_info("good bye reader!\n");
}
static struct platform_driver mypdrv = {
```
.probe = my_pdrv_probe,
.remove = my_pdrv_remove,
```c
.driver = {
```
.name = "gpio_descriptor_sample",
.of_match_table = of_match_ptr(gpiod_dt_ids),
.owner = THIS_MODULE,
```c
},
};
module_platform_driver(mypdrv);
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_LICENSE("GPL");
```
## The GPIO interface and the device tree
```dts
Whatever interface you need to use GPIO for, how to specify GPIOs depends on the controller providing them, especially regarding its #gpio-cells property, which determines the number of cells used for a GPIO specifier. A GPIO specifier contains at least the controller phandle and one or more arguments, where the number of arguments on the #gpio-cells property of the controller that provides the GPIO. The first cell is generally the GPIO offset number on the controller, and the second represents the GPIO
```
flags. GPIO properties should be named [<name>-]gpios], with <name> being the purpose of this GPIO for the device. Keep in mind this rule is a must for descriptor-based interfaces, and becomes <name>-gpios (note the absence of square brackets, meaning that the <name> prefix is mandatory):
```c
gpio1: gpio1 {
dts
gpio-controller;
#gpio-cells = <2>;
c
};
gpio2: gpio2 {
dts
gpio-controller;
#gpio-cells = <1>;
c
};
```
[...]
```dts
cs-gpios = <&gpio1 17 0>,
```
<&gpio2 2>;
<0>, /* holes are permitted, means no GPIO 2 */
<&gpio1 17 0>;
```dts
reset-gpios = <&gpio1 30 0>;
cd-gpios = <&gpio2 10>;
```
In the preceding sample, CS GPIOs contain both controller-1 and controller-2 GPIOs. If you do not need to specify a GPIO at a given index in the list, you can use <0>. The reset GPIO
has two cells (two arguments after the controller phandle), whereas the CD GPIO has only one cell. You can see how meaningful the names are that I gave to my GPIO specifier.
## The legacy integer-based interface and device tree
This interface relies on the following header:
```c
#include <linux/of_gpio.h>
```
There are two functions you should remember when you need to support DT from within your driver using legacy integer-based interfaces; these are of_get_named_gpio() and of_get_named_gpio_count():
```c
int of_get_named_gpio(struct device_node *np,
```
const char *propname, int index)
```c
int of_get_named_gpio_count(struct device_node *np,
```
const char* propname)
Given a device node, the former returns the GPIO number of the *propname property at the index position. The second just returns the number of GPIOs specified in the property:
```dts
int n_gpios = of_get_named_gpio_count(dev.of_node,
```
"cs-gpios"); /* return 4 */
```c
int second_gpio = of_get_named_gpio(dev.of_node, "cs-gpio", 1);
int rst_gpio = of_get_named_gpio("reset-gpio", 0);
gpio_request(second_gpio, "my-gpio);
```
There are drivers still supporting the old specifier, where GPIO properties are named
[<name>-gpio] or gpios. In that case, you should use unnamed API versions, by means of of_get_gpio() and of_gpio_count():
```c
int of_gpio_count(struct device_node *np)
int of_get_gpio(struct device_node *np, int index)
```
The DT node would look like this:
```c
my_node@addr {
dts
compatible = "[...]";
gpios = <&gpio1 2 0>, /* INT */
```
<&gpio1 5 0>; /* RST */
[...]
```c
};
```
The code in the driver would look like this:
```c
struct device_node *np = dev->of_node;
if (!np)
return ERR_PTR(-ENOENT);
dts
int n_gpios = of_gpio_count(); /* Will return 2 */
c
int gpio_int = of_get_gpio(np, 0);
if (!gpio_is_valid(gpio_int)) {
dev_err(dev, "failed to get interrupt gpio\n");
return ERR_PTR(-EINVAL);
}
gpio_rst = of_get_gpio(np, 1);
if (!gpio_is_valid(pdata->gpio_rst)) {
dev_err(dev, "failed to get reset gpio\n");
return ERR_PTR(-EINVAL);
}
```
You can summarize this by rewriting the first driver (the one for integer-based interfaces),
in order to comply with the platform driver structure, and use the DT API:
```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h> /* For platform devices */
#include <linux/interrupt.h> /* For IRQ */
#include <linux/gpio.h> /* For Legacy integer based GPIO */
#include <linux/of_gpio.h> /* For of_gpio* functions */
#include <linux/of.h> /* For DT*/
```
/*
* Let us consider the following node
*
```c
* foo_device {
dts
* compatible = "packt,gpio-legacy-sample";
* led-gpios = <&gpio2 15 GPIO_ACTIVE_HIGH>, // red
```
* <&gpio2 16 GPIO_ACTIVE_HIGH>, // green
*
```dts
* btn1-gpios = <&gpio2 1 GPIO_ACTIVE_LOW>;
* btn2-gpios = <&gpio2 1 GPIO_ACTIVE_LOW>;
c
* };
```
*/
```c
static unsigned int gpio_red, gpio_green, gpio_btn1, gpio_btn2;
static int irq;
static irqreturn_t btn1_pushed_irq_handler(int irq, void *dev_id)
{
```
/* The content of this function remains unchanged */
[...]
```c
}
static const struct of_device_id gpio_dt_ids[] = {
dts
{ .compatible = "packt,gpio-legacy-sample", },
c
{ /* sentinel */ }
};
static int my_pdrv_probe (struct platform_device *pdev)
{
int retval;
struct device_node *np = &pdev->dev.of_node;
if (!np)
return ERR_PTR(-ENOENT);
gpio_red = of_get_named_gpio(np, "led", 0);
gpio_green = of_get_named_gpio(np, "led", 1);
gpio_btn1 = of_get_named_gpio(np, "btn1", 0);
gpio_btn2 = of_get_named_gpio(np, "btn2", 0);
gpio_request(gpio_green, "green-led");
gpio_request(gpio_red, "red-led");
gpio_request(gpio_btn1, "button-1");
gpio_request(gpio_btn2, "button-2");
```
/* Code to configure GPIO and request IRQ remains unchanged */
[...]
```c
return 0;
}
static void my_pdrv_remove(struct platform_device *pdev)
{
```
/* The content of this function remains unchanged */
[...]
```c
}
static struct platform_driver mypdrv = {
```
.probe = my_pdrv_probe,
.remove = my_pdrv_remove,
```c
.driver = {
```
.name = "gpio_legacy_sample",
.of_match_table = of_match_ptr(gpio_dt_ids),
.owner = THIS_MODULE,
```c
},
};
module_platform_driver(mypdrv);
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_LICENSE("GPL");
```
## GPIO mapping to IRQ in the device tree
You can easily map the GPIO to the IRQ in the device tree. Two properties are used to specify an interrupt:
```dts
interrupt-parent: This is the GPIO controller for the GPIO
```
interrupts: This is the interrupts specifier list
These apply to legacy and descriptor-based interfaces. The IRQ specifier depends on the
#interrupt-cell property of the GPIO controller providing this GPIO. #interruptcell determines the number of cells used when specifying the interrupt. Generally, the first cell represents the GPIO number to map to an IRQ and the second cell represents level/edge should trigger the interrupt. In any case, the interrupt specifier always depends on its parent (the one that has the interrupt-controller set), so refer to its binding documentation in the kernel source:
```c
gpio4: gpio4 {
dts
gpio-controller;
#gpio-cells = <2>;
```
interrupt-controller;
```dts
#interrupt-cells = <2>;
c
};
my_label: node@0 {
dts
reg = <0>;
```
spi-max-frequency = <1000000>;
```dts
interrupt-parent = <&gpio4>;
interrupts = <29 IRQ_TYPE_LEVEL_LOW>;
c
};
```
There are two solutions for obtaining the corresponding IRQ:
1. Your device sits on a known bus (I2C or SPI): The IRQ mapping will be done for you, and made available either through the struct i2c_client or struct spi_device structure given to your probe() function (by means of i2c_client.irq or spi_device.irq)
2. Your device sits on the pseudo-platform bus: The probe() function will be given a struct platform_device, on which you can call platform_get_irq():
```c
int platform_get_irq(struct platform_device *dev, unsigned int num);
```
Feel free to revisit Chapter 6, The Concept of a Device Tree.
## GPIO and sysfs
The sysfs GPIO interface lets people manage and control GPIOs through sets or files. It is located under /sys/class/gpio. The device model is heavily used here, and there are three kinds of entry available:
```bash
/sys/class/gpio/: This is where everything begins. This directory contains two special files, export and unexport:
```
export: This allow us to ask the kernel to export control of a given
GPIO to the user space by writing its number to this file. An example is echo 21 > export, which will create a GPIO21 node for GPIO #21, if that's not requested by the kernel code.
unexport: This reverses the effect of exporting to user space.
Example: echo 21 > unexport will remove any GPIO21 node exported using the export file.
```bash
/sys/class/gpio/gpioN/: This directory corresponds to the GPIO number N
```
(where N is global to the system, not relative to the chip), exported either using the export file or from within the kernel. For example, /sys/class/gpio/gpio42/ (for GPIO #42) with the following read/write attributes:
The direction file is used to get/set GPIO direction. Allowed values are either in or out strings. This value may normally be written. Writing as out defaults to initializing the value as low. To ensure glitch-free operation, low and high values may be written to configure the GPIO as an output with that initial value. This attribute will not exist if the kernel code has exported this GPIO,
disabling direction (see the gpiod_export() or gpio_export()
function).
The value attribute lets us get/set the state of the GPIO line,
depending on the direction, input, or output. If the GPIO is configured as an output, any non-zero value written will be treated as a HIGH state. If configured as an output, writing 0 will set the output low, whereas 1 will set the output high. If the pin can be configured as an interrupt-generating lines and if it has been configured to generate, you can call the poll(2) system call on that file and poll(2) will return whenever the interrupt was triggered. Using poll(2) wil require setting the events POLLPRI
and POLLERR. If you use select(2) instead, you should set the file descriptor in exceptfds. After poll(2) returns, either lseek(2) to the beginning of the sysfs file and read the new value or close the file and re-open it to read the value. It is the same principle we discussed regarding the pollable sysfs attribute.
edge determines the signal edge that will let the poll() or select() function return. Allowed values are none, rising,
falling, or both. This file is readable/writable, and exists only if the pin can be configured as an interrupt-generating input pin.
active_low reads as either 0 (false) or 1 (true). Writing any nonzero value will invert the value attribute for both reading and writing. Existing and subsequent poll(2) support configurations through the edge attribute for rising and falling edges will follow this setting. The relevant function from the kernel to set this value is gpio_sysf_set_active_low().
## Exporting a GPIO from kernel code
Apart from using the /sys/class/gpio/export file to export a GPIO to the user space,
you can use functions such as gpio_export (for legacy interfaces) or gpioD_export (the new interface) from the kernel code in order to explicitly manage exporting the GPIOs that have already been requested using gpio_request() or gpiod_get():
```c
int gpio_export(unsigned gpio, bool direction_may_change);
int gpiod_export(struct gpio_desc *desc, bool direction_may_change);
```
The direction_may_change parameter decides whether you can change the signal direction from input to output and vice versa. The reverse operations from the kernel are gpio_unexport() or gpiod_unexport():
```c
void gpio_unexport(unsigned gpio); /* Integer-based interface */
void gpiod_unexport(struct gpio_desc *desc) /* Descriptor-based */
```
Once exported, you can use gpio_export_link() (or gpiod_export_link() for descriptor-based interfaces) in order to create symbolic links from elsewhere in sysfs, which will point to the GPIO sysfs node. Drivers can use this to provide the interface under their own device in sysfs with a descriptive name:
```c
int gpio_export_link(struct device *dev, const char *name,
unsigned gpio)
int gpiod_export_link(struct device *dev, const char *name,
struct gpio_desc *desc)
```
You could use this in the probe() function for descriptor-based interfaces as follows:
```c
static struct gpio_desc *red, *green, *btn1, *btn2;
static int my_pdrv_probe (struct platform_device *pdev)
{
```
[...]
red = gpiod_get_index(dev, "led", 0, GPIOD_OUT_LOW);
green = gpiod_get_index(dev, "led", 1, GPIOD_OUT_LOW);
```c
gpiod_export(&pdev->dev, "Green_LED", green);
gpiod_export(&pdev->dev, "Red_LED", red);
```
[...]
```c
return 0;
}
```
For integer-based interfaces, the code would look like this:
```c
static int my_pdrv_probe (struct platform_device *pdev)
{
```
[...]
```c
gpio_red = of_get_named_gpio(np, "led", 0);
gpio_green = of_get_named_gpio(np, "led", 1);
```
[...]
```c
int gpio_export_link(&pdev->dev, "Green_LED", gpio_green)
int gpio_export_link(&pdev->dev, "Red_LED", gpio_red)
return 0;
}
```
## Summary
Dealing with a GPIO from within the kernel is an easy task, as shown in this chapter. Both legacy and new interfaces were discussed, giving the possibility to choose the one that fits your needs, in order to write enhanced GPIO drivers. You should now be able to handle
IRQs mapped to GPIOs. The next chapter will deal with the chip that provides and exposes
GPIO lines, known as the GPIO controller