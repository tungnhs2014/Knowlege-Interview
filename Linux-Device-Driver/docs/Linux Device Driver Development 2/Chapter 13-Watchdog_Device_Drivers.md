```bash
# Chapter 13 - Watchdog Device Drivers
```
A watchdog is a hardware (sometimes emulated by software) device intended to ensure the availability of a given system. It helps make sure that the system always reboots upon a critical hang, thus allowing to monitor the "normal" behavior of the system.
Whether it is hardware-based or emulated by software, the watchdog is, most of the time,
nothing but a timer initialized with a reasonable timeout that should be periodically refreshed by software running on the monitored system. If for any reason the software stops/fails at refreshing the timer (and has not explicitly shut it down) before it expires
(it runs to timeout), this will trigger a (hardware) reset of the whole system (the computer,
in our case). Such a mechanism can even help with recovering from a kernel panic. By the end of this chapter, you will be able to do the following:
• Read/understand an existing watchdog kernel driver and use what it exposes in user space.
• Write new watchdog device drivers.
• Master some not-so-well-known concepts, such as watchdog governor and pretimeout.
564 Watchdog Device Drivers
In this chapter, we will also address the concepts behind the Linux kernel watchdog subsystem with the following topics:
• Watchdog data structures and APIs
• The watchdog user space interface
## Technical requirements
Before we start walking through this chapter, the following elements are required:
• C programming skills
• Basic electronics knowledge
• Linux kernel v4.19.X sources, available at https://git.kernel.org/pub/
scm/linux/kernel/git/stable/linux.git/refs/tags
## Watchdog data structures and APIs
In this section, we will walk through the watchdog framework and learn how it works under the hood. The watchdog subsystem has a few data structures. The main one is struct watchdog_device, which is the Linux kernel representation of a watchdog device, containing all the information about it. It is defined in include/linux/
watchdog.h, as follows:
```c
struct watchdog_device {
int id;
struct device *parent;
const struct watchdog_info *info;
const struct watchdog_ops *ops;
const struct watchdog_governor *gov;
unsigned int bootstatus;
unsigned int timeout;
unsigned int pretimeout;
unsigned int min_timeout;
struct watchdog_core_data *wd_data;
unsigned long status;
```
[...]
```c
};
```
Watchdog data structures and APIs 565
The following are descriptions of the fields in this data structure:
• id: The watchdog's ID allocated by the kernel during the device registration.
• parent: Represents the parent for this device.
• info: This struct watchdog_info structure pointer provides some additional information about the watchdog timer itself. This is the structure that is returned to the user when calling the WDIOC_GETSUPPORT ioctl on the watchdog char device in order to retrieve its capabilities. We will introduce this structure later in detail.
• ops: A pointer to the list of watchdog operations. Once again, we will introduce this data structure later.
• gov: A pointer to the watchdog pretimeout governor. A governor is nothing but a policy manager that reacts according to certain events or system parameters.
• bootstatus: The status of the watchdog device at boot. This is a bitmask of reasons that triggered the system reset. Possible values will be enumerated later when describing the struct watchdog_info structure.
• timeout: This is the watchdog device's timeout value in seconds.
• pretimeout: The concept of pretimeout can be explained as an event that occurs sometime before the real timeout occurs, so if the system is in an unhealthy state,
it triggers an interrupt before the real timeout reset. These interrupts are usually non-maskable ones (NMI, which stands for Non-Maskable Interrupt), and this can be used to secure important data and shut down specific applications or panic the system (which allows us to gather useful information prior to the reset, instead of a blind and sudden reboot).
In this context, the pretimeout field is actually the time interval (the number of seconds) before triggering the real timeout's interrupt. This is not the number of seconds until the pretimeout. As an example, if you set the timeout to 60 seconds and the pretimeout to 10, you'll have the pretimeout event triggered in 50 seconds.
Setting a pretimeout to 0 disables it.
• min_timeout and max_timeout are, respectively, the watchdog device's minimum and maximum timeout values (in seconds). These are actually the lower and upper bounds for a valid timeout range. If the values are 0, then the framework will leave a check for the watchdog driver itself.
• wd_data: A pointer to the watchdog core internal data. This field must be accessed via the watchdog_set_drvdata() and watchdog_get_drvdata() helpers.
566 Watchdog Device Drivers
• status is a field that contains the device's internal status bits. Possible values are listed here:
--WDOG_ACTIVE: Tells whether the watchdog is running/active.
--WDOG_NO_WAY_OUT: Informs whether the nowayout feature is set. You can use watchdog_set_nowayout() to set the nowayout feature; its signature is void watchdog_set_nowayout(struct watchdog_device *wdd, bool nowayout).
--WDOG_STOP_ON_REBOOT: Should be stopped on reboot.
--WDOG_HW_RUNNING: Informs that the hardware watchdog is running. You can use the watchdog_hw_running() helper to check whether this flag is set or not. However, you should set this flag on the success path of the watchdog's start function (or in the probe function if for any reason you start it there or you discover that the watchdog is already started). You can use the set_bit() helper for this.
--WDOG_STOP_ON_UNREGISTER: Specifies that the watchdog should be stopped on unregistration. You can use the watchdog_stop_on_unregister() helper to set this flag.
As we introduced it previously, let's delve in detail into the struct watchdog_info structure, defined in include/uapi/linux/watchdog.h, actually, because it is part of the user space API:
```c
struct watchdog_info {
```
u32 options;
u32 firmware_version;
u8 identity[32];
```c
};
```
This structure is also the one returned to the user space on the success path of the
```c
WDIOC_GETSUPPORT ioctl. In this structure, the fields have the following meanings:
```
• options represents the supported capabilities of the card/driver. It is a bitmask of the capabilities supported by the watchdog device/driver since some watchdog cards offer more than just a countdown. Some of these flags may also be set in the watchdog_device.bootstatus field in response to the GET_BOOT_STATUS
ioctl. These flags are listed as follows, with dual explanations given where necessary:
--WDIOF_SETTIMEOUT means the watchdog device can have its timeout set. If this flag is set, then a set_timeout callback has to be defined.
Watchdog data structures and APIs 567
--WDIOF_MAGICCLOSE means the driver supports the magic close char feature.
As the closing of the watchdog char device file does not stop the watchdog, this feature means writing a V character (also called a magic character or magic V)
sequence in this watchdog file will allow the next close to turn off the watchdog
(if nowayout is not set).
--WDIOF_POWERUNDER means the device can monitor/detect bad powers or power faults.. When set in watchdog_device.bootstatus, this flag means that it is the fact that the machine showed an under-voltage that triggered the reset.
--WDIOF_POWEROVER, on the other hand, means the device can monitor the operating voltage. When set in watchdog_device.bootstatus, it means the system reset may be due to an over-voltage status. Note that if one level is under and one over, both bits will be set.
--WDIOF_OVERHEAT means the watchdog device can monitor the chip/SoC
temperature. When set in watchdog_device.bootstatus, it means the reason for the last machine reboot via the watchdog was due to exceeding the thermal limit.
--WDIOF_FANFAULT informs us that this watchdog device can monitor the fan.
When set, it means a system fan monitored by the watchdog card has failed.
Some devices even have separate event inputs. If defined, electrical signals are present on these inputs, which also leads to a reset. This is the aim of
```c
WDIOF_EXTERN1 and WDIOF_EXTERN2. When set in watchdog_device.
```
bootstatus, it means the machine was last rebooted because of an external relay/
source 1 or 2.
--WDIOF_PRETIMEOUT means this watchdog device supports a pretimeout feature.
--WDIOF_KEEPALIVEPING means this driver supports the WDIOC_KEEPALIVE
ioctl (it can be pinged via an ioctl); otherwise, the ioctl will return -EOPNOTSUPP.
When set in watchdog_device.bootstatus, this flag means the watchdog saw a keep-alive ping since it was last queried.
--WDIOF_CARDRESET: This is a special flag that may appear in watchdog_
device.bootstatus only. It means the last reboot was caused by the watchdog itself (its timeout, actually).
• firmware_version is the firmware version of the card.
• identity should be a string describing the device.
568 Watchdog Device Drivers
The other data structure without which nothing is possible is struct watchdog_ops,
defined as follows:
```c
struct watchdog_ops { struct module *owner;
```
/* mandatory operations */
```c
int (*start)(struct watchdog_device *);
int (*stop)(struct watchdog_device *);
```
/* optional operations */
```c
int (*ping)(struct watchdog_device *);
unsigned int (*status)(struct watchdog_device *);
int (*set_timeout)(struct watchdog_device *, unsigned int);
int (*set_pretimeout)(struct watchdog_device *,
unsigned int);
unsigned int (*get_timeleft)(struct watchdog_device *);
int (*restart)(struct watchdog_device *,
unsigned long, void *);
```
long (*ioctl)(struct watchdog_device *, unsigned int,
```c
unsigned long);
};
```
The preceding structure contains the list of operations allowed on the watchdog device.
Each operation's meaning is presented in the following descriptions:
• start and stop: These are mandatory operations that, respectively, start and stop the watchdog.
• A ping callback is used to send a keep-alive ping to the watchdog. This method is optional. If not defined, then the watchdog will be restarted via the .start operation, as it would mean that the watchdog does not have its own ping method.
• status is an optional routine that returns the status of the watchdog device.
If defined, its return value will be sent in response to a WDIOC_GETBOOTSTATUS
ioctl.
• set_timeout is the callback to set the watchdog timeout value (in seconds).
If defined, you should also set the X option flag; otherwise, any attempt to set the timeout will result in an -EOPNOTSUPP error.
• set_pretimeout is the callback to set the pretimeout. If defined, you should also set the WDIOF_PRETIMEOUT option flag; otherwise, any attempt to set the pretimeout will result in an -EOPNOTSUPP error.
Watchdog data structures and APIs 569
• get_timeleft is an optional operation that returns the number of seconds left before a reset.
• restart: This is actually the routine to restart the machine (not the watchdog device). If set, you may want to call watchdog_set_restart_priority()
on the watchdog device in order to set the priority of this restart handler prior to registering the watchdog with the system.
• ioctl: You should not implement this callback unless you have to – for example,
if you need to handle extra/non-standard ioctl commands. If defined, this method will override the watchdog core default ioctl, unless it returns -ENOIOCTLCMD.
This structure contains the callback functions supported by the device according to its capabilities.
Now that we are familiar with the data structures, we can switch to watchdog APIs and particularly see how to register and unregister such a device with the system.
## Registering/unregistering a watchdog device
The watchdog framework provides two elementary functions to register/unregister watchdog devices with the system. These are watchdog_register_device() and watchdog_unregister_device(), and their respective prototypes are the following:
```c
int watchdog_register_device(struct watchdog_device *wdd)
void watchdog_unregister_device(struct watchdog_device *wdd)
```
The preceding registration method returns zero on a success path or a negative errno code on failure. On the other hand, watchdog_unregister_device() performs the reverse operation. In order to no longer bother with unregistration, you can use the managed version of this function, devm_watchdog_register_device, whose prototype is as follows:
```c
int devm_watchdog_register_device(struct device *dev,
struct watchdog_device *wdd)
```
The preceding managed version will automatically handle unregistration on driver detach.
570 Watchdog Device Drivers
```c
The registration method (whatever it is, managed or not) will check whether the wdd->ops->restart function is provided and will register this method as a restart handler. Thus, prior to registering the watchdog device with the system, the driver should set the restart priority using the watchdog_set_restart_priority()
```
helper, knowing that the priority value of the restart handler should follow the following guidelines:
• 0: This is the lowest priority, which means using the watchdog's restart function as a last resort; that is, when there is no other restart handler provided in the system.
• 128: This is the default priority, and means using this restart handler by default if no other handler is expected to be available and/or if a restart is sufficient to restart the entire system.
• 255: This is the highest priority, preempting all other handlers.
The device registration should be done only after you have dealt with all the elements we have discussed; that is, after providing the valid .info, .ops, and timeout-related fields of the watchdog device. Prior to all this, memory space should be allocated for the watchdog_device structure. Wrapping this structure in a bigger and per-driver data structure is good practice, as shown in the following example, which is an excerpt from drivers/watchdog/imx2_wdt.c:
[...]
```c
struct imx2_wdt_device {
struct clk *clk;
struct regmap *regmap;
struct watchdog_device wdog;
bool ext_reset;
};
```
You can see how the watchdog device data structure is embedded in a bigger structure,
```c
struct imx2_wdt_device. Now comes the probe method, which initializes everything and sets the watchdog device in the bigger structure:
static int init imx2_wdt_probe(struct platform_device *pdev)
{
struct imx2_wdt_device *wdev;
struct watchdog_device *wdog; int ret;
```
[...]
```c
wdev = devm_kzalloc(&pdev->dev, sizeof(*wdev), GFP_KERNEL);
Watchdog data structures and APIs 571 if (!wdev)
return -ENOMEM;
```
[...]
```c
Wdog = &wdev->wdog;
if (imx2_wdt_is_running(wdev)) {
imx2_wdt_set_timeout(wdog, wdog->timeout);
set_bit(WDOG_HW_RUNNING, &wdog->status);
}
```
ret = watchdog_register_device(wdog);
```c
if (ret) {
dev_err(&pdev->dev, "cannot register watchdog device\n");
```
[...]
```c
}
return 0;
}
static int exit imx2_wdt_remove(struct platform_device *pdev)
{
struct watchdog_device *wdog = platform_get_drvdata(pdev);
struct imx2_wdt_device *wdev = watchdog_get_drvdata(wdog);
watchdog_unregister_device(wdog);
if (imx2_wdt_is_running(wdev)) {
imx2_wdt_ping(wdog);
dev_crit(&pdev->dev, "Device removed: Expect reboot!\n");
}
return 0;
}
```
[...]
Additionally, the bigger structure can be used in the move method to track the device state, and particularly the watchdog data structure embedded inside. This is what the preceding code excerpt highlights.
572 Watchdog Device Drivers
So far, we have dealt with watchdog basics, walked through the base data structures, and described the main APIs. Now, we can learn about fancy features such as pretimeouts and governors in order to define the behavior of the system upon the watchdog event.
## Handling pretimeouts and governors
The concept of governor appears in several subsystems in the Linux kernel (thermal governors, CPUFreq governors, and now watchdog governors). It is nothing but a driver that implements policy management (sometimes in the form of an algorithm) that reacts to some states/events of the system.
The way each subsystem implements its governor drivers may be different from other subsystems, but the main idea remains the same. Moreover, governors are identified by a unique name and the governor (policy manager) in use. They may be changed on the fly,
most often from within the sysfs interface.
Now, back to watchdog pretimeouts and governors. Support for them can be added to the
Linux kernel by enabling the CONFIG_WATCHDOG_PRETIMEOUT_GOV kernel config option. There are actually two watchdog governor drivers in the kernel: drivers/
watchdog/pretimeout_noop.c and drivers/watchdog/pretimeout_
panic.c. Their unique names are, respectively, noop and panic. Either can be used by default by enabling CONFIG_WATCHDOG_PRETIMEOUT_DEFAULT_GOV_NOOP or
```c
CONFIG_WATCHDOG_PRETIMEOUT_DEFAULT_GOV_PANIC.
```
The main goal of this section is to deliver the pretimeout event to the watchdog governor that is currently active. This can be achieved by means of the watchdog_notify_
pretimeout() interface, which has the following prototype:
```c
void watchdog_notify_pretimeout(struct watchdog_device *wdd)
```
As we have discussed, some watchdog devices generate an IRQ in response to a pretimeout event. The main idea is to call watchdog_notify_pretimeout() from within this IRQ handler. Under the hood, this interface will internally find the watchdog governor (by looking for its name in the global list of watchdog governors registered with the system) and call its .pretimeout callback.
Just for your information, the following is what a watchdog governor structure looks like (you can find more information on watchdog governor drivers by looking at the source in drivers/watchdog/pretimeout_noop.c or drivers/watchdog/
pretimeout_panic.c):
```c
struct watchdog_governor {
const char name[WATCHDOG_GOV_NAME_MAXLEN];
```
Watchdog data structures and APIs 573 void (*pretimeout)(struct watchdog_device *wdd);
```c
};
```
Obviously, its fields have to be filled in by the underlying watchdog governor driver. For the real usage of a pretimeout notification, you can refer to the IRQ handler of the i.MX6 watchdog driver, defined in drivers/watchdog/imx2_wdt.c. An excerpt of this was shown earlier in the previous section. There, you will notice that watchdog_notify_
pretimeout() gets called from within the watchdog (the pretimeout, actually) IRQ
handler. Moreover, you will notice that the driver uses a different watchdog_info structure depending on whether there is a valid IRQ for the watchdog. If there is a valid one, the structure with the WDIOF_PRETIMEOUT flag set in .options is used, meaning that the device has a pretimeout feature. Otherwise, it uses the structure without the
```c
WDIOF_PRETIMEOUT flag set.
```
Now that we are familiar with the concept of governor and pretimeout, we can think about learning an alternative way of implementing watchdogs, such as GPIO-based ones.
## GPIO-based watchdogs
Sometimes, it may be better to use an external watchdog device instead of the one provided by the SoC itself, such as for power efficiency reasons, for example, as there are
SoCs whose internal watchdog requires much more power than external ones. Most of the time, if not always, this kind of external watchdog device is controlled through a GPIO
line and has the possibility to reset the system. It is pinged by toggling the GPIO line to which it is connected. This kind of configuration is used in UDOO QUAD (not checked on other UDOO variants).
The Linux kernel is able to handle this device by enabling the CONFIG_GPIO_WATCHDOG
config option, which will pull the underlying driver, drivers/watchdog/gpio_
wdt.c. If enabled, it will periodically ping the hardware connected to the GPIO line by toggling it from 1-to-0-to-1. If that hardware does not receive its ping periodically,
it will reset the system. You should use this instead of talking directly to the GPIOs using sysfs; it offers a better sysfs user space interface than the GPIO and it integrates with kernel frameworks better than your user space code could.
The support for this comes from the device tree only, and better documentation on its binding can be found in Documentation/devicetree/bindings/watchdog/
gpio-wdt.txt, obviously from within the kernel sources.
The following is a binding example:
```c
watchdog: watchdog {
dts
compatible = "linux,wdt-gpio";
574 Watchdog Device Drivers gpios = <&gpio3 9 GPIO_ACTIVE_LOW>;
hw_algo = "toggle";
hw_margin_ms = <1600>;
c
};
```
The compatible property must always be linux,wdt-gpio. gpios is a GPIO
```dts
specifier that controls the watchdog device. hw_algo should be either toggle or level. The former means that either low-to-high or high-to-low transitions should be used to ping the external watchdog device, and that the watchdog is disabled when the
```
GPIO line is left floating or connected to a three-state buffer. To achieve this, configuring the GPIO as input is sufficient. The second algo means that applying a signal level (high or low) is enough to ping the watchdog.
The way it works is the following: when user space code pings the watchdog through the
```dts
/dev/watchdog device file, the underlying driver (gpio_wdt.c, actually) will either toggle the GPIO line (1-0-1 if hw_algo is toggle) or assign a specific level (high or low if hw_algo is level) on that GPIO line. For example, the UDOO QUAD uses
```
APX823-31W5, a GPIO-controlled watchdog, whose event output is connected to the i.MX6 PORB line (reset line, actually). Its schematic is available here: http://udoo.
org/download/files/schematics/UDOO_REV_D_schematics.pdf.
Now, we are done with the watchdog on the kernel side. We went through the underlying data structure, dealt with its APIs, introduced the concept of pretimeouts, and even dealt with the GPIO-based watchdog alternative. In the next section, we will look into user space implementation, which is a kind of consumer of the watchdog services.
## The watchdog user space interface
On Linux-based systems, the standard user space interface to the watchdog is the /dev/
watchdog file, through which a daemon will notify the kernel watchdog driver that the user space is still alive. The watchdog starts right after the file is opened, and gets pinged by periodically writing into this file.
When the notification occurs, the underlying driver will notify the watchdog device,
which will result in resetting its timeout; the watchdog will then wait for yet another timeout duration prior to resetting the system. However, if for any reason the user space does not perform the notification before the timeout is elapsed, the watchdog will reset the system (causing a reboot). This mechanism provides a way to enforce the system availability. Let's start with the basics, learning how to start and stop the watchdog.
The watchdog user space interface 575
## Starting and stopping the watchdog
```c
The watchdog is automatically started once you open the /dev/watchdog device file,
```
as in the following example:
```c
int fd;
fd = open("/dev/watchdog", O_WRONLY);
if (fd == -1) {
if (errno == ENOENT)
printf("Watchdog device not enabled.\n");
else if (errno == EACCES)
printf("Run watchdog as root.\n");
```
else printf("Watchdog device open failed %s\n",
strerror(errno));
```c
exit(-1);
}
```
Only, closing the watchdog device file does not stop it. You will be surprised to face a system reset after closing the file. To properly stop the watchdog, you will first need to write the magic character V into the watchdog device file. This instructs the kernel to turn off the watchdog next time the device file is closed, as shown:
```c
const char v = 'V';
```
printf("Send magic character: V\n"); ret = write(fd, &v, 1);
```c
if (ret < 0)
```
printf("Stopping watchdog ticks failed (%d)...\n", errno);
Then, you'll need to close the watchdog device file in order to stop it:
```c
printf("Close for stopping..\n");
close(fd);
```
576 Watchdog Device Drivers
Important note
There is an exception when stopping the watchdog by closing the file device: it is when the kernel's CONFIG_WATCHDOG_NOWAYOUT config option is enabled. When this option is enabled, the watchdog cannot be stopped at all. Hence, you will need to service it all the time or it will reset the system. Moreover, the watchdog driver should have set the WDIOF_
MAGICCLOSE flag in its option; otherwise, the magic close feature won't work.
Now that we have seen how to start and stop the watchdog, it is time to learn how to refresh the device in order to prevent the system from suddenly rebooting.
## Pinging/kicking the watchdog – sending keep-alive pings
There are two ways to kick or feed the watchdog:
```c
1. Writing any character into /dev/watchdog: A write to the watchdog device file is defined as a keep-alive ping. It is recommended not to write a V character at all
```
(as it has a particular meaning), even if it is in a string.
2. Using the WDIOC_KEEPALIVE ioctl, ioctl(fd, WDIOC_KEEPALIVE, 0);:
The argument to the ioctl is ignored. The watchdog driver should have set the
```c
WDIOF_KEEPALIVEPING flag in its options prior to this ioctl so that it works.
```
It is good practice to feed the watchdog every half of its timeout value. This means if its timeout is 30s, you should feed it every 15s. Now, let's learn about gathering some information on how the watchdog is ruling our system.
## Getting watchdog capabilities and identity
Getting the watchdog capabilities and/or identity consists of grabbing the underlying struct watchdog_info structure associated with the watchdog. If you remember,
this info structure is mandatory and is provided by the watchdog driver.
To achieve this, you need to use the WDIOC_GETSUPPORT ioctl. The following is an example:
```c
struct watchdog_info ident;
ioctl(fd, WDIOC_GETSUPPORT, &ident);
printf("WDIOC_GETSUPPORT:\n");
```
/* Printing the watchdog's identity, its unique name actually
The watchdog user space interface 577
*/
```c
printf("\tident.identity = %s\n",ident.identity);
```
/* Printing the firmware version */
printf("\tident.firmware_version = %d\n",
ident.firmware_version);
/* Printing supported options (capabilities) in hex format */
printf("WDIOC_GETSUPPORT: ident.options = 0x%x\n",
ident.options);
We can go further by testing some fields in the capabilities, as follows:
```c
if (ident.options & WDIOF_KEEPALIVEPING)
printf("\tKeep alive ping reply.\n");
if (ident.options & WDIOF_SETTIMEOUT)
printf("\tCan set/get the timeout.\n");
```
You can (or should I say "must") use this in order to check the watchdog features prior to performing certain actions on it. Now, we can go further and learn how to get and set more fancy watchdog properties.
## Setting and getting the timeout and pretimeout
Prior to setting/getting the timeout, the watchdog info should have the WDIOF_
SETTIMEOUT flag set. There are drivers with which it is possible to modify the watchdog timeout on the fly using the WDIOC_SETTIMEOUT ioctl. These drivers must have the WDIOF_SETTIMEOUT flag set in their watchdog info structure and provide a .set_timeout callback.
While the argument here is an integer representing the timeout value in seconds, the return value is the real timeout applied to the hardware device, as it may differ from the one requested in the ioctl due to hardware limitations:
```c
int timeout = 45;
ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
printf("The timeout was set to %d seconds\n", timeout);
```
578 Watchdog Device Drivers
When it comes to querying the current timeout, you should use the WDIOC_
GETTIMEOUT ioctl, as in the following example:
```c
int timeout;
ioctl(fd, WDIOC_GETTIMEOUT, &timeout);
printf("The timeout is %d seconds\n", timeout);
```
Finally, when it comes to the pretimeout, the watchdog driver should have set
```c
WDIOF_PRETIMEOUT in the options and provided a .set_pretimeout callback in its ops. You should then use WDIOC_SETPRETIMEOUT with the pretimeout value as a parameter:
```
pretimeout = 10;
```c
ioctl(fd, WDIOC_SETPRETIMEOUT, &pretimeout);
```
If the desired pretimeout value is either 0 or bigger than the current timeout, you will get an -EINVAL error.
Now that we have seen how to get and set the timeout/pretimeout on the watchdog device,
we can learn how to get the time left before the watchdog fires.
## Getting the time left
The WDIOC_GETTIMELEFT ioctl allows checking how much time is left on the watchdog counter before a reset occurs. Moreover, the watchdog driver should support this feature by providing a .get_timeleft() callback; otherwise, you'll have an EOPNOTSUPP
error. The following is an example showing how to use this ioctl:
```c
int timeleft;
ioctl(fd, WDIOC_GETTIMELEFT, &timeleft);
printf("The remaining timeout is %d seconds\n", timeleft);
```
The timeleft variable is filled on the return path of the ioctl.
Once the watchdog fires, it triggers a reboot when it is configured to do so. In the next section, we will learn how to get the last reboot reason, in order to see whether the reboot was caused by the watchdog.
The watchdog user space interface 579
## Getting the (boot/reboot) status
There are two ioctl commands to play with in this section. These are WDIOC_GETSTATUS
and WDIOC_GETBOOTSTATUS. The way that these are handled depends on the driver implementation, and there are two types of driver implementations:
• Old drivers that provide watchdog features through a miscellaneous device. These drivers do not use the generic watchdog framework interface and provide their own file_ops along with their own .ioctl ops. Moreover, these drivers only support
```c
WDIOC_GETSTATUS, while others may support both WDIOC_GETSTATUS and
WDIOC_GETBOOTSTATUS. The difference between the two is that the former will return the raw content of the device's status register, while the latter is supposed to be a bit smarter as it parses the raw content and only returns the boot status flag.
```
These drivers need to be migrated to the new generic watchdog framework.
Note that some of those drivers supporting both commands may return the same value (the same case statement) for both ioctls, while others may return a different one (each command has its own case statement).
• New drivers use the generic watchdog framework. These drivers rely on the framework and do not care about file_ops anymore. Everything is done from within the drivers/watchdog/watchdog_dev.c file (you can have a look,
especially at how the ioctl command is implemented). With these kinds of drivers,
```c
WDIOC_GETSTATUS and WDIOC_GETBOOTSTATUS are handled by the watchdog core separately. This section will deal with these drivers.
```
Now, let's focus on the generic implementation. For these drivers, WDIOC_
GETBOOTSTATUS will return the value of the underlying watchdog_device.
bootstatus field. For WDIOC_GETSTATUS, if the watchdog .status ops is provided,
it will be called and its return value will be copied to the user; otherwise, the content of watchdog_device.bootstatus will be tweaked with an AND operation in order to clear (or flag) the bits that are not meaningful. The following code snippet shows how it is done in kernel space:
```c
static unsigned int watchdog_get_status(struct watchdog_device *wdd)
{
struct watchdog_core_data *wd_data = wdd->wd_data;
unsigned int status;
if (wdd->ops->status)
status = wdd->ops->status(wdd);
```
else
```c
580 Watchdog Device Drivers status = wdd->bootstatus &
```
(WDIOF_CARDRESET | WDIOF_OVERHEAT |
```c
WDIOF_FANFAULT | WDIOF_EXTERN1 |
WDIOF_EXTERN2 | WDIOF_POWERUNDER |
WDIOF_POWEROVER);
if (test_bit(_WDOG_ALLOW_RELEASE, &wd_data->status))
```
status |= WDIOF_MAGICCLOSE;
```c
if (test_and_clear_bit(_WDOG_KEEPALIVE, &wd_data->status))
```
status |= WDIOF_KEEPALIVEPING;
```c
return status;
}
```
The preceding code is a generic watchdog core function to get the watchdog status. It is actually a wrapper that is responsible for calling the underlying ops.status callback.
Now, back to our user space usage. We can do the following:
```c
int flags = 0;
int flags;
ioctl(fd, WDIOC_GETSTATUS, &flags);
```
/* or ioctl(fd, WDIOC_GETBOOTSTATUS, &flags); */
Obviously, we can proceed to individual flag checking as we did earlier in the Getting watchdog capabilities and identity section.
So far, we have written code to play with the watchdog device. The next section will show us how to deal with the watchdog from user space without writing code, essentially using the sysfs interface.
The watchdog user space interface 581
## The watchdog sysfs interface
The watchdog framework offers the possibility of managing watchdog devices from user space through the sysfs interface. This is possible if the CONFIG_WATCHDOG_
```c
SYSFS config option is enabled in the kernel, and the root directory is /sys/class/
```
watchdogX/. X is the index of the watchdog device in the system. Each watchdog directory in sysfs has the following:
• nowayout: Gives 1 if the device supports the nowayout feature, and 0 otherwise.
• status: This is the sysfs equivalent of the WDIOC_GETSTATUS ioctl. This sysfs file reports the watchdog's internal status bits.
• timeleft: This is the sysfs equivalent of the WDIOC_GETTIMELEFT ioctl.
This sysfs entry returns the time (the number of seconds, actually) left before the watchdog resets the system.
• timeout: Gives the current value of the timeout programmed.
• identity: Contains an identity string of the watchdog device.
• bootstatus: This is the sysfs equivalent of the WDIOC_GETBOOTSTATUS ioctl.
This entry informs whether the system reset was caused by the watchdog device or not.
• state: Gives the active/inactive status of the watchdog device.
Now that the preceding watchdog properties have been described, we can focus on pretimeout management from user space.
## Handling a pretimeout event
Setting the governor is done via sysfs. A governor is nothing but a policy manager that takes certain actions depending on some external (but input) parameters. There are thermal governors, CPUFreq governors, and now watchdog governors. Each governor is implemented in its own driver.
You can check the available governors for a watchdog (let's say watchdog0) using the following command:
```bash
# cat /sys/class/watchdog/watchdog0/pretimeout_available_
```
governors noop panic
582 Watchdog Device Drivers
Now, we can check whether pretimeout governors can be selected:
```bash
# cat /sys/class/watchdog/watchdog0/pretimeout_governor panic
# echo -n noop > /sys/class/watchdog/watchdog0/pretimeout_
```
governor
```bash
# cat /sys/class/watchdog/watchdog0/pretimeout_governor noop
```
To check the pretimeout value, you can simply do the following:
```bash
# cat /sys/class/watchdog/watchdog0/pretimeout
```
10
Now we are familiar with using the watchdog sysfs interface from the user space. Though we are not in the kernel, we can leverage the whole framework, particularly playing with the watchdog parameters.
## Summary
In this chapter, we discussed all aspects of watchdog devices: their APIs, the GPIO
alternative, and how they help keep the system reliable. We saw how to start, how (when it is possible) to stop, and how to service the watchdog devices. Moreover, we introduced the concept of pretimeout and watchdog-dedicated governors.
In the next chapter, we will discuss some Linux kernel development and debugging tips,
such as analyzing kernel panic messages and kernel tracing