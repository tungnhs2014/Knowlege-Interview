```bash
# Chapter 19 - PWM Drivers
```
Pulse Wide Modulation (PWM) operates like a switch that constantly cycles on and off. It is a hardware feature used to control servomotors, for voltage regulation and so on. The most well-known applications of PWM are:
Motor speed control
Light dimming
Voltage regulation
Now, let's introduce PWM with a simple diagram:
The preceding figure illustrates a complete PWM cycle, introducing some terms that we need to clarify prior to getting deeper into the kernel PWM framework:
Ton: This is the duration during which the signal is high.
Toff: This is the duration during which the signal is low.
Period: This is the duration of a complete PWM cycle. It represents the sum of the Ton and Toff of the PWM signal.
Duty cycle: This is represented as a percentage of the time signal that remains on during the period of the PWM signal.
The different formulas are detailed as follows:
PWM period:
Duty cycle:
You can find details about PWM at https://en.wikipedia.org/wiki/
Pulse-width_modulation.
The Linux PWM framework has two interfaces:
1. Controller interface: The one that exposes the PWM line. It is the PWM chip, that is, the producer.
2. Consumer interface: The device consuming PWM lines exposed by the controller. The drivers of such devices use helper functions exported by the controller by means of a generic PWM framework.
Either the consumer or producer interface depends on the following header file:
```c
#include <linux/pwm.h>
```
In this chapter, we will deal with the following:
The PWM driver architecture and data structures, for both the controller and consumer, along with a dummy driver
Instantiating PWM devices and controllers in the device tree
Requesting and consuming PWM devices
Using PWM from the user space, through the sysfs interface
## PWM controller driver
As you need struct gpio_chip when writing GPIO-controller drivers and struct irq_chip when writing IRQ-controller drivers, a PWM controller is represented in the kernel as an instance of the struct pwm_chip structure:
```c
## PWM controller and devices struct pwm_chip {
struct device *dev;
```
const struct pwm_ops *ops;
```c
int base;
unsigned int npwm;
struct pwm_device *pwms;
struct pwm_device * (*of_xlate)(struct pwm_chip *pc,
```
const struct of_phandle_args *args);
```c
unsigned int of_pwm_n_cells;
```
bool can_sleep;
```c
};
```
The following lists the meaning of each element in the structure:
dev: This represents the device associated with the chip.
Ops: This is a data structure providing callback functions this chip exposes to consumer drivers.
```c
Base: This is the number of the first PWM controlled by this chip. If chip->base
```
< 0, then the kernel will dynamically assign a base.
can_sleep: This should be set to true by the chip driver if the .config(),
.enable(), or .disable() operations of the ops field might sleep.
npwm: This is the number of PWM channels (devices) that this chip provides.
pwms: This is an array of the PWM devices of this chip, allocated by the framework to consumer drivers.
of_xlate: This is an optional callback to request a PWM device, given a DT
PWM specifier. If not defined, it will be set to of_pwm_simple_xlate by the
PWM core, which will force of_pwm_n_cells to 2, as well.
of_pwm_n_cells: This is the number of cells expected in the DT for a PWM
specifier.
PWM controller/chip addition and removal rely on two basic functions: pwmchip_add()
and pwmchip_remove(). Each function should be given a filled in struct pwm_chip structure as an argument. Their respective prototypes are as follows:
```c
int pwmchip_add(struct pwm_chip *chip)
int pwmchip_remove(struct pwm_chip *chip)
```
Unlike other framework removal functions, which do not have return values,
```c
pwmchip_remove()has a return value. It returns 0 up on success, or -EBUSY if the chip has a PWM line still in use (still requested).
```
Each PWM driver must implement some hooks through the struct pwm_ops field, which is used by the PWM core or the consumer interface to configure and make full use of its
PWM channels. Some of them are optional:
```c
struct pwm_ops {
int (*request)(struct pwm_chip *chip, struct pwm_device *pwm);
void (*free)(struct pwm_chip *chip, struct pwm_device *pwm);
int (*config)(struct pwm_chip *chip, struct pwm_device *pwm,
int duty_ns, int period_ns);
int (*set_polarity)(struct pwm_chip *chip, struct pwm_device *pwm,
enum pwm_polarity polarity);
int (*enable)(struct pwm_chip *chip,struct pwm_device *pwm);
void (*disable)(struct pwm_chip *chip, struct pwm_device *pwm);
void (*get_state)(struct pwm_chip *chip, struct pwm_device *pwm,
struct pwm_state *state); /* since kernel v4.7 */
struct module *owner;
};
```
Let's look at what each element in the structure means:
request: This is an optional hook that, if provided, is executed during a PWM
channel request.
free: This is the same as request; runs during PWM freeing.
config: This is the PMW configuration hook. It configures duty cycles and period length for this PWM.
set_polarity: This hook configures the polarity of this PWM.
Enable: This enables the PWM line, starting output toggling.
Disable: This disables the PWM line, stopping output toggling.
Apply: This automatically applies a new PWM config. The state argument should be adjusted with the real hardware config.
get_state: This returns the current PWM state. This function is only called once per PWM device, when the PWM chip is registered.
Owner: This is the module that owns the chip, usually THIS_MODULE.
In the probe function of the PWM controller driver, it is a good practice to retrieve DT
resources, initialize hardware, fill a struct pwm_chip and its struct pwm_ops, and then add the PWM chip with the pwmchip_add function.
## Driver example
Now, let's summarize things by writing a dummy driver for a PWM controller, which has three channels:
```c
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
struct fake_chip {
struct pwm_chip chip;
int foo;
int bar;
```
/* put the client structure here (SPI/I2C) */
```c
};
static inline struct fake_chip *to_fake_chip(struct pwm_chip *chip)
{
return container_of(chip, struct fake_chip, chip);
}
static int fake_pwm_request(struct pwm_chip *chip,
struct pwm_device *pwm)
{
```
/*
* One may need to do some initialization when a PWM channel
* of the controller is requested. This should be done here.
*
* One may do something like
```c
* prepare_pwm_device(struct pwm_chip *chip, pwm->hwpwm);
```
*/
```c
return 0;
}
static int fake_pwm_config(struct pwm_chip *chip,
struct pwm_device *pwm,
int duty_ns, int period_ns)
{
```
/*
* In this function, one ne can do something like:
* struct fake_chip *priv = to_fake_chip(chip);
*
* return send_command_to_set_config(priv,
* duty_ns, period_ns);
*/
```c
return 0;
}
static int fake_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
```
/*
* In this function, one ne can do something like:
* struct fake_chip *priv = to_fake_chip(chip);
*
```c
* return foo_chip_set_pwm_enable(priv, pwm->hwpwm, true);
```
*/
```c
pr_info("Somebody enabled PWM device number %d of this chip",
pwm->hwpwm);
return 0;
}
static void fake_pwm_disable(struct pwm_chip *chip,
struct pwm_device *pwm)
{
```
/*
* In this function, one ne can do something like:
* struct fake_chip *priv = to_fake_chip(chip);
*
```c
* return foo_chip_set_pwm_enable(priv, pwm->hwpwm, false);
```
*/
```c
pr_info("Somebody disabled PWM device number %d of this chip",
pwm->hwpwm);
}
static const struct pwm_ops fake_pwm_ops = {
```
.request = fake_pwm_request,
.config = fake_pwm_config,
.enable = fake_pwm_enable,
.disable = fake_pwm_disable,
.owner = THIS_MODULE,
```c
};
static int fake_pwm_probe(struct platform_device *pdev)
{
struct fake_chip *priv;
priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
if (!priv)
return -ENOMEM;
priv->chip.ops = &fake_pwm_ops;
priv->chip.dev = &pdev->dev;
priv->chip.base = -1; /* Dynamic base */
priv->chip.npwm = 3; /* 3 channel controller */
platform_set_drvdata(pdev, priv);
return pwmchip_add(&priv->chip);
}
static int fake_pwm_remove(struct platform_device *pdev)
{
struct fake_chip *priv = platform_get_drvdata(pdev);
return pwmchip_remove(&priv->chip);
}
static const struct of_device_id fake_pwm_dt_ids[] = {
dts
{ .compatible = "packt,fake-pwm", },
c
{ }
};
MODULE_DEVICE_TABLE(of, fake_pwm_dt_ids);
static struct platform_driver fake_pwm_driver = {
.driver = {
```
.name = KBUILD_MODNAME,
.owner = THIS_MODULE,
.of_match_table = of_match_ptr(fake_pwm_dt_ids),
```c
},
```
.probe = fake_pwm_probe,
.remove = fake_pwm_remove,
```c
};
module_platform_driver(fake_pwm_driver);
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_DESCRIPTION("Fake pwm driver");
MODULE_LICENSE("GPL");
```
## PWM controller binding
While binding the PWM controller from within the DT, the most important property is
```dts
#pwm-cells. It represents the number of cells used to represent a PWM device on this controller. If you remember, in the struct pwm_chip structure, the of_xlate hook is used to translate a given PWM specifier. If the hook has not been set, pwm-cells must be set to 2; otherwise, it should be set with the same value as of_pwm_n_cells. The following is an example of a PWM controller node in the DT for an i.MX6 SoC:
c
pwm3: pwm@02088000 {
dts
#pwm-cells = <2>;
compatible = "fsl,imx6q-pwm", "fsl,imx27-pwm";
reg = <0x02088000 0x4000>;
interrupts = <0 85 IRQ_TYPE_LEVEL_HIGH>;
clocks = <&clks IMX6QDL_CLK_IPG>,
```
<&clks IMX6QDL_CLK_PWM3>;
```dts
clock-names = "ipg", "per";
status = "disabled";
c
};
```
On the other hand, the node that corresponds to our fake-pwm driver looks like this:
```c
fake_pwm: pwm@0 {
dts
#pwm-cells = <2>;
compatible = "packt,fake-pwm";
```
/*
* Our driver does not use resource
* neither mem, IRQ, nor Clock)
*/
```c
};
```
## The PWM consumer interface
The consumer is the device that actually uses PWM channels. A PWM channel is represented in the kernel as an instance of the struct pwm_device structure:
```c
struct pwm_device {
```
const char *label;
```c
unsigned long flags;
unsigned int hwpwm;
unsigned int pwm;
struct pwm_chip *chip;
void *chip_data;
unsigned int period; /* in nanoseconds */
unsigned int duty_cycle; /* in nanoseconds */
enum pwm_polarity polarity;
};
```
label: This is the name of the PWM device flags: This represents the flags associated with the PWM device hwpw: This is a relative index of the PWM device, local to the chip pwm: This is the system global index of the PWM device chip: This is a PWM chip, the controller providing the PWM device chip_data: This is chip-private data associated with the PWM device
From kernel v4.7, the structure has changed to:
```c
struct pwm_device {
```
const char *label;
```c
unsigned long flags;
unsigned int hwpwm;
unsigned int pwm;
struct pwm_chip *chip;
void *chip_data;
struct pwm_args args;
struct pwm_state state;
};
```
args: This represents the board-dependent PWM arguments attached to this
PWM device, which are usually retrieved from the PWM lookup table or device tree. PWM arguments represent the initial configuration that users want to use on this PWM device rather than the current PWM hardware state.
state: This represents the current PWM channel state:
```c
struct pwm_args {
unsigned int period; /* Device's initial period */
enum pwm_polarity polarity;
};
struct pwm_state {
unsigned int period; /* PWM period (in nanoseconds) */
unsigned int duty_cycle; /* PWM duty cycle (in nanoseconds) */
enum pwm_polarity polarity; /* PWM polarity */
```
bool enabled; /* PWM enabled status */
```c
}
```
Throughout Linux evolution, the PWM framework has faced several changes. These changes concern the way that you request PWM devices from within the consumer side.
We can split the consumer interface into two parts, or, more precisely, into two versions:
The legacy version, where you use pwm_request() and pwm_free() to request a PWM
device and free it after use.
The new and recommended API, using the pwm_get()and pwm_put() functions. The former is given the consumer device, along with the channel name, as arguments to request the PWM device, and the second is given the PWM device, to be freed as a parameter.
Managed variants of these functions, devm_pwm_get() and devm_pwm_put(), also exist:
```c
struct pwm_device *pwm_get(struct device *dev, const char *con_id)
void pwm_put(struct pwm_device *pwm)
pwm_request()/pwm_get() and pwm_free()/pwm_put() cannot be called from an atomic context, since the PWM core make use of mutexes,
```
which may sleep.
After being requested, a PWM has to be configured, using the following:
```c
int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns);
```
To start/stop toggling the PWM output, use pwm_enable()/pwm_disable(). Both functions take a pointer to a struct pwm_device as a parameter, and they are all wrappers around hooks exposed by the controller through the pwm_chip.pwm_ops field:
```c
int pwm_enable(struct pwm_device *pwm)
void pwm_disable(struct pwm_device *pwm)
pwm_enable() returns 0 upon success and a negative error code upon failure. A good example of a PWM consumer driver is drivers/leds/leds-pwm.c, in the kernel source tree. The following is an example of consumer code, driving a PWM LED:
static void pwm_led_drive(struct pwm_device *pwm,
struct private_data *priv)
{
```
/* Configure the PWM, applying a period and duty cycle */
```c
pwm_config(pwm, priv->duty, priv->pwm_period);
```
/* Start toggling */
```c
pwm_enable(pchip->pwmd);
```
[...] /* Do some work */
/* And then stop toggling*/
```c
pwm_disable(pchip->pwmd);
}
```
## PWM client binding
PWM devices can be assigned to the consumer from:
The device tree
ACPI
Static lookup tables, in the board init file
This book will only deal with DT binding, as it is the recommended method. When binding a PWM consumer (client) to its driver, you need to provide the phandle of the controller to which it is linked.
```dts
It is recommended that you give the name pwms to PWM properties; since PWM devices are named resources, you can provide an optional property, pwm-names, containing a list of strings, to name each of the PWM devices listed in the pwms property. If no pwm-names property is given, the name of the user node will be used as a fallback.
Drivers for devices that use more than a single PWM device can use the pwm-names property to map the name of the PWM device requested by the pwm_get() call to an index into the list given by the pwms property.
```
The following example describes a PWM-based backlight device, which is an excerpt from the kernel documentation on PWM device binding (see
Documentation/devicetree/bindings/pwm/pwm.txt):
```c
pwm: pwm {
dts
#pwm-cells = <2>;
c
};
```
[...]
```c
bl: backlight {
dts
pwms = <&pwm 0 5000000>;
pwm-names = "backlight";
c
};
```
The PWM-specifier typically encodes the chip-relative PWM number and the PWM period in nanoseconds, with the following line:
```dts
pwms = <&pwm 0 5000000>;
0 corresponds to the PWM index, relative to the controller, and 5000000 represents the period, in nanoseconds. Note that in the preceding example, specifying the pwm-names is redundant, because the name backlight would be used as a fallback, anyway. Therefore,
```
the driver would have to call the following:
```c
static int my_consummer_probe(struct platform_device *pdev)
{
struct pwm_device *pwm;
pwm = pwm_get(&pdev->dev, "backlight");
if (IS_ERR(pwm)) {
pr_info("unable to request PWM, trying legacy API\n");
```
/*
* Some drivers use the legacy API as fallback, in order
* to request a PWM ID, global to the system
* pwm = pwm_request(global_pwm_id, "pwm beeper");
*/
```c
}
```
[...]
```c
return 0;
}
```
The PWM specifier typically encodes the chip-relative PWM number and the PWM period, in nanoseconds.
## Using PWMs with the sysfs interface
The PWM core sysfs root path is /sys/class/pwm/. It is the user space way to manage
PWM devices. Each PWM controller/chip added to the system creates a pwmchipN
directory entry under the sysfs root path, where N is the base of the PWM chip. The directory contains the following files:
npwm: This is a read-only file, printing the number of PWM channels that the chip supports
Export: This is a write-only file, allowing you to export a PWM channel for use with sysfs (this functionality is equivalent to the GPIO sysfs interface)
Unexport: This unexports a PWM channel from sysfs (write-only)
The PWM channels are numbered using an index from 0 to pwm<n-1>. These numbers are local to the chip. Each PWM channel exportation creates a pwmX directory in the pwmchipN,
which is the same directory as the one containing the export file used. X is the number of the channel that was exported. Each channel directory contains the following files:
Period: This is a readable/writable file to get/set the total period of the PWM
signal. The value is in nanoseconds.
duty_cycle: This is a readable/writable file to get/set the duty cycle of the PWM
signal. It represents the active time of the PWM signal. The value is in nanoseconds, and must always be less than the period.
Polarity: This is a readable/writable file to be used only if the chip of this PWM
device supports polarity inversion. It is better to change the polarity only when this PWM is not enabled. Accepted values are string normal or inversed.
Enable: This is a readable/writable file, to enable (start toggling) or disable (stop toggling) the PWM signal. The accepted values are as follows:
0: disabled
1: enabled
The following is an example of using PWM from the user space through the sysfs interface:
1. Enable PWM:
```bash
# echo 1 > /sys/class/pwm/pwmchip<pwmchipnr>/pwm<pwmnr>/enable
```
2. Set the PWM period:
```bash
# echo <value in nanoseconds> >
/sys/class/pwm/pwmchip<pwmchipnr>/pwm<pwmnr>/period
```
3. Set the PWM duty cycle; the value of the duty cycle must be less than the value of the PWM period:
```bash
# echo <value in nanoseconds> >
/sys/class/pwm/pwmchip<pwmchipnr>/pwm<pwmnr>/duty_cycle
```
4. Disable PWM:
```bash
# echo 0 > /sys/class/pwm/pwmchip<pwmchipnr>/pwm<pwmnr>/enable
```
The complete PWM framework API and sysfs description is available in the Documentation/pwm.txt file, in the kernel source tree.
## Summary
Having reached the end of this chapter, you are now armed to deal with any PWM
controller, whether it is memory mapped or externally sitting on a bus. The API described in this chapter will be sufficient to write and enhance controller driver as a consumer device driver. If you are not comfortable with the PWM kernel side yet, you can use the user space sysfs interface fully. In the next chapter, we will discuss regulators, which are sometimes driven by PWM. So, please hold on; we are almost done