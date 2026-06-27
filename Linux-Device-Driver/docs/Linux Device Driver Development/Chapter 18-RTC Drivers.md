```bash
# Chapter 18 - RTC Drivers
```
Real Time Clocks (RTCs) are devices used to track the absolute time in nonvolatile memory, which can be internal to the processor or externally connected through the I2C or
SPI bus.
You can use an RTC to do the following:
Read and set the absolute clock, and generate interrupts during clock updates
Generate periodic interrupts
Set alarms
RTCs and the system clock have different purposes. The former is a hardware clock that maintains the absolute time and date in a nonvolatile manner, whereas the latter is a software clock maintained by the kernel and used to implement the gettimeofday(2) and time(2 system calls, set the timestamps on files, and so on. The system clock reports seconds and microseconds from a starting point, defined to be the POSIX epoch:
1970-01-01 00:00:00 +0000 (UTC).
In this chapter, we will cover the following topics:
Introducing the RTC framework API
Describing driver's architecture, along with a dummy driver example
Dealing with alarms
Managing RTC devices from the user space, either through the sysfs interface or by using the hwclock tool
## RTC framework data structures
There are three main data structures used by the RTC framework in Linux systems. They are the strcut rtc_time, struct rtc_device, and struct rtc_class_ops structures. The first is an opaque structure that represents a given date and time; the second structure represents the physical RTC device; the last one represents a set of operations exposed by the driver, and is used by the RTC core to read/update a device's date/time/alarm.
The only header needed to pull RTC functions from within your driver is:
```c
#include <linux/rtc.h>
```
The same file contains all of the three structures enumerated in the preceding section:
```c
struct rtc_time {
int tm_sec; /* seconds after the minute */
int tm_min; /* minutes after the hour - [0, 59] */
int tm_hour; /* hours since midnight - [0, 23] */
int tm_mday; /* day of the month - [1, 31] */
int tm_mon; /* months since January - [0, 11] */
int tm_year; /* years since 1900 */
int tm_wday; /* days since Sunday - [0, 6] */
int tm_yday; /* days since January 1 - [0, 365] */
int tm_isdst; /* Daylight saving time flag */
};
```
This structure is similar to the struct tm in <time.h>, used to pass time. The next structure is struct rtc_device, which represents the chip in the kernel:
```c
struct rtc_device {
struct device dev;
struct module *owner;
int id;
```
char name[RTC_DEVICE_NAME_SIZE];
const struct rtc_class_ops *ops;
```c
struct mutex ops_lock;
struct cdev char_dev;
unsigned long flags;
unsigned long irq_data;
```
spinlock_t irq_lock;
wait_queue_head_t irq_queue;
```c
struct rtc_task *irq_task;
```
spinlock_t irq_task_lock;
```c
int irq_freq;
int max_user_freq;
struct work_struct irqwork;
};
```
The following are the meanings of the elements of the structure:
dev: This is the device structure.
owner: This is the module that owns the RTC device. Using THIS_MODULE will be enough.
id: This is the global index given to the RTC device by the /dev/rtc<id> kernel.
name: This is the name given to the RTC device.
ops: This is a set of operations (such as read/set time/alarm) exposed by this RTC
device to be managed by the core or from the user space.
ops_lock: This is a mutex used internally by the kernel, to protect ops function calls.
cdev: This is the char device associated to this RTC, /dev/rtc<id>.
The next important structure is struct rtc_class_ops, which is a set of functions used as a callback to perform standard and limited operations on the RTC device. It is the communication interface between top-layer and bottom-layer RTC drivers:
```c
struct rtc_class_ops {
int (*open)(struct device *);
void (*release)(struct device *);
int (*ioctl)(struct device *, unsigned int, unsigned long);
int (*read_time)(struct device *, struct rtc_time *);
int (*set_time)(struct device *, struct rtc_time *);
int (*read_alarm)(struct device *, struct rtc_wkalrm *);
int (*set_alarm)(struct device *, struct rtc_wkalrm *);
int (*read_callback)(struct device *, int data);
int (*alarm_irq_enable)(struct device *, unsigned int enabled);
};
```
All of the hooks in the preceding code are given a struct device structure as a parameter, which is the same as the one embedded in the struct rtc_device structure.
This means that from within these hooks, you can access the RTC device itself at any given time, using the to_rtc_device() macro, which is built on top of the container_of()
macro:
```c
#define to_rtc_device(d) container_of(d, struct rtc_device, dev)
```
The open(), release(), and read_callback() hooks are internally called by the kernel when the open(), close(), or read() functions are called on the device from the user space.
read_time() is a driver function that reads the time from the device and fills the struct rtc_time output argument. This function should return 0 up on success, or else the negative error code.
set_time() is a driver function that updates the device's time according to the struct rtc_time structure given as the input parameter. The return parameter's remarks are the same as the read_time function.
If your device supports an alarm feature, read_alarm() and set_alarm() should be provided by the driver, to read/set the alarm on the device. struct rtc_wkalrm will be described later in the chapter. alarm_irq_enable() should be provided, too, to enable the alarm.
## RTC API
An RTC device is represented in the kernel as an instance of the struct rtc_device structure. Unlike other kernel framework device registrations (where the device is given as a parameter to the registering function), the RTC device is built by the core, and is registered before the rtc_device structure gets returned to the driver. The device is built and registered with the kernel using the rtc_device_register() function:
```c
struct rtc_device *rtc_device_register(const char *name,
struct device *dev,
```
const struct rtc_class_ops *ops,
```c
struct module *owner)
```
The meanings of the parameters of the are as follows:
name: This is your RTC device name. It could be the chip's name; for example,
ds1343.
dev: This is the parent device, used for device model purposes. For chips sitting on I2C or SPI buses, for example, dev could be set with spi_device.dev or i2c_client.dev.
ops: This is your RTC ops, filled according to the features the RTC has or those that your driver can support.
owner: This is the module to which the RTC device belongs. In most cases, THIS_MODULE is enough.
The registration should be performed in the probe function, and obviously, you can use the resource-managed version of this function:
```c
struct rtc_device *devm_rtc_device_register(struct device *dev,
```
const char *name,
const struct rtc_class_ops *ops,
```c
struct module *owner)
```
Both functions return a pointer on a struct rtc_device structure built by the kernel up on success, or a pointer error on which you should use the IS_ERR and PTR_ERR macros.
The associated reverse operations are rtc_device_unregister() and devm_
```c
rtc_device_unregister():
void rtc_device_unregister(struct rtc_device *rtc)
void devm_rtc_device_unregister(struct device *dev,
struct rtc_device *rtc)
```
## Reading and setting time
The driver is responsible for providing functions to read and set the device's time. These are the least an RTC driver can provide. When it comes to reading, the read callback function is given a pointer to an allocated/zeroed struct rtc_time structure, which the driver has to fill. Therefore, RTCs almost always store/restitute time in Binary Coded Decimal (BCD),
where each quartet (series of four bits) represents a number between 0 and 9 (rather than between 0 and 15). The kernel provides two macros, bcd2bin() and bin2bcd(), to convert from BCD encoding to decimal or from decimal to BCD, respectively. The next things that you should pay attention to are rtc_time fields, which have some boundary requirements, and where some translation must be done. Data is read in BCD from the device, and should be converted using bcd2bin().
Since the struct rtc_time structure is complex, the kernel provides the rtc_valid_tm() helper in order to validate a given rtc_time structure; it returns 0
on success, meaning that the structure represents a valid date/time:
```c
int rtc_valid_tm(struct rtc_time *tm);
```
The following example shows an RTC read operation callback:
```c
static int foo_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
struct foo_regs regs;
int error;
```
error = foo_device_read(dev, &regs, 0, sizeof(regs));
```c
if (error)
return error;
tm->tm_sec = bcd2bin(regs.seconds);
tm->tm_min = bcd2bin(regs.minutes);
tm->tm_hour = bcd2bin(regs.cent_hours);
tm->tm_mday = bcd2bin(regs.date);
```
/*
* This device returns weekdays from 1 to 7
* But rtc_time.wday expect days from 0 to 6.
* So we need to subtract 1 to the value returned by the chip
*/
```c
tm->tm_wday = bcd2bin(regs.day) - 1;
```
/*
* This device returns months from 1 to 12
* But rtc_time.tm_month expect a months 0 to 11.
* So we need to subtract 1 to the value returned by the chip
*/
```c
tm->tm_mon = bcd2bin(regs.month) - 1;
```
/*
* This device's Epoch is 2000.
* But rtc_time.tm_year expect years from Epoch 1900.
* So we need to add 100 to the value returned by the chip
*/
```c
tm->tm_year = bcd2bin(regs.years) + 100;
return rtc_valid_tm(tm);
}
```
The following header is necessary prior to using BCD conversion functions:
```c
#include <linux/bcd.h>
```
When it comes to the set_time function, a pointer to a struct rtc_time is given as an input parameter. This parameter is already filled with values to be stored in the RTC chip.
Unfortunately, these are decimal encoded, and should be converted into BCD prior to being sent to the chip. bin2bcd does the conversion. The same attention should be paid to some fields of the struct rtc_time structure. The following is pseudo code describing a generic set_time function:
```c
static int foo_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
regs.seconds = bin2bcd(tm->tm_sec);
regs.minutes = bin2bcd(tm->tm_min);
regs.cent_hours = bin2bcd(tm->tm_hour);
```
/*
* This device expects week days from 1 to 7
* But rtc_time.wday contains week days from 0 to 6.
* So we need to add 1 to the value given by rtc_time.wday
*/
```c
regs.day = bin2bcd(tm->tm_wday + 1);
regs.date = bin2bcd(tm->tm_mday);
```
/*
* This device expects months from 1 to 12
* But rtc_time.tm_mon contains months from 0 to 11.
* So we need to add 1 to the value given by rtc_time.tm_mon
*/
```c
regs.month = bin2bcd(tm->tm_mon + 1);
```
/*
* This device expects year since Epoch 2000
* But rtc_time.tm_year contains year since Epoch 1900.
* We can just extract the year of the century with the
* rest of the division by 100.
*/
regs.cent_hours |= BQ32K_CENT;
```c
regs.years = bin2bcd(tm->tm_year % 100);
return write_into_device(dev, &regs, 0, sizeof(regs));
}
```
RTC's epoch differs from the POSIX epoch, which is only used for the system clock. If the year (according to RTC's epoch and the year register)
is less than 1970, it is assumed to be 100 years later; that is, between 2000
and 2069.
## Driver example
You can summarize the preceding concepts in a simple and fake driver, which simply registers an RTC device on the system:
```c
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/time.h>
#include <linux/err.h>
#include <linux/rtc.h>
#include <linux/of.h>
static int fake_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
```
/*
* One can update "tm" with fake values and then call
*/
```c
return rtc_valid_tm(tm);
}
static int fake_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
return 0;
}
static const struct rtc_class_ops fake_rtc_ops = {
```
.read_time = fake_rtc_read_time,
.set_time = fake_rtc_set_time
```c
};
static const struct of_device_id rtc_dt_ids[] = {
{ .compatible = "packt,rtc-fake", },
{ /* sentinel */ }
};
static int fake_rtc_probe(struct platform_device *pdev)
{
struct rtc_device *rtc;
rtc = rtc_device_register(pdev->name, &pdev->dev,
```
&fake_rtc_ops, THIS_MODULE);
```c
if (IS_ERR(rtc))
return PTR_ERR(rtc);
platform_set_drvdata(pdev, rtc);
pr_info("Fake RTC module loaded\n");
return 0;
}
static int fake_rtc_remove(struct platform_device *pdev)
{
rtc_device_unregister(platform_get_drvdata(pdev));
return 0;
}
static struct platform_driver fake_rtc_drv = {
```
.probe = fake_rtc_probe,
.remove = fake_rtc_remove,
```c
.driver = {
```
.name = KBUILD_MODNAME,
.owner = THIS_MODULE,
.of_match_table = of_match_ptr(rtc_dt_ids),
```c
},
};
module_platform_driver(fake_rtc_drv);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_DESCRIPTION("Fake RTC driver description");
```
## Playing with alarms
RTC alarms are programmable events, to be triggered by the device at a given time. An
RTC alarm is represented as an instance of the struct rtc_wkalrm structure:
```c
struct rtc_wkalrm {
unsigned char enabled; /* 0 = alarm disabled, 1 = enabled */
unsigned char pending; /* 0 = alarm not pending, 1 = pending */
struct rtc_time time; /* time the alarm is set to */
};
```
The driver should provide set_alarm() and read_alarm() operations, to set and read the time at which the alarm should occur, as well as alarm_irq_enable(), which is a function used to enable/disable the alarm. When the set_alarm() function is invoked, it is given as an input parameter, a pointer to a struct rtc_wkalrm whose .time field contains the time the alarm must be set to. It is up to the driver to extract each value in a correct manner (using bin2dcb(), if necessary) and write it into the device in appropriate registers. rtc_wkalrm.enabled indicates whether the alarm should be enabled right after it has been set. If true, the driver must enable the alarm in the chip. The same is true for read_alarm(), which is given a pointer to struct rtc_wkalrm, but this time, as an output parameter. The driver has to fill the structure with data read from the device.
The {read | set}_alarm() and {read | set}_time() functions behave the same way, except that each pair of functions reads/stores data from/into different sets of registers in the device.
Prior to reporting an alarm event to the system, it is mandatory to connect the RTC chip to an IRQ line of the SoC. It relies on the INT line of the RTC driven low when the alarm occurs. Depending on the manufacturer, the line remains low until a status register gets read or a special bit gets cleared:
At this point, we can use a generic IRQ API, such as request_threaded_irq(), in order to register the alarm IRQ's handler. From within the IRQ handler, it is important to inform the kernel about the RTC IRQ event, using the rtc_update_irq() function:
```c
void rtc_update_irq(struct rtc_device *rtc,
unsigned long num, unsigned long events)
```
rtc: This is the RTC device that raised the IRQ
num: This shows how many IRQs are being reported (usually one)
events: This is a mask of RTC_IRQF, with one or more of RTC_PF, RTC_AF,
and RTC_UF
/* RTC interrupt flags */
```c
#define RTC_IRQF 0x80 /* Any of the following is active */
#define RTC_PF 0x40 /* Periodic interrupt */
#define RTC_AF 0x20 /* Alarm interrupt */
#define RTC_UF 0x10 /* Update interrupt for 1Hz RTC */
```
That function can be called from any context, atomic or not. The IRQ handler might look as follows:
```c
static irqreturn_t foo_rtc_alarm_irq(int irq, void *data)
{
struct foo_rtc_struct * foo_device = data;
dev_info(foo_device->dev, "%s:irq(%d)\n", __func__, irq);
rtc_update_irq(foo_device->rtc_dev, 1, RTC_IRQF | RTC_AF);
return IRQ_HANDLED;
}
```
Keep in mind that RTC devices that have the alarm feature can be used as a wake-up source. That said, the system can be woken up from suspend mode whenever the alarm triggers. This feature relies on the interrupt raised by the RTC device. You declare a device as being the wake-up source by using the device_init_wakeup() function. The IRQ that actually wakes the system up must be registered with the power management core, too, by using the dev_pm_set_wake_irq() function:
```c
int device_init_wakeup(struct device *dev, bool enable)
int dev_pm_set_wake_irq(struct device *dev, int irq)
```
We will not discuss power management in detail in this book. We'd just like to give you an overview of how RTC devices can improve your system. The drivers/rtc/rtcds1343.c driver can help to implement such functions. Let's put everything together by writing a fake probe function for an SPI foo RTC device:
```c
static const struct rtc_class_ops foo_rtc_ops = {
```
.read_time = foo_rtc_read_time,
.set_time = foo_rtc_set_time,
.read_alarm = foo_rtc_read_alarm,
.set_alarm = foo_rtc_set_alarm,
.alarm_irq_enable = foo_rtc_alarm_irq_enable,
.ioctl = foo_rtc_ioctl,
```c
};
static int foo_spi_probe(struct spi_device *spi)
{
int ret;
```
/* initialize and configure the RTC chip */
[...]
```c
foo_rtc->rtc_dev =
devm_rtc_device_register(&spi->dev, "foo-rtc",
```
&foo_rtc_ops, THIS_MODULE);
```c
if (IS_ERR(foo_rtc->rtc_dev)) {
dev_err(&spi->dev, "unable to register foo rtc\n");
return PTR_ERR(priv->rtc);
}
foo_rtc->irq = spi->irq;
if (foo_rtc->irq >= 0) {
ret = devm_request_threaded_irq(&spi->dev, spi->irq,
```
NULL, foo_rtc_alarm_irq,
```c
IRQF_ONESHOT, "foo-rtc", priv);
if (ret) {
foo_rtc->irq = -1;
dev_err(&spi->dev,
```
"unable to request irq for rtc foo-rtc\n");
```c
} else {
device_init_wakeup(&spi->dev, true);
dev_pm_set_wake_irq(&spi->dev, spi->irq);
}
}
return 0;
}
```
## RTCs and user space
On Linux systems, there are two kernel options that you need to consider in order to properly manage RTCs from the user space. These are CONFIG_RTC_HCTOSYS and
CONFIG_RTC_HCTOSYS_DEVICE.
CONFIG_RTC_HCTOSYS includes the drivers/rtc/hctosys.c code file in the kernel build process, which sets the system time from the RTC on startup and resume. Once this option is enabled, the system time will be set by using the value read from the specified RTC
device. RTC devices should be specified in CONFIG_RTC_HCTOSYS_DEVICE:
CONFIG_RTC_HCTOSYS=y
CONFIG_RTC_HCTOSYS_DEVICE="rtc0"
In the preceding example, we tell the kernel to set the system time from the RTC, and we specify that the RTC to use is rtc0.
## The sysfs interface
The kernel code responsible for instantiating RTC attributes in sysfs is defined in drivers/rtc/rtc-sysfs.c, in the kernel source tree. Once registered, an RTC device will create a rtc<id> directory under /sys/class/rtc. That directory contains a set of readonly attributes, the most important of which are:
date: This file prints the current date of the RTC interface:
```bash
$ cat /sys/class/rtc/rtc0/date
```
2017-08-28
time: This prints the current time of the RTC:
```bash
$ cat /sys/class/rtc/rtc0/time
```
14:54:20
hctosys: This attribute indicates whether the RTC device is the one specified in
CONFIG_RTC_HCTOSYS_DEVICE, meaning that this RTC is used to set the system time on startup and resume. Read 1 as true, and 0 as false:
```bash
$ cat /sys/class/rtc/rtc0/hctosys
```
1
dev: This attribute shows the device's major and minor. Read as major:minor:
```bash
$ cat /sys/class/rtc/rtc0/dev
```
251:0
since_epoch: This attribute will print the number of seconds elapsed since the
UNIX epoch (since January 1st, 1970):
```bash
$ cat /sys/class/rtc/rtc0/since_epoch
```
1503931738
## The hwclock utility
The hardware clock (hwclock) is a tool used to access RTC devices. The man hwclock command will probably be much more meaningful than everything else discussed in this section. That said, let's write some commands to set the hwclock RTC from the system clock:
```bash
$ sudo ntpd -q # make sure system clock is set from network time
$ sudo hwclock --systohc # set rtc from the system clock
$ sudo hwclock --show # check rtc was set
```
Sat May 17 17:36:50 2017 -0.671045 seconds
The preceding example assumes that the host has a network connection on which it can access an NTP server. It is also possible to set the system time manually:
```bash
$ sudo date -s '2017-08-28 17:14:00' '+%s' #set system clock manually
$ sudo hwclock --systohc #synchronize rtc chip on system time
```
If not given as an argument, hwclock assumes that the RTC device file is /dev/rtc, which is actually a symbolic link to the real RTC device:
```bash
$ ls -l /dev/rtc lrwxrwxrwx 1 root root 4 août 27 17:50 /dev/rtc -> rtc0
```
## Summary
This chapter introduced you to the RTC framework and its API. The RTC framework's reduced set of functions and data structures make it the most lightweight framework, easy to master. Using the skills described in this chapter, you will be able to develop a driver for most existing RTC chips. . You will also be able to handle such devices from the user space,
easily setting up the date and time, as well as alarms. The next chapter has nothing in common with this one, but is a must-read for embedded engineers