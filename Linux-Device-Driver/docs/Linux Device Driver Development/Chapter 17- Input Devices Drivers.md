```bash
# Chapter 17 - Input Devices Drivers
```
Input devices are devices with which you can interact with the system. Such devices include buttons, keyboards, touchscreens, the mouse, and so on. They work by sending events that are caught and broadcast over the system by the input core. This chapter will explain each structure used by the input core to handle input devices. We will also illustrate how you can manage events from the user space.
In this chapter, we will cover the following topics:
Inputting core data structures
Allocating and registering input devices, as well as the polled device family
Generating and reporting events to the input core
Inputting devices from user space
Writing a driver example
## Inputting device structures
First of all, the main file to include, in order to interface with the input subsystem is linux/input.h:
```c
#include <linux/input.h>
```
No matter what type of input device it is, and what type of event it sends, an input device is represented in the kernel as an instance of struct input_dev:
```c
struct input_dev {
```
const char *name;
const char *phys;
```c
unsigned long evbit[BITS_TO_LONGS(EV_CNT)];
unsigned long keybit[BITS_TO_LONGS(KEY_CNT)];
unsigned long relbit[BITS_TO_LONGS(REL_CNT)];
unsigned long absbit[BITS_TO_LONGS(ABS_CNT)];
unsigned long mscbit[BITS_TO_LONGS(MSC_CNT)];
unsigned int repeat_key;
int rep[REP_CNT];
struct input_absinfo *absinfo;
unsigned long key[BITS_TO_LONGS(KEY_CNT)];
int (*open)(struct input_dev *dev);
void (*close)(struct input_dev *dev);
unsigned int users;
struct device dev;
unsigned int num_vals;
unsigned int max_vals;
struct input_value *vals;
```
bool devres_managed;
```c
};
```
The meanings of the preceding fields are as follows:
name represents the name of the device.
phys is the physical path to the device in the system hierarchy.
evbit is a bitmap of the types of events supported by the device. Some types of area are as follows:
EV_KEY is for devices supporting sending key events (keyboards,
button, and so on).
EV_REL is for devices supporting sending relative positions (the mouse, digitizers, and so on).
EV_ABS is for devices supporting sending absolute positions
(joysticks, for example).
The list of events is available in the kernel source, in the include/linux/inputevent-codes.h file. You can use the set_bit() macro to set the appropriate bit, depending on your input devices capabilities. Of course, a device can support more than one type of event. For example, a mouse will set both EV_KEY and
EV_REL:
```c
set_bit(EV_KEY, my_input_dev->evbit);
set_bit(EV_REL, my_input_dev->evbit);
```
keybit is for an EV_KEY type-enabled device, a bitmap of keys/buttons that this device exposes; for example, BTN_0, KEY_A, KEY_B, and so on. The complete list of keys/buttons is in the include/linux/input-event-codes.h file.
relbit is for an EV_REL type enabled device, a bitmap of relative axes for the device. For example, REL_X, REL_Y, REL_Z, REL_RX, and so on. Take a look at include/linux/input-event-codes.h for the complete list.
absbit is for an EV_ABS type enabled device and shows a bitmap of absolute axes for the device; for example, ABS_Y, ABS_X, and so on. Take a look at the same previous file for the complete list.
mscbit is for EV_MSC type enabled devices and shows a bitmap of miscellaneous events supported by the device.
repeat_key stores the keycode of the last key pressed; it is used to implement software auto-repeat.
rep is for the current values of auto-repeat parameters (delay, rate).
absinfo is an array of &struct input_absinfo elements, holding information about absolute axes (current value, min, max, flat, fuzz, resolution, and so on).
You should use the input_set_abs_params() function to set those values:
```c
void input_set_abs_params(struct input_dev *dev, unsigned int axis,
int min, int max, int fuzz, int flat)
```
min and max specify the lower and upper bound values. fuzz indicates the expected noise on the specified channel of the specified input device. The following is an example in which we only set each channel's bounds:
```c
#define ABSMAX_ACC_VAL 0x01FF
#define ABSMIN_ACC_VAL -(ABSMAX_ACC_VAL)
```
[...]
```c
set_bit(EV_ABS, idev->evbit);
input_set_abs_params(idev, ABS_X, ABSMIN_ACC_VAL,
```
ABSMAX_ACC_VAL, 0, 0);
```c
input_set_abs_params(idev, ABS_Y, ABSMIN_ACC_VAL,
```
ABSMAX_ACC_VAL, 0, 0);
```c
input_set_abs_params(idev, ABS_Z, ABSMIN_ACC_VAL,
```
ABSMAX_ACC_VAL, 0, 0);
key reflects the current state of the device's keys/buttons.
open is a method called when the very first user calls input_open_device().
Use this method to prepare the device, including the interrupt request, the polling thread start, and so on.
close is called when the very last user calls input_close_device(). Here, you can stop polling (which consumes a lot of resources).
```c
users stores the number of users (input handlers) that opened this device. It is used by input_open_device() and input_close_device(), to make sure that dev->open() is only called when the first user opens the device, and dev->close() is called when the very last user closes the device.
```
dev is the struct device associated with the device (for device model).
num_vals is the number of values queued in the current frame.
max_vals is the maximum number of values queued in a frame.
Vals is the array of values queued in the current frame.
devres_managed indicates that devices are managed with the devres framework and need not be explicitly unregistered or freed.
## Allocating and registering an input device
Prior to registering and sending the event with an input device, it should be allocated with the input_allocate_device() function. In order to free the previously allocated memory for a non-registered input device, the input_free_device() function should be used. If the device has already been registered, input_unregister_device() should be used instead. Like every function where memory allocation is needed, we can use a resource-managed version of functions, as follows:
```c
struct input_dev *input_allocate_device(void)
struct input_dev *devm_input_allocate_device(struct device *dev)
void input_free_device(struct input_dev *dev)
static void devm_input_device_unregister(struct device *dev,
void *res)
int input_register_device(struct input_dev *dev)
void input_unregister_device(struct input_dev *dev)
```
Device allocation may sleep, and therefore, it must not be called in the atomic context or with a spinlock held.
The following is an excerpt of the probe function of an input device sitting on the I2C bus:
```c
struct input_dev *idev;
int error;
```
idev = input_allocate_device();
```c
if (!idev)
return -ENOMEM;
idev->name = BMA150_DRIVER;
idev->phys = BMA150_DRIVER "/input0";
idev->id.bustype = BUS_I2C;
idev->dev.parent = &client->dev;
set_bit(EV_ABS, idev->evbit);
input_set_abs_params(idev, ABS_X, ABSMIN_ACC_VAL,
```
ABSMAX_ACC_VAL, 0, 0);
```c
input_set_abs_params(idev, ABS_Y, ABSMIN_ACC_VAL,
```
ABSMAX_ACC_VAL, 0, 0);
```c
input_set_abs_params(idev, ABS_Z, ABSMIN_ACC_VAL,
```
ABSMAX_ACC_VAL, 0, 0);
error = input_register_device(idev);
```c
if (error) {
input_free_device(idev);
return error;
}
error = request_threaded_irq(client->irq,
```
NULL, my_irq_thread,
```c
IRQF_TRIGGER_RISING | IRQF_ONESHOT,
```
BMA150_DRIVER, NULL);
```c
if (error) {
dev_err(&client->dev, "irq request failed %d, error %d\n",
client->irq, error);
input_unregister_device(bma150->input);
```
goto err_free_mem;
```c
}
```
## The polled input device sub-class
A polled input device is a special type of input device, which relies on polling to sense device state changes whereas the generic input device type relies on the IRQ to sense changes and send events to the input core.
A polled input device is described as an instance of the struct input_polled_dev structure in the kernel, and is a wrapper around the generic struct input_dev structure:
```c
struct input_polled_dev {
void *private;
void (*open)(struct input_polled_dev *dev);
void (*close)(struct input_polled_dev *dev);
void (*poll)(struct input_polled_dev *dev);
unsigned int poll_interval; /* msec */
unsigned int poll_interval_max; /* msec */
unsigned int poll_interval_min; /* msec */
struct input_dev *input;
```
bool devres_managed;
```c
};
```
The following are the meanings of the elements in this structure:
private is the driver's private data.
open is an optional method that prepares a device for polling (enabling the device, and maybe flushing the device state).
close is an optional method that is called when the device is no longer being polled. It is used to put devices into low power mode.
poll is a mandatory method called whenever the device needs to be polled. It is called at the frequency of poll_interval.
poll_interval is the frequency at which the poll() method should be called.
It defaults to 500 msec, unless overridden when registering the device.
poll_interval_max specifies the upper bound for the poll interval. It defaults to the initial value of poll_interval.
poll_interval_min specifies the lower bound for the poll interval. It defaults to 0.
input is the input device around which the polled device is built. It must be properly initialized by the driver (ID, name, and bits). A polled input device provides an interface to use polling instead of IRQ, to sense device state changes.
Allocating/freeing the struct input_polled_dev structure is done by using input_allocate_polled_device() and input_free_polled_device(). You should take care of initializing the mandatory fields of the struct input_dev embedded in it.
The polling interval should be set, too; otherwise, it defaults to 500 msec. You can use resource manage version too. Both prototypes are as follows:
```c
struct input_polled_dev *devm_input_allocate_polled_device(struct device *dev)
struct input_polled_dev *input_allocate_polled_device(void)
void input_free_polled_device(struct input_polled_dev *dev)
For resource managed devices, the input_dev->devres_managed field will be set to true by the input core.
```
After allocation and proper field initialization, the polled input device can be registered by using input_register_polled_device(), which returns 0 on success. The reverse operation (unregister) is done with the input_unregister_polled_device() function:
```c
int input_register_polled_device(struct input_polled_dev *dev)
void input_unregister_polled_device(struct input_polled_dev *dev)
```
A typical example of the probe() function for such a device is as follows:
```c
static int button_probe(struct platform_device *pdev)
{
struct my_struct *ms;
struct input_dev *input_dev;
int retval;
ms = devm_kzalloc(&pdev->dev, sizeof(*ms), GFP_KERNEL);
if (!ms)
return -ENOMEM;
ms->poll_dev = input_allocate_polled_device();
if (!ms->poll_dev){
kfree(ms);
return -ENOMEM;
}
```
/* This gpio is not mapped to IRQ */
```c
ms->reset_btn_desc = gpiod_get(dev, "reset", GPIOD_IN);
ms->poll_dev->private = ms ;
ms->poll_dev->poll = my_btn_poll;
ms->poll_dev->poll_interval = 200; /* Poll every 200ms */
ms->poll_dev->open = my_btn_open; /* consist */
input_dev = ms->poll_dev->input;
input_dev->name = "System Reset Btn";
```
/* The gpio belong to an expander sitting on I2C */
```c
input_dev->id.bustype = BUS_I2C;
input_dev->dev.parent = &pdev->dev;
```
/* Declare the events generated by this driver */
```c
set_bit(EV_KEY, input_dev->evbit);
set_bit(BTN_0, input_dev->keybit); /* buttons */
retval = input_register_polled_device(mcp->poll_dev);
if (retval) {
dev_err(&pdev->dev, "Failed to register input device\n");
input_free_polled_device(ms->poll_dev);
kfree(ms);
}
return retval;
}
```
The following is how our struct my_struct structure looks:
```c
struct my_struct {
struct gpio_desc *reset_btn_desc;
struct input_polled_dev *poll_dev;
}
```
The following is how the open function looks:
```c
static void my_btn_open(struct input_polled_dev *poll_dev)
{
struct my_strut *ms = poll_dev->private;
dev_dbg(&ms->poll_dev->input->dev, "reset open()\n");
}
```
The open method is used to prepare the resources needed by the device. We do not really need this method for this example.
## Generating and reporting an input event
Device allocation and registration are essential, but they are not the main goals of an input device driver, which is designed to report events to the input core. Depending on the type of event your device supports, the kernel provides the appropriate APIs to report them to the core.
Given an EV_XXX capable device, the corresponding report function would be input_report_xxx(). The following table shows mappings between the most important event types and their report functions:
Event type Report function Code example
```c
EV_KEY input_report_key()
input_report_key(poll_dev->input, BTN_0,
gpiod_get_value(ms-> reset_btn_desc) & 1);
EV_REL input_report_rel()
input_report_rel(nunchuk->input, REL_X,
(nunchuk->report.joy_x - 128)/10);
EV_ABS input_report_abs()
input_report_abs(bma150->input, ABS_X,
```
x_value);
```c
input_report_abs(bma150->input, ABS_Y,
```
y_value);
```c
input_report_abs(bma150->input, ABS_Z,
```
z_value);
Their respective prototypes are as follows:
```c
void input_report_abs(struct input_dev *dev,
unsigned int code, int value)
void input_report_key(struct input_dev *dev,
unsigned int code, int value)
void input_report_rel(struct input_dev *dev,
unsigned int code, int value)
```
The list of available report functions can be found in include/linux/input.h, in the kernel source file. They all have the same skeleton, as follows:
dev is the input device responsible for the event.
code represents the event code; for example, REL_X or KEY_BACKSPACE. The complete list is in include/linux/input-event-codes.h.
value is the value the event carries. For the EV_REL event type, it carries the relative change. For a EV_ABS (joysticks, and so on) event type, it contains an absolute new value. For the EV_KEY event type, it should be set to 0 for key release, 1 for key press, and 2 for auto-repeat.
After all changes have been reported, the driver should call input_sync() on the input device, in order to indicate that this event is complete. The input subsystem will collect these into a single packet and send it through /dev/input/event<X>, which is the character device representing our struct input_dev on the system and where <X> is the interface number assigned to the driver by the input core:
```c
void input_sync(struct input_dev *dev)
```
Let's look at an example, which is an excerpt from the bma150 digital acceleration sensor drivers in drivers/input/misc/bma150.c:
```c
static void threaded_report_xyz(struct bma150_data *bma150)
{
```
u8 data[BMA150_XYZ_DATA_SIZE];
s16 x, y, z;
s32 ret;
```c
ret = i2c_smbus_read_i2c_block_data(bma150->client,
```
BMA150_ACC_X_LSB_REG, BMA150_XYZ_DATA_SIZE, data);
```c
if (ret != BMA150_XYZ_DATA_SIZE)
```
return;
x = ((0xc0 & data[0]) >> 6) | (data[1] << 2);
y = ((0xc0 & data[2]) >> 6) | (data[3] << 2);
z = ((0xc0 & data[4]) >> 6) | (data[5] << 2);
/* sign extension */
x = (s16) (x << 6) >> 6;
y = (s16) (y << 6) >> 6;
z = (s16) (z << 6) >> 6;
```c
input_report_abs(bma150->input, ABS_X, x);
input_report_abs(bma150->input, ABS_Y, y);
input_report_abs(bma150->input, ABS_Z, z);
```
/* Indicate this event is complete */
```c
input_sync(bma150->input);
}
```
In the preceding example, input_sync() tells the core to consider the three reports as the same event. That makes sense since the position has three axes (X, Y, Z), and we do not want
X, Y, or Z to be reported separately. The best place to report the event is inside the poll function for a polled device, or inside the IRQ routine (the threaded part, or not) for an
IRQ-enabled device. If you perform some operations that may sleep, you should process your report inside of the threaded part of the IRQ handler:
```c
static void my_btn_poll(struct input_polled_dev *poll_dev)
{
struct my_struct *ms = poll_dev->private;
struct i2c_client *client = mcp->client;
input_report_key(poll_dev->input, BTN_0,
gpiod_get_value(ms->reset_btn_desc) & 1);
input_sync(poll_dev->input);
}
```
## The user space interface
Every registered input device is represented by a /dev/input/event<X> char device,
from which we can read the event from the user space. An application reading this file will receive event packets in the struct input_event format:
```c
struct input_event {
struct timeval time;
```
__u16 type;
__u16 code;
__s32 value;
```c
}
```
Let's look at the meaning of each element in the structure:
time is the timestamp. It returns the time at which the event happened.
type is the event type; for example, EV_KEY for a key press or release, EV_REL
for a relative moment, or EV_ABS for an absolute one. More types are defined in include/linux/input-event-codes.h.
code is the event code; for example, REL_X or KEY_BACKSPACE. Again, a complete list is in include/linux/input-event-codes.h.
value is the value that the event carries. For the EV_REL event type, it carries the relative change. For an EV_ABS (joysticks, and so on) event type, it contains the absolute new value. For the EV_KEY event type, it should be set to 0 for key release, 1 for key press, and 2 for auto-repeat.
A user space application can use blocking and non-blocking reads, and also poll() or select() system calls, in order to get notified of events after opening the device. The following is an example including the select() system call, with the complete source code provided in the book source repository:
```c
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <sys/select.h>
#define INPUT_DEVICE "/dev/input/event1"
int main(int argc, char **argv)
{
int fd;
struct input_event event;
```
ssize_t bytesRead;
```c
int ret;
```
fd_set readfds;
fd = open(INPUT_DEVICE, O_RDONLY);
/* Let's open our input device */
```c
if(fd < 0){
fprintf(stderr, "Error opening %s for reading", INPUT_DEVICE);
exit(EXIT_FAILURE);
}
while(1){
```
/* Wait on fd for input */
```c
FD_ZERO(&readfds);
FD_SET(fd, &readfds);
```
ret = select(fd + 1, &readfds, NULL, NULL, NULL);
```c
if (ret == -1) {
```
fprintf(stderr, "select call on %s: an error occurred",
INPUT_DEVICE);
break;
```c
}
```
else if (!ret) { /* If we have decided to use timeout */
```c
fprintf(stderr, "select on %s: TIMEOUT", INPUT_DEVICE);
```
break;
```c
}
```
/* File descriptor is now ready */
```c
if (FD_ISSET(fd, &readfds)) {
```
bytesRead = read(fd, &event,
sizeof(struct input_event));
```c
if(bytesRead == -1)
```
/* Process read input error*/
if(bytesRead != sizeof(struct input_event))
/* Read value is not an input even */
/*
* We could have done a switch/case if we had
* many codes to look for
*/
```c
if(event.code == BTN_0) {
```
/* it concerns our button */
```c
if(event.value == 0){
```
/* Process Release */
[...]
```c
}
else if(event.value == 1){
```
/* Process KeyPress */
[...]
```c
}
}
}
}
close(fd);
return EXIT_SUCCESS;
}
```
## Putting it all together
So far, we have described the structures used when writing drivers for input devices, and how they can be managed from the user space:
1. Allocate a new input device, according to its type (polled, or not), using input_allocate_polled_device() or input_allocate_device().
2. Fill in the mandatory fields (if necessary):
Specify the type of event the device supports by using the set_bit() helper macro on the input_dev.evbit field.
Depending on the event type, EV_REL, EV_ABS, EV_KEY, or other,
indicate the code this device can report, using either input_dev.relbit, input_dev.absbit, input_dev.keybit,
or other.
Specify input_dev.dev, in order to set up a proper device tree.
Fill abs_info if necessary.
For polled devices, indicate at which interval the poll() function should be called.
3. Write the open() function, if necessary, in which you should prepare and set up the resources used by the device. This function is called only once. In this function, set up GPIO, request an interrupt (if needed), and initialize the device.
4. Write your close() function, in which you will release and deallocate what you have done in the open() function. For example, free GPIO, IRQ, put device to power saving mode.
5. Pass either your open() or close() function (or both) to the input_dev.open and input_dev.close fields.
6. Register your device using input_register_polled_device() (if polled), or input_register_device() (if not).
7. In your IRQ function (threaded or not) or in your poll() function, gather and report events depending on their types, using either input_report_key(),
```c
input_report_rel(), input_report_abs(), or other, and then call input_sync() on the input device, to indicate the end of the frame (the report is complete).
```
The usual method is to use classic input devices if no IRQ is provided, or to fall back to a polled device, as follows:
```c
if(client->irq > 0){
```
/* Use generic input device */
```c
} else {
```
/* Use polled device */
```c
}
```
To see how to manage such devices from the user space, please refer to the example provided in the source of the book.
## Driver examples
You can summarize things in the two following drivers. The first one is a polled input device, based on a GPIO not mapped to IRQ. The polled input core will poll the GPIO to sense any changes. This driver is configured to send a 0 keycode. Each GPIO state corresponds to either a key press or a key release:
```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h> /* For DT*/
#include <linux/platform_device.h> /* For platform devices */
#include <linux/gpio/consumer.h> /* For GPIO Descriptor interface */
#include <linux/input.h>
#include <linux/input-polldev.h>
struct poll_btn_data {
struct gpio_desc *btn_gpiod;
struct input_polled_dev *poll_dev;
};
static void polled_btn_open(struct input_polled_dev *poll_dev)
{
/* struct poll_btn_data *priv = poll_dev->private; */
pr_info("polled device opened()\n");
}
static void polled_btn_close(struct input_polled_dev *poll_dev)
{
/* struct poll_btn_data *priv = poll_dev->private; */
pr_info("polled device closed()\n");
}
static void polled_btn_poll(struct input_polled_dev *poll_dev)
{
struct poll_btn_data *priv = poll_dev->private;
input_report_key(poll_dev->input, BTN_0,
gpiod_get_value(priv->btn_gpiod) & 1);
input_sync(poll_dev->input);
}
static const struct of_device_id btn_dt_ids[] = {
{ .compatible = "packt,input-polled-button", },
{ /* sentinel */ }
};
static int polled_btn_probe(struct platform_device *pdev)
{
struct poll_btn_data *priv;
struct input_polled_dev *poll_dev;
struct input_dev *input_dev;
int ret;
priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
if (!priv)
return -ENOMEM;
```
poll_dev = input_allocate_polled_device();
```c
if (!poll_dev){
devm_kfree(&pdev->dev, priv);
return -ENOMEM;
}
```
/* We assume this GPIO is active high */
```c
priv->btn_gpiod = gpiod_get(&pdev->dev, "button", GPIOD_IN);
poll_dev->private = priv;
poll_dev->poll_interval = 200; /* Poll every 200ms */
poll_dev->poll = polled_btn_poll;
poll_dev->open = polled_btn_open;
poll_dev->close = polled_btn_close;
priv->poll_dev = poll_dev;
input_dev = poll_dev->input;
input_dev->name = "Packt input polled Btn";
input_dev->dev.parent = &pdev->dev;
```
/* Declare the events generated by this driver */
```c
set_bit(EV_KEY, input_dev->evbit);
set_bit(BTN_0, input_dev->keybit); /* buttons */
ret = input_register_polled_device(priv->poll_dev);
if (ret) {
pr_err("Failed to register input polled device\n");
input_free_polled_device(poll_dev);
devm_kfree(&pdev->dev, priv);
return ret;
}
platform_set_drvdata(pdev, priv);
return 0;
}
static int polled_btn_remove(struct platform_device *pdev)
{
struct poll_btn_data *priv = platform_get_drvdata(pdev);
input_unregister_polled_device(priv->poll_dev);
input_free_polled_device(priv->poll_dev);
gpiod_put(priv->btn_gpiod);
return 0;
}
static struct platform_driver mypdrv = {
```
.probe = polled_btn_probe,
.remove = polled_btn_remove,
```c
.driver = {
```
.name = "input-polled-button",
.of_match_table = of_match_ptr(btn_dt_ids),
.owner = THIS_MODULE,
```c
},
};
module_platform_driver(mypdrv);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_DESCRIPTION("Polled input device");
```
The second driver sends events to the input core, according to the IRQ on which the button's GPIO is mapped. When using IRQ to sense key presses or releases, it is a good practice to trigger the interrupt on an edge change:
```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h> /* For DT*/
#include <linux/platform_device.h> /* For platform devices */
#include <linux/gpio/consumer.h> /* For GPIO Descriptor interface */
#include <linux/input.h>
#include <linux/interrupt.h>
struct btn_data {
struct gpio_desc *btn_gpiod;
struct input_dev *i_dev;
struct platform_device *pdev;
int irq;
};
static int btn_open(struct input_dev *i_dev)
{
pr_info("input device opened()\n");
return 0;
}
static void btn_close(struct input_dev *i_dev)
{
pr_info("input device closed()\n");
}
static irqreturn_t packt_btn_interrupt(int irq, void *dev_id)
{
struct btn_data *priv = dev_id;
input_report_key(priv->i_dev, BTN_0, gpiod_get_value(priv->btn_gpiod) &
```
1);
```c
input_sync(priv->i_dev);
return IRQ_HANDLED;
}
static const struct of_device_id btn_dt_ids[] = {
{ .compatible = "packt,input-button", },
{ /* sentinel */ }
};
static int btn_probe(struct platform_device *pdev)
{
struct btn_data *priv;
struct gpio_desc *gpiod;
struct input_dev *i_dev;
int ret;
priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
if (!priv)
return -ENOMEM;
```
i_dev = input_allocate_device();
```c
if (!i_dev)
return -ENOMEM;
i_dev->open = btn_open;
i_dev->close = btn_close;
i_dev->name = "Packt Btn";
i_dev->dev.parent = &pdev->dev;
priv->i_dev = i_dev;
priv->pdev = pdev;
```
/* Declare the events generated by this driver */
```c
set_bit(EV_KEY, i_dev->evbit);
set_bit(BTN_0, i_dev->keybit); /* buttons */
```
/* We assume this GPIO is active high */
```c
gpiod = gpiod_get(&pdev->dev, "button", GPIOD_IN);
if (IS_ERR(gpiod))
return -ENODEV;
priv->irq = gpiod_to_irq(priv->btn_gpiod);
priv->btn_gpiod = gpiod;
ret = input_register_device(priv->i_dev);
if (ret) {
pr_err("Failed to register input device\n");
```
goto err_input;
```c
}
ret = request_any_context_irq(priv->irq,
```
packt_btn_interrupt,
(IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING),
"packt-input-button", priv);
```c
if (ret < 0) {
dev_err(&pdev->dev,
```
"Unable to acquire interrupt for GPIO line\n");
goto err_btn;
```c
}
platform_set_drvdata(pdev, priv);
return 0;
```
err_btn:
```c
gpiod_put(priv->btn_gpiod);
```
err_input:
```c
printk("will call input_free_device\n");
input_free_device(i_dev);
printk("will call devm_kfree\n");
return ret;
}
static int btn_remove(struct platform_device *pdev)
{
struct btn_data *priv;
```
priv = platform_get_drvdata(pdev);
```c
input_unregister_device(priv->i_dev);
input_free_device(priv->i_dev);
free_irq(priv->irq, priv);
gpiod_put(priv->btn_gpiod);
return 0;
}
static struct platform_driver mypdrv = {
```
.probe = btn_probe,
.remove = btn_remove,
```c
.driver = {
```
.name = "input-button",
.of_match_table = of_match_ptr(btn_dt_ids),
.owner = THIS_MODULE,
```c
},
};
module_platform_driver(mypdrv);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_DESCRIPTION("Input device (IRQ based)");
```
For both examples, when a device matches the module, a node will be created in the /dev/input directory. The node corresponds to event0 in our example. You can use the udevadm tool to display information about the device:
```bash
# udevadm info /dev/input/event0
```
P: /devices/platform/input-button.0/input/input0/event0
N: input/event0
S: input/by-path/platform-input-button.0-event
E: DEVLINKS=/dev/input/by-path/platform-input-button.0-event
E: DEVNAME=/dev/input/event0
E: DEVPATH=/devices/platform/input-button.0/input/input0/event0
E: ID_INPUT=1
E: ID_PATH=platform-input-button.0
E: ID_PATH_TAG=platform-input-button_0
E: MAJOR=13
E: MINOR=64
E: SUBSYSTEM=input
E: USEC_INITIALIZED=74842430
The tool that actually allows us to print the event key to the screen is evtest, given the path of the input device:
```bash
# evtest /dev/input/event0
c
input device opened()
```
Input driver version is 1.0.1
Input device ID: bus 0x0 vendor 0x0 product 0x0 version 0x0
Input device name: "Packt Btn"
Supported events:
```c
Event type 0 (EV_SYN)
Event type 1 (EV_KEY)
Event code 256 (BTN_0)
```
Since the second module is based on IRQ, you can easily check whether the IRQ request succeeded, and how many times it has been fired:
```bash
$ cat /proc/interrupts | grep packt
```
160: 0 0 0 0 gpio-mxc 0 packt-input-button
Finally, you can successively push/release the button, to check whether the GPIO's state has changed:
```bash
$ cat /sys/kernel/debug/gpio | grep button gpio-193 (button-gpio ) in hi
$ cat /sys/kernel/debug/gpio | grep button gpio-193 (button-gpio ) in lo
```
## Summary
This chapter described the whole input framework and highlighted the differences between polled and interrupt-driven input devices. Having reached the end of this chapter, you have gained the knowledge necessary to write a driver for any input driver, whatever its type and whatever input event it supports. The user space interface was also discussed, and a sample was provided. The next chapter will discuss another important framework, the
RTC, which is a key element of time management in PCs and embedded devices