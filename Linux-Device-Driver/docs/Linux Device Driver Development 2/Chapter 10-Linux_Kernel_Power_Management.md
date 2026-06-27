```bash
# Chapter 10 - Linux Kernel Power Management
```
Mobile devices are becoming increasingly complex with more and more features in order to follow commercial trends and satisfy consumers. While a few parts of such devices run proprietary or bare metal software, most of them run Linux-based operating systems
(embedded Linux distributions, Android, to name but a few), and all of them are battery powered. In addition to full functionality and performance, consumers require the longest possible autonomy and long-lasting batteries. It goes without saying that full performance and autonomy (power saving) are two totally incompatible concepts, and that a compromise must be found at all times when using the device. This compromise comes with Power Management, which allows us to deal with the lower consumption possible and device performance without ignoring the time needed for the device to wake up (or to be fully operational) after it has been put in a low-power state.
The Linux kernel comes with several power management capabilities, ranging from allowing you to save power during brief idle periods (or execution of tasks with lower power demands) to putting the whole system into a sleep state when it is not actively in use.
454 Linux Kernel Power Management
Additionally, as and when devices are added to the system, they can participate in this power management effort thanks to the generic Power Management APIs that the
Linux kernel offers in order to allow device driver developers to benefit from power management mechanisms implemented in devices, whatever they are. This allows either a per-device or system-wide power parameters to be adjusted in order to extend not only the autonomy of the device, but also the lifetime of the battery.
In this chapter, we will walk through the Linux kernel power management subsystem,
leveraging its APIs and managing its options from user space. Hence, the following topics will be covered:
• The concept of power management on Linux-based systems
• Adding power management capabilities to device drivers
• Being a source of system wakeup
## Technical requirements
For a better understanding of this chapter, you’ll require the following:
• Basic electrical knowledge
• Basic C programming skills
• Good knowledge of computer architecture
• Linux Kernel 4.19 sources available at https://github.com/torvalds/
linux
The concept of power management on Linux-based systems 455
The concept of power management on Linuxbased systems
Power management (PM) entails consuming as little power as possible at any time. There are two types of power management that the operating system must handle: Device Power
Management and System Power Management.
• Device Power Management: This is device specific. It allows a device to be put in a low-power state while the system is running. This may allow, among other things, part of the device not currently in use to be turned off in order to conserve power, such as the keyboard backlight when you are not typing. Individual device power management may be invoked explicitly on devices regardless of the power management activity, or may happen automatically after a device has been idle for a set amount of time. Device power management is an alias for the so-called Runtime
Power Management.
• System Power Management, also known as Sleep States: This enables platforms to enter a system-wide low-power state. In other words, entering a sleep state is the process by which the entire system is placed in a low-power state. There are several low-power states (or sleep states) that a system may enter, depending on the platform, its capabilities, and the target wake up latency. This happens, for example,
when the lid is closed on a laptop computer, when turning off the screen of the phone, or when some critical state has been reached (such as battery level). Many of these states are similar across platforms (such as freezing, which is purely software,
and hence not device or system dependent), and will be discussed in detail later. The general concept is that the state of the running system is saved before the system is powered down (or put into a sleep state, which is different from a shutdown),
and restored once the system has regained power. This prevents the system from performing an entire shutdown and startup sequence.
Although system PM and runtime PM deals with different scenarios for idle management,
deploying both is important to prevent wasting power for a platform. You should think of them as complementary, as we will see in forthcoming sections.
## Runtime power management
This is the part of Linux PM that manages power for individual devices without taking the whole system into a low-power state. In this mode, actions take effect while the system is running, hence its name, Runtime Power Management. In order to adapt device power consumption, its properties are changed on the fly with the system still in operation,
hence its other name, dynamic power management.
456 Linux Kernel Power Management
## A tour of some dynamic power management interfaces
Aside from per-device power management capabilities that driver developers can implement in device drivers, the Linux kernel provides user space interfaces to add/
remove/modify power policies. The most well-known of these are listed here:
• CPU Idle: This assists in managing CPU power consumption when it has no task to execute.
• CPUFreq: This allows CPU power properties (that is, voltage and frequency, which are related) to be changed depending on the system load.
• Thermal: This allows power properties to be adjusted according to temperatures sensed in predefined zones of the system, most of the time areas close to the CPU.
You may have noticed that the preceding policies deal with the CPU. This is because the
CPU is one of the principal sources of power dissipation on mobile devices (or embedded systems). While only three interfaces are introduced in the next sections, other interfaces exist, too, such as QoS and DevFreq. Readers are free to explore these to satisfy their curiosity.
## CPU Idle
Whenever a logical CPU in the system has no task to execute, it may need to be put in a particular state in order to save power. In this situation, most operating systems simply schedule a so-called idle thread. While executing this thread, the CPU is said to be idle,
or in an idle state. CPU Idle is a framework that manages idle threads. There are several levels (or modes or states) of idling. It depends on the built-in power-saving hardware embedded in the CPU. CPU idle modes are sometimes referred to as C-modes or even
C-states, which is an Advanced Configuration and Power Interface (ACPI) term. Those states usually start from C0, which is the normal CPU operating mode; in other words,
the CPU is 100% turned on. As the C number increases, the CPU sleep mode becomes deeper; in other words, more circuits and signals are turned off and the longer the amount of time the CPU will require to return to C0 mode, that is, to wake up. C1 is the first
C-state, C2 is the second one, and so on. When a logical processor is idle (any C-state except C0), its frequency is typically 0.
The next event (in time) determines how long the CPU can sleep for. Each idle state is described by three characteristics:
• An exit latency, in µS: This is the latency to get out of this state.
• A power consumption, in mW: This is not always reliable.
• A target residency, in µS: This is the idle duration from which it becomes interesting to use this state.
The concept of power management on Linux-based systems 457
CPU Idle drivers are platform specific, and the Linux kernel expects CPU drivers to support at most 10 states (see CPUIDLE_STATE_MAX in the kernel source code).
However, the real number of states depends on the underlying CPU hardware (which embeds built-in power-saving logic), and the majority of ARM platforms only provide one or two idle states. The choice of the state to enter is based on policies managed by governors.
A governor in this context is a simple module implementing an algorithm enabling the best C-state choice to be made, depending on some properties. In other words,
the governor is the one that decides the target C-state of the system. Though multiple governors can exist on the system, only one will be in control of a given CPU at any time.
It is designed in a way that, if the scheduler run queue is empty (which means the CPU
has nothing else to do) and it needs to idle the CPU, it will request CPU idling to the
CPU idle framework. The framework will then rely on the currently selected governor to select the appropriate C-state. There are two CPU Idle governors: ladder (for periodic timer tick-based systems) and menu (for tick-less systems). While the ladder governor is always available, if CONFIG_CPU_IDLE is selected, the menu governor additionally requires CONFIG_NO_HZ_IDLE (or CONFIG_NO_HZ on older kernels) to be set. The governor is selected while configuring the kernel. Roughly speaking, which of them to use depends on the configuration of the kernel and, in particular, on whether or not the scheduler tick can be stopped by the idle loop, hence CONFIG_NO_HZ_IDLE. You can refer to Documentation/timers/NO_HZ.txt for further reading on this.
```c
The governor may decide whether to continue in the current state or transition to a different state, in which case it will instruct the current driver to transition to the selected state. The current idle driver can be identified by reading the content of the /sys/
devices/system/cpu/cpuidle/current_driver file, and the current governor from /sys/devices/system/cpu/cpuidle/current_governor_ro:
bash
$ cat /sys/devices/system/cpu/cpuidle/current_governor_ro menu
c
On a given system, each directory in /sys/devices/system/cpu/cpuX/cpuidle/
```
corresponds to a C-state, and the contents of each C-state directory attribute files describing this C-state:
```bash
$ ls /sys/devices/system/cpu/cpu0/cpuidle/
```
state0 state1 state2 state3 state4 state5 state6 state7 state8
```bash
$ ls /sys/devices/system/cpu/cpu0/cpuidle/state0/
```
above below desc disable latency name power residency time usage
458 Linux Kernel Power Management
On ARM platforms, idle states can be described in the device tree. You can consult the
Documentation/devicetree/bindings/arm/idle-states.txt file in kernel sources for more reading on this.
Important note
Unlike other power management frameworks, CPU Idle requires no user intervention for it to work.
There is a framework slightly similar to this one, that is, CPU Hotplug, which allows the dynamic enabling and disabling of CPUs at runtime without having to reboot the system.
For example, to hotplug CPU #2 out of the system, you can use the following command:
```bash
# echo 0 > /sys/devices/system/cpu/cpu2/online
c
We can make sure that CPU #2 is actually disabled by reading /proc/cpuinfo:
bash
# grep processor /proc/cpuinfo processor : 0 processor : 1 processor : 3 processor : 4 processor : 5 processor : 6 processor : 7
```
The preceding confirms that CPU2 is now offline. In order to hotplug that CPU back into the system, we can execute the following command:
```bash
# echo 1 > /sys/devices/system/cpu/cpu2/online
```
What CPU hotplugging does under the hood will depend on your particular hardware and drivers. It may simply result in the CPU being put into idle on some systems, whereas other systems may physically remove power from the specified core.
## CPUfreq or dynamic voltage and frequency scaling (DVFS)
This framework allows dynamic voltage selection and frequency scaling for the CPU,
based on constraints and requirements, user preferences, or other factors. Because this framework deals with frequency, it unconditionally involves a clock framework. This framework uses the concept of Operating Performance Points (OPPs), which consists of representing the performance state of a system with {Frequency,voltage} tuples.
The concept of power management on Linux-based systems 459
OPPs can be described in the device tree, and its binding documentation in the kernel sources can be a good starting point for more information on it: Documentation/
devicetree/bindings/opp/opp.txt.
Important note
You’ll occasionally come across the term P-state. This is also an ACPI term
```c
(as is C-state) to designate CPU built-in hardware OPPs. This is the case with some Intel CPUs, and the operating system uses the policy objects to deal with these. You can check the result of ls /sys/devices/system/cpu/
```
cpufreq/ on an Intel-based machine. Thus, C-states are idle power-saving states, in contrast to P-states, which are execution power-saving states.
CPUfreq also uses the concept of governors (which implement scaling algorithms), and the governors in this framework are as follows:
• ondemand: This governor samples the load of the CPU and scales it up aggressively in order to provide the proper amount of processing power, but resets the frequency to the maximum when necessary.
• conservative: This is similar to ondemand, but uses a less aggressive method of increasing the OPP. For example, it will never skip from the lowest OPP to the highest one even if the system suddenly requires high performance. It will do it progressively.
• performance: This governor always selects the OPP with the highest frequency possible. This governor prioritizes performance.
• powersave: In contrast to performance, this governor always selects the OPP with the lowest frequency possible. This governor prioritizes power saving.
```c
• userspace: This governor allows the user to set the desired OPP using any value found within /sys/devices/system/cpu/cpuX/cpufreq/scaling_
available_frequencies by echoing it into /sys/devices/system/cpu/
```
cpuX/cpufreq/scaling_setspeed.
• schedutil: This governor is part of the scheduler, so it can access the scheduler data structure internally, allowing it to grab more reliable and accurate stats about the system load, for the purpose of better selecting the appropriate OPP.
460 Linux Kernel Power Management
The userspace governor is the only one that allows users to select the OPP. For other governors, OPP change happens automatically based on the system load of their algorithm. That said, from userspace, the governors available are listed here:
```bash
$ cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_
```
governors performance powersave
To view the current governor, implement the following command:
```bash
$ cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor powersave
```
To set a governor, the following command can be used:
```bash
$ echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/
```
scaling_governor
To view the current OPP (frequency in kHz), implement the following command:
```bash
$ cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
```
800031
To view supported OPPs (frequency in kHz), implement the following command:
```bash
$ cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_
```
frequencies
275000 500000 600000 800031
To change the OPP, you can use the following command:
```bash
$ echo 275000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_
```
setspeed
Important note
There is also the devfreq framework, which is a generic Dynamic Voltage and Frequency Scaling (DVFS) framework for non-CPU devices, and with governors such as Ondemand, performance, powersave, and passive.
The concept of power management on Linux-based systems 461
Note that the preceding command only works when the ondemand governor is selected,
as it is the only one that allows the OPP to be changed. However, in all the preceding commands, cpu0 has been used only for didactic purposes. Think of it like cpuX, where X
is the index of the CPU as seen by the system.
## Thermal
This framework is dedicated to monitoring the system temperature. It has dedicated profiles according to temperature thresholds. Thermal sensors sense the hot points and report. This framework works in conjunction with cooling devices, which aid in power dissipation to control/limit overheating.
The thermal framework uses the following concepts:
• Thermal zones: You can think of a thermal zone as hardware whose temperature needs to be monitored.
• Thermal sensors: These are components used to take temperature measurements.
Thermal sensors provide temperature sensing capabilities in thermal zones.
• Cooling devices: These devices provide control in terms of power dissipation.
Typically, there are two cooling methods: passive cooling, which consists of regulating device performance, in which case DVFS is used; and active cooling,
which consists of activating special cooling devices, such as fans (GPIO-fan,
PWM-fan).
• Trip points: These describe key temperatures (actually thresholds) at which a cooling action is recommended. Those sets of points are chosen based on hardware limits.
• Governors: These comprise algorithms to choose the best cooling according to some criteria.
• Cooling maps: These are used to describe links between trip points and cooling devices.
The thermal framework can be divided into four parts, these being thermal zone,
```c
thermal governor, thermal cooling, and thermal core, which is the glue between the three previous parts. It can be managed in user space from within the /sys/
```
class/thermal/ directory:
```bash
$ ls /sys/class/thermal/
```
cooling_device0 cooling_device4 cooling_device8 thermal_zone3 thermal_zone7 cooling_device1 cooling_device5 thermal_zone0 thermal_zone4
462 Linux Kernel Power Management cooling_device2 cooling_device6 thermal_zone1 thermal_zone5 cooling_device3 cooling_device7 thermal_zone2 thermal_zone6
In the preceding, each thermal_zoneX file represents a thermal zone driver, or a thermal driver. A thermal zone driver is the driver of the thermal sensor associated with a thermal zone. This driver exposes trip points at which cooling is necessary, but also provides a list of cooling devices associated with the sensor. The thermal workflow is designed to obtain the temperature through the thermal zone driver, and then make decisions through the thermal governor, and finally perform temperature control by means of thermal cooling. Further reading on this is available in the thermal sysfs documentation in the kernel sources, Documentation/thermal/sysfs- api.txt.
Moreover, thermal zone description, trip point definitions, and cooling device binding can be performed in the device tree, and its associated documentation in the sources is
Documentation/devicetree/bindings/thermal/thermal.txt.
## System power management sleep states
System power management targets the entire system. Its aim is to put it into a low-power state. In this low-power state, the system is consuming a small, but minimal amount of power, yet maintaining a relatively low response latency to the user. The exact amount of power and response latency depends on how deep is the sleep state that the system is in. This is also referred to as Static Power Management because it is activated when the system is inactive for an extended period.
The states a system can enter are dependent on the underlying platform, and differ across architectures and even generations or families of the same architecture. There are,
however, four sleep states that are commonly found on most platforms. These are suspend to idle (also known as freeze), power-on standby (standby), suspend to ram (mem), and suspend to disk (hibernation). These are also occasionally referred to by their ACPI states:
S0, S1, S3, and S4, respectively:
```bash
# cat /sys/power/state freeze mem disk standby
c
CONFIG_SUSPEND is the kernel configuration option that must be set in order for the system to support the system’s power management sleep state. That said, except for freeze,
```
each sleep state is platform specific. Thus, for a platform to support any of the three remaining states, it must explicitly register for each state with the core system suspend subsystem. However, the support for hibernation depends on other kernel configuration options, as we will see later.
The concept of power management on Linux-based systems 463
Important note
Because only the user knows when the system is not going to be used (or even user code, such as GUI), system power management actions are always initiated from the user space. The kernel has no idea of that. This is why most of the content in this section deals with sysfs and the command line.
## Suspend to idle (freeze)
This is the most basic and lightweight. This state is purely software driven and involves keeping the CPUs in their deepest idle state as much as possible. To achieve this, the user space is frozen (all user space tasks are) and all I/O devices are put into low-power states
(possibly lower power than available at runtime) so that the processors can spend more time in their idle states. The following is the command that idles the system:
```bash
$ echo freeze > /sys/power/state
```
The preceding command puts the system in an idle state. Because it is purely software, this state is always supported (assuming the CONFIG_SUSPEND kernel configuration option is set). This state can be used for platforms without power-on-suspend or suspend-to-ram support. However, as we will see later, it can be used in addition to suspend-to-ram to provide reduced resume latency.
Important note
Suspend to idle equals frozen processes + suspended devices + idle processors
## Power-on standby (standby or power-on suspend)
In addition to freezing the user space and putting all I/O devices into a low-power state,
another action performed by this state is to power off all non-boot CPUs. The following is the command that puts the system in standby, assuming it is supported by the platform:
```bash
$ echo standby > /sys/power/state
```
As this state goes further than the freeze state, it also allows more energy to be saved relative to Suspend-to-Idle, but the resume latency will generally be greater than for the freeze state, although it is quite low.
464 Linux Kernel Power Management
## Suspend-to-ram (suspend, or mem)
In addition to putting everything in the system into a low-power state, this state goes further by powering off all CPUs and putting the memory into self-refresh so that its contents are not lost, although additional operations may take place depending on the platform’s capabilities. Response latency is higher than standby, yet still quite low. In this state, the system and device state is saved and kept in memory. This is the reason why only the RAM is fully operational, hence the state name:
```bash
# echo mem > /sys/power/state
```
The preceding command is supposed to put the system into a Suspend-to-RAM state.
However, the real actions performed while writing the mem string are controlled by the
```c
/sys/power/mem_sleep file. This file contains a list of strings where each string represents a mode the system can enter after mem has been written to /sys/power/
```
state. Although not all are always available (it depends on the platform), possible modes include the following:
• s2idle: This is the equivalent of Suspend-to-Idle. For this reason, it is always available.
• shallow: This is equivalent to Power-On Suspend, or standby. Its availability depends on the platform’s support of the standby mode.
• deep: This is the real Suspend-To-RAM state and its availability depends on the platform.
An example of querying the content can be seen here:
```bash
$ cat /sys/power/mem_sleep
```
[s2idle] deep
```c
The selected mode is enclosed in square brackets, [ ]. If a mode is not supported by the platform, the string that corresponds to it will still not be present in /sys/power/
mem_sleep. Writing one of the other strings present in /sys/power/mem_sleep to it causes the suspend mode to be used subsequently to change to the one represented by that string.
When the system is booted, the default suspend mode (in other words, the one to be used without writing anything into /sys/power/mem_sleep) is either deep (if SuspendTo-RAM is supported) or s2idle, but it can be overridden by the value of the mem_
```
sleep_default parameter in the kernel command line.
The concept of power management on Linux-based systems 465
One method for testing this is to use an RTC available on the system, assuming it supports the wakeup alarm feature. You can identify available RTCs on your system using ls /
sys/class/rtc/. There will be a directory for each RTC (in other words, rtc0 and rtc1). For an rtc that supports the alarm feature, there will be a wakealarm file in that rtc directory, which can be used as follows to configure an alarm and then suspend the system to RAM:
/* No value returned means no alarms are set */
```bash
$ cat /sys/class/rtc/rtc0/wakealarm
```
/* Set the wakeup alarm for 20s */
```bash
# echo +20 > /sys/class/rtc/rtc0/wakealarm
```
/* Now Suspend system to RAM */
```bash
# echo mem > /sys/power/state
```
You should see no further activity on the console until wakeup.
## Suspend to disk (hibernation)
This state gives the greatest power savings as a result of powering off as much of the system as possible, including the memory. Memory contents (a snapshot) are written to persistent media, usually a disk. After this, memory is powered down, along with the entire system. Upon resumption, the snapshot is read back into memory and the system boots from this hibernation image. However, this state is also the longest to resume but still quicker than performing a full (re)boot sequence:
```bash
$ echo disk > /sys/power/state
c
Once the memory state is written to disk, several actions can take place. The action to be performed is controlled by the /sys/power/disk file and its contents. This file contains a list of strings where each string represents an action that can be performed once the system state is saved on persistent storage media (after the hibernation image has actually been saved). Possible actions include the following:
```
• platform: Custom- and platform-specific, which may require firmware (BIOS)
intervention.
• shutdown: Power off the system.
• reboot: Reboots the system (useful for diagnostics mostly).
466 Linux Kernel Power Management
• suspend: Puts the system into the suspend sleep state selected through the mem_sleep file described earlier. If the system is successfully woken up from that state, then the hibernation image is simply discarded and everything continues.
Otherwise, the image is used to restore the previous state of the system.
• test_resume: This is for system resumption diagnostic purposes. Loads the image as if the system had just woken up from hibernation and the currently running kernel instance was a restore kernel and follows up with full system resumption.
```c
However, supported actions on a given platform depend on the content of the /sys/
```
power/disk file:
```bash
$ cat /sys/power/disk
```
[platform] shutdown reboot suspend test_resume
The selected action is enclosed in square brackets, [ ]. Writing one of the listed strings to this file causes the option represented by it to be selected. Hibernation is such a complex operation that it has its own configuration option, CONFIG_HIBERNATION. This option has to be set in order to enable the hibernation feature. That said, this option can only be set if support for the given CPU architecture includes the low-level code for system resumption (refer to the ARCH_HIBERNATION_POSSIBLE kernel configuration option).
For suspend-to-disk to work, and depending on where the hibernation image should be stored, a dedicated partition may be required on the disk. This partition is also known as a swap partition. This partition is used to write memory contents to free swap space. In order to check whether hibernation works as expected, it is common to try to hibernate in reboot mode as follows:
```bash
$ echo reboot > /sys/power/disk
# echo disk > /sys/power/state
```
The first command informs the power management core of what action should be performed when the hibernation image has been created. In this case, it is a reboot. Upon reboot, the system is restored from the hibernation image and you should get back to the command prompt where you started the transition. The success of this test may show that hibernation is most likely to work correctly. That said, it should be done several times in order to reinforce the test.
Now that we are done with sleep state management from a running system, we can see how to implement its support in driver code.
Adding power management capabilities to device drivers 467
Adding power management capabilities to device drivers
Device drivers on their own can implement a distinct power management capability,
which is known as runtime power management. Not all devices support runtime power management. However, those that do must export some callbacks for controlling their power state depending on the user or system’s policy decisions. As we have seen earlier, this is device-specific. In this section, we will learn how to extend device driver capabilities with power management support.
Though device drivers provide runtime power management callbacks, they also facilitate and participate in the system sleep state by providing another set of callbacks, where each set participates in a particular system sleep state. Whenever the system needs to enter or resume from a given set, the kernel will walk through each driver that provided callbacks for this state and then invoke them in a precise order. Simply speaking, device power management consists of a description of the state a device is in, and a mechanism for controlling those states. This is facilitated by the kernel providing the struct dev_
```c
pm_ops that each device driver/class/bus interested in power management must fill. This allows the kernel to communicate with every device in the system, regardless of the bus the device resides on or the class it belongs to. Let’s take a step back and remember what a struct device looks like:
struct device {
```
[...]
```c
struct device *parent;
struct bus_type *bus;
struct device_driver *driver;
struct dev_pm_info power;
struct dev_pm_domain *pm_domain;
}
```
In the preceding struct device data structure, we can see that a device can be either a child (its .parent field points to another device) or a device parent (when the .parent field of another device points to it), can sit behind a given bus, or can belong to a given class, or can belong indirectly to a given subsystem. Moreover, we can see that a device can be part of a given power domain. The .power field is of the struct dev_pm_info type. It mainly saves PM-related states, such as the current power state, whether it can be awakened, whether it has been prepared, and whether it has been suspended. Since there is so much content involved, we will explain these in detail when we use them.
468 Linux Kernel Power Management
In order for devices to participate in power management, either at the subsystem level or at the device driver level, their drivers need to implement a set of device power management operations by defining and populating objects of the struct dev_pm_
ops type defined in include/linux/pm.h as follows:
```c
struct dev_pm_ops {
int (*prepare)(struct device *dev);
void (*complete)(struct device *dev);
int (*suspend)(struct device *dev);
int (*resume)(struct device *dev);
int (*freeze)(struct device *dev);
int (*thaw)(struct device *dev);
int (*poweroff)(struct device *dev);
int (*restore)(struct device *dev);
```
[...]
```c
int (*suspend_noirq)(struct device *dev);
int (*resume_noirq)(struct device *dev);
int (*freeze_noirq)(struct device *dev);
int (*thaw_noirq)(struct device *dev);
int (*poweroff_noirq)(struct device *dev);
int (*restore_noirq)(struct device *dev);
int (*runtime_suspend)(struct device *dev);
int (*runtime_resume)(struct device *dev);
int (*runtime_idle)(struct device *dev);
};
```
In the preceding data structure, *_early() and *_late() callbacks have been removed for the sake of readability. I suggest you have a look at the full definition. That said, given the huge number of callbacks in there, we will describe them in due course in the sections of the chapter where their use will be necessary.
Important note
Device power states are sometimes referred to as D states, inspired by the
PCI device and ACPI specifications. Those states range from state D0 to D3,
inclusive. Although not all device types define power states in this way, this representation can map to all known device types.
Adding power management capabilities to device drivers 469
## Implementing runtime PM capability
Runtime power management is a per-device power management feature allowing a particular device to have its states controlled when the system is running, regardless of the global system. For a driver to implement runtime power management, it should provide only a subset of the whole list of callbacks in struct dev_pm_ops, shown as follows:
```c
struct dev_pm_ops {
```
[...]
```c
int (*runtime_suspend)(struct device *dev);
int (*runtime_resume)(struct device *dev);
int (*runtime_idle)(struct device *dev);
};
```
The kernel also provides SET_RUNTIME_PM_OPS(), which accepts the three callbacks to be populated in the structure. This macro is defined as follows:
```c
#define SET_RUNTIME_PM_OPS(suspend_fn, resume_fn, idle_fn) \
```
.runtime_suspend = suspend_fn, \
.runtime_resume = resume_fn, \
.runtime_idle = idle_fn,
The preceding callbacks are the only ones involved in runtime power management, and here are descriptions of what they must do:
• .runtime_suspend() must record the device’s current state if necessary and put the device in a quiescent state. This method is invoked by the PM when the device is not used. In its simple form, this method must put the device in a state in which it won’t be able to communicate with the CPU(s) and RAM.
• .runtime_resume() is invoked when the device must be put in a fully functional state. This may be the case if the system needs to access this device. This method must restore power and reload any required device state.
470 Linux Kernel Power Management
• .runtime_idle() is invoked when the device is no longer used based on the device usage counter (actually when it reaches 0), as well as the number of active children. However, the action performed by this callback is driver-specific. In most cases, the driver invokes runtime_suspend() on the device if some conditions are met, or invokes pm_schedule_suspend() (given a delay in order to set up a timer to submit a suspend request in future), or pm_runtime_autosuspend()
(to schedule a suspend request in the future based on a delay that has already been set using pm_runtime_set_autosuspend_delay()). If the .runtime_
idle callback doesn’t exist or if it returns 0, the PM core will immediately invoke the .runtime_suspend() callback. For the PM core to do nothing, .runtime_
idle() must return a non-zero value. It is common for drivers to return -EBUSY,
or 1 in this case.
After callbacks have been implemented, they can be fed in struct dev_pm_ops, as in the following example:
```c
static const struct dev_pm_ops bh1780_dev_pm_ops = {
SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
pm_runtime_force_resume)
SET_RUNTIME_PM_OPS(bh1780_runtime_suspend,
```
bh1780_runtime_resume, NULL)
```c
};
```
[...]
```c
static struct i2c_driver bh1780_driver = {
```
.probe = bh1780_probe,
.remove = bh1780_remove,
.id_table = bh1780_id,
```c
.driver = {
```
.name = “bh1780”,
.pm = &bh1780_dev_pm_ops,
.of_match_table = of_match_ptr(of_bh1780_match),
```c
},
};
module_i2c_driver(bh1780_driver);
```
Adding power management capabilities to device drivers 471
The preceding is an excerpt from drivers/iio/light/bh1780.c, an IIO ambient light sensor driver. In this excerpt, we can see how struct dev_pm_ops is populated,
using convenient macros. SET_SYSTEM_SLEEP_PM_OPS is used here to populate system sleep-related macros, as we will see in the next sections. pm_runtime_force_
suspend and pm_runtime_force_resume are special helpers that the PM core exposes to force device suspension and resumption, respectively.
## Runtime PM anywhere in the driver
In fact, the PM core keeps track of the activity of each device using two counters. The first counter is power.usage_count, which counts active references to the device. These may be external references, such as open file handles, or other devices that are making use of this one, or they may be internal references used to keep the device active for the duration of an operation. The other counter is power.child_count, which counts the number of children that are active.
These counters define the active/idle conditions of a given device from the PM point of view. The active/idle condition of a device is the only reliable means for the PM core to determine whether a device is accessible. An idle condition is when the device usage count is decremented until 0, and an active condition (also known as a resume condition)
occurs whenever the device usage count is incremented.
In the event of an idle condition, the PM core sends/performs an idle notification (that is,
setting the device’s power.idle_notification field to true, invoking the bus type/
```c
class/device ->runtime_idle() callback, and setting the .idle_notification field back to false again) in order to check whether the device can be suspended.
If the ->runtime_idle() callback doesn’t exist or if it returns 0, the PM core will immediately invoke the ->runtime_suspend() callback to suspend the device, after which the device’s power.runtime_status field is set to RPM_SUSPENDED, which means the device is suspended. Upon a resume condition (the device usage count is incremented), the PM core will carry out a resumption (under certain conditions only) of this device, either synchronously or asynchronously. Have a look at the rpm_resume()
```
function and its description in drivers/base/power/runtime.c.
472 Linux Kernel Power Management
Initially, the runtime PM is disabled for all devices. This means invoking most PM-related helpers on the device will fail until pm_runtime_enable() is called for the device,
which enables a runtime PM of this device. Though the initial runtime PM status of all devices is suspended, it need not reflect the actual physical state of the device. Thus, if the device is initially active (in other words, it is able to process I/O), its runtime PM status must be changed to active with the help of pm_runtime_set_active() (which will set power.runtime_status to RPM_ACTIVE), and if possible, its usage count must be increased using pm_runtime_get_noresume() before pm_runtime_enable()
is called for the device. Once the device is fully initialized, you can call pm_runtime_
put() on it.
The reason for invoking pm_runtime_get_noresume() here is that, if there is a call to pm_runtime_put(), the device usage count will come back to zero, which corresponds to an idle condition, and then an idle notification will be carried out. At this time, you’ll be able to check whether necessary conditions have been met and suspend the device. However, if the initial device state is disabled, there is no need to do so.
There are also pm_runtime_get(), pm_runtime_get_sync(), pm_runtime_
put_noidle(), and pm_runtime_put_sync() helpers. The difference between pm_runtime_get_sync(), pm_runtime_get(), and pm_runtime_get_
noresume() is that the former will synchronously (immediately) carry out a resumption of the device if the active/resume condition is matched after the device usage count has been incremented, while the second helper will do it asynchronously (submitting a request for it). The third and final one will return immediately after having decremented the device usage count (without even checking the resume condition). The same mechanism applies to pm_runtime_put_sync(), pm_runtime_put(), and pm_runtime_
put_noidle().
The number of active children of a given device affects the usage count of this device.
Normally, the parent is needed to access the child, so powering down the parent while children are active would be counterproductive. Sometimes, however, it might be necessary to ignore active children of a device when determining whether this device is idle. One good example is the I2C bus, where the bus can be reported as idle while devices sitting on this bus (children) are active. For such cases, pm_suspend_ignore_
children() can be invoked to allow a device to report as idle even when it has active children(s).
Adding power management capabilities to device drivers 473
## Runtime PM synchronous and asynchronous operations
In the previous section, we introduced the fact that the PM core could carry out synchronous or asynchronous PM operations. While things are straightforward for synchronous operations (method calls are serialized), we need to pay some attention to what steps are performed while invoking things asynchronously in a PM context.
You should keep in mind that, in asynchronous mode, a request for the action is submitted instead or invoking this action’s handler immediately. It works as follows:
1. The PM core sets the device’s power.request field (which is of the enum rpm_request type) with the type of request to be submitted (in other words,
RPM_REQ_IDLE for an idle notification request, RPM_REQ_SUSPEND for a suspend request, or RPM_REQ_AUTOSUSPEND for an autosuspend request), which corresponds to the action to be performed.
2. The PM core sets the device’s power.request_pending field to true.
3. The PM core queues (schedules for a later execution) the device’s RPM-related work (power.work, whose work function is pm_runtime_work(); see pm_
runtime_init() where it is initialized) in the Global PM-related work queue.
4. When this work has the chance to run, the work function (that is, pm_runtime_
work()) will first check whether there is still a request pending on the device (if
```c
(dev->power.request_pending)) and perform a switch ... case on the device’s power.request_pending field in order to invoke the underlying request handler.
```
Do note that a work queue manages its own thread(s), which can run scheduled works.
Because, in asynchronous mode, the handler is scheduled in a work queue, asynchronous
PM-related helpers are totally safe to be invoked in an atomic context. If invoked within an IRQ handler, for example, it would be equivalent to deferring the PM request handling.
## Autosuspend
Autosuspend is a mechanism used by drivers that do not want their device to suspend as soon as it becomes idle at runtime, but they rather want the device to remain inactive for a certain minimum period of time first.
474 Linux Kernel Power Management
In the context of RPM, the term autosuspend does not mean the device automatically suspends itself. It is instead based on a timer that, upon expiration, will queue a suspend request. This timer is actually the device’s power.suspend_timer field (see pm_
runtime_init() where it is set up). Calling pm_runtime_put_autosuspend()
```c
will start the timer, while pm_runtime_set_autosuspend_delay() will set the timeout (though that can be set via sysfs in the /sys/devices/.../
```
power/autosuspend_delay_ms attribute) represented by the device’s power.
autosuspend_delay field.
This timer can be used by the pm_schedule_suspend() helper as well, with a delay in argument (which in this case will take precedence on the one set in the power.
autosuspend_delay field), after which a suspend request will be submitted. You can regard this timer as something that can be used to add a delay between the counters reaching zero and the device being considered to be idle. This is useful for devices with a high cost associated with turning on or off.
In order to use autosuspend, subsystems or drivers must call pm_runtime_use_
autosuspend() (preferably before registering the device). This helper will set the device’s power.use_autosuspend field to true. After soliciting a device on which autosuspend is enabled, you should invoke pm_runtime_mark_last_busy() on this device, which lets it set the power.last_busy field to the current time (in jiffies),
because this field is used in calculating inactivity periods for autosuspend (for example,
new_expire_time = last_busy + msecs_to_jiffies(autosuspend_
delay)).
Given all the runtime PM concepts introduced, let’s put it all together now and see how things are done in a real driver.
## Putting it all together
The preceding theoretical studies of the runtime PM core would be less significant without a genuine case study. Now is the time to see how the previous concept is applied. For this case study, we will pick the bh1780 Linux driver, which is a digital 16-bit I2C ambient light sensor. The driver of this device is drivers/iio/light/bh1780.c in the Linux kernel sources.
Adding power management capabilities to device drivers 475
To start with, let’s see an excerpt of the probe method:
```c
static int bh1780_probe(struct i2c_client *client,
const struct i2c_device_id *id)
{
```
[...]
/* Power up the device */ [...]
```c
pm_runtime_get_noresume(&client->dev);
pm_runtime_set_active(&client->dev);
pm_runtime_enable(&client->dev);
```
ret = bh1780_read(bh1780, BH1780_REG_PARTID);
```c
dev_info(&client->dev, “Ambient Light Sensor, Rev : %lu\n”,
```
(ret & BH1780_REVMASK));
/*
* As the device takes 250 ms to even come up with a fresh
* measurement after power-on, do not shut it down
* unnecessarily.
* Set autosuspend to five seconds.
*/
```c
pm_runtime_set_autosuspend_delay(&client->dev, 5000);
pm_runtime_use_autosuspend(&client->dev);
pm_runtime_put(&client->dev);
```
[...]
ret = iio_device_register(indio_dev);
```c
if (ret)
```
goto out_disable_pm; return 0;
out_disable_pm:
```c
pm_runtime_put_noidle(&client->dev);
pm_runtime_disable(&client->dev); return ret;
}
```
476 Linux Kernel Power Management
In the preceding snippet, only the power management-related calls are left, for the sake of readability. First, pm_runtime_get_noresume() will increment the device usage count without carrying an idle notification of the device (the _noidle suffix). You may use the pm_runtime_get_noresume() interface to turn off the runtime suspend function or to make the usage count positive even while the device is suspended, so as to avoid issues that do not wake up normally due to the runtime suspension. Then, the next line in the driver is pm_runtime_set_active(). This helper marks the device as active (power.runtime_status = RPM_ACTIVE) and clears the device’s power.
runtime_error field. Additionally, the device parent’s counter of unsuspended (active)
children is modified to reflect the new status (it is incremented actually). Invoking pm_
runtime_set_active() on a device will prevent this device’s parent from suspending at runtime (assuming the parent’s runtime PM is enabled), unless the parent’s power.
ignore_children flag is set. For this reason, once pm_runtime_set_active()
has been called for the device, pm_runtime_enable() should be called for it too,
as soon as is reasonably possible. Invoking this function is not mandatory; it has to be coherent with the PM core and the status of the device, assuming the initial status is RPM_
SUSPENDED.
Important note
The opposite of pm_runtime_set_active() is pm_runtime_set_
suspended(), which changes the device status to RPM_SUSPENDED, and decrements the parent’s counter of active children. An idle notification request for the parent is submitted.
```c
pm_runtime_enable() is the mandatory runtime PM helper, which enables the runtime PM of a device, that is, de-increments the device’s power.disable_depth value in case its value is greater than 0. For information, the device’s power.disable_
```
depth value is checked on each runtime PM helper call, and its value must be 0 for the helper to progress. Its initial value is 1, and this value is decremented upon a call to pm_
runtime_enable(). On the error path, pm_runtime_put_noidle() is invoked in order to make the PM runtime counter balance, and pm_runtime_disable()
completely disables the runtime PM on the device.
Adding power management capabilities to device drivers 477
As you may have guessed, this driver also deals with the IIO framework, which means it exposes entries in sysfs, which correspond to its physical conversion channels. Reading the sysfs file corresponding to a channel will report the digital value of the conversion resulting from this channel. However, for the bh1780, the channel read entry point in its driver is bh1780_read_raw(). An excerpt of this method can be seen here:
```c
static int bh1780_read_raw(struct iio_dev *indio_dev,
struct iio_chan_spec const *chan,
int *val, int *val2, long mask)
{
struct bh1780_data *bh1780 = iio_priv(indio_dev);
int value;
switch (mask) {
```
case IIO_CHAN_INFO_RAW:
```c
switch (chan->type) {
```
case IIO_LIGHT:
```c
pm_runtime_get_sync(&bh1780->client->dev);
```
value = bh1780_read_word(bh1780, BH1780_REG_DLOW);
```c
if (value < 0)
return value;
pm_runtime_mark_last_busy(&bh1780->client->dev);
pm_runtime_put_autosuspend(&bh1780->client->dev);
```
*val = value;
```c
return IIO_VAL_INT;
```
default:
```c
return -EINVAL;
```
case IIO_CHAN_INFO_INT_TIME:
*val = 0;
*val2 = BH1780_INTERVAL * 1000;
```c
return IIO_VAL_INT_PLUS_MICRO;
```
default:
```c
return -EINVAL;
}
}
```
478 Linux Kernel Power Management
Here again, only runtime PM-related function calls are worthy of our attention. In the event of a channel read, the preceding function is invoked. The device driver has to instruct the device to sample the channel, to perform the conversion whose result will be read by the device driver and reported to the reader. The thing is, the device may be in a suspended state. Thus, because the driver needs immediate access to the device, the driver calls pm_runtime_get_sync() on it. If you recall, this method increments the device usage count and carries out a synchronous (_sync suffix) resumption of the device.
After the device resumes, the driver can talk with the device and read the conversion value. Because the driver supports autosuspend, pm_runtime_mark_last_busy()
is called in order to mark the last time the device was active. This will update the timeout value of the timer used for autosuspend. Finally, the driver invokes pm_runtime_put_
autosuspend(), which will carry out a runtime suspend of the device following the autosuspend timer expiration, unless this timer is restarted again by pm_runtime_
mark_last_busy() being invoked somewhere or when entering the read function again (the reading of the channel, in sysfs, for example) prior to expiration.
To summarize, before accessing the hardware, the driver can resume the device with pm_
runtime_get_sync(), and when it’s finished with the hardware, the driver can notify the device as being idle with either pm_runtime_put_sync(), pm_runtime_put(),
or pm_runtime_put_autosuspend() (assuming autosuspend is enabled, in which case pm_runtime_mark_last_busy() must be invoked beforehand in order to update the autosuspend timer’s timeout).
Finally, let’s focus on the method invoked when the module is being unloaded. The following is an excerpt in which only PM-related calls are of interest:
```c
static int bh1780_remove(struct i2c_client *client)
{
int ret;
struct iio_dev *indio_dev = i2c_get_clientdata(client);
struct bh1780_data *bh1780 = iio_priv(indio_dev);
iio_device_unregister(indio_dev);
pm_runtime_get_sync(&client->dev);
pm_runtime_put_noidle(&client->dev);
pm_runtime_disable(&client->dev);
```
ret = bh1780_write(bh1780, BH1780_REG_CONTROL,
BH1780_POFF);
```c
if (ret < 0) {
Adding power management capabilities to device drivers 479 dev_err(&client->dev, “failed to power off\n”);
return ret;
}
return 0;
}
```
The first runtime PM method invoked here is pm_runtime_get_sync(). This call gets us guessing that the device is going to be used, that is, the driver needs to access the hardware. Thus, this helper immediately resumes the device (it actually increments the device usage counter and carries out a synchronous resumption of the device). After this,
```c
pm_runtime_put_noidle() is called in order to de-increment the device usage count without carrying an idle notification. Next, pm_runtime_disable() is called in order to disable runtime PM on the device. This will increment power.disable_depth for the device and if it was zero previously, cancel all pending runtime PM requests for the device and wait for all operations in progress to complete, so that with regard to the PM
```
core, the device no longer exists (remember, power.disable_depth will not match what the PM core expects, meaning that any further runtime PM helper invoked on this device will fail). Finally, the device is powered off thanks to an i2c command, after which its hardware status will reflect its runtime PM status.
The following are general rules that apply to runtime PM callback and execution:
```c
• ->runtime_idle() and ->runtime_suspend() can only be executed for active devices (those whose status is active).
• ->runtime_idle() and ->runtime_suspend() can only be executed for a device with the usage counter equal to zero and either with the counter of active children equal to zero, or with the power.ignore_children flag set.
• ->runtime_resume() can only be executed for suspended devices (those whose status is suspended).
```
Additionally, the helper functions provided by the PM core obey the following rules:
```c
• If ->runtime_suspend() is about to be executed or there’s a pending request to execute it, ->runtime_idle() will not be executed for the same device.
• A request to execute or to schedule the execution of ->runtime_suspend() will cancel any pending requests to execute ->runtime_idle() for the same device.
```
480 Linux Kernel Power Management
```c
• If ->runtime_resume() is about to be executed or there’s a pending request to execute it, the other callbacks will not be executed for the same device.
• A request to execute ->runtime_resume() will cancel any pending or scheduled requests to execute the other callbacks for the same device, except for scheduled autosuspends.
```
The preceding rules are good indicators of reasons why any invocation of these callbacks may fail. From these, we can also observe that a resumption, or a request to resume,
outperforms any other callback or request.
## The concept of power domain
Technically, a power domain is a set of devices sharing power resources (for example,
clocks or power planes). From the kernel’s perspective, a power domain is a set of devices whose power management uses the same set of callbacks with common PM data at the subsystem level. From the hardware perspective, a power domain is a hardware concept for managing devices whose power voltages are correlated; for example, the video core IP
sharing a power rail with the display IP.
Because of SoC designs being more complex, an abstraction method needed to be found so that drivers remain as generic as possible; then, genpd came out. This stands for
Generic Power Domain. It is a Linux Kernel abstraction that extends per-device runtime power management to a group of devices sharing power rails. Moreover, power domains are defined as part of a device tree in which relationships between devices and power controllers are described. This allows power domains to be redesigned on the fly and drivers to adapt without having to reboot the whole system or rebuild a new kernel.
It is designed so that if a power domain object exists for a device, its PM callbacks take precedence over the bus type (or device class or type) callback. Generic documentation on this is available in Documentation/devicetree/bindings/power/power_
domain.txt in kernel sources, and documentation related to your SoC can be found in the same directory.
## System suspend and resume sequences
The introduction of the struct dev_pm_ops data structure has somehow facilitated the understanding of the steps and actions performed by the PM core during a suspension or resumption phase, which can be summarized as follows:
“prepare —> Suspend —> suspend_late —> suspend_noirq”
|---------- Wakeup ----------|
```c
“resume_noirq —> resume_early —> resume -> complete”
```
Adding power management capabilities to device drivers 481
The preceding is the full system PM chain, as enumerated in enum suspend_stat_
step, defined in include/linux/suspend.h. This flow should remind you of the struct dev_pm_ops data structure.
In the Linux kernel code, enter_state() is the function invoked by the system power management core to enter a system sleep state. Let’s now spend some time on what really goes on during system suspension and resumption.
## Suspend stages
The following are the steps that enter_state() goes through when suspended:
1. It first invokes sync() on the filesystem (see ksys_sync()) if the CONFIG_
SUSPEND_SKIP_SYNC kernel configuration option is not set.
2. It invokes suspend notifiers (while the user space is still there). Refer to register_pm_notifier(), which is the helper used for their registration.
3. It freezes tasks (see suspend_freeze_processes()), which freezes the user space as well as kernel threads. This step is skipped if CONFIG_SUSPEND_
FREEZER is not set in a kernel configuration.
4. Devices are suspended by invoking every .suspend() callbacks registered by drivers. This is the first phase of suspending (see suspend_devices_and_
enter()).
5. It disables device interrupts (see suspend_device_irqs()). This prevents device drivers from receiving interrupts.
6. Then, the second phase of suspending devices happens (.suspend_noirq callbacks are invoked). This step is known as the noirq stage.
7. It disables non-boot CPUs (using a CPU hotplug). The CPU scheduler is told not to schedule anything on those CPUs before they go offline (see disable_nonboot_
cpus()).
8. It turns interrupts off.
9. It executes system core callbacks (see syscore_suspend()).
10. It puts the system to sleep.
This is a rough description of the actions performed before the system goes to sleep. The behavior of certain actions may vary slightly according to the sleep state the system is going to enter.
482 Linux Kernel Power Management
## Resume stages
Once a system is suspended (however deep it is), once a wakeup event occurs, the system needs to resume. The following are the steps and actions the PM core performs in order to wake up the system:
1. (Wakeup signal.)
2. Run the CPU’s wakeup code.
3. Execute system core callbacks.
4. Turn the interrupts on.
5. Enable non-boot CPUs (using the CPU hotplug).
6. The first phase of resuming devices (.resume_noirq() callbacks).
7. Enable device interrupts.
8. The second phase of suspending devices (.resume() callbacks).
9. Thaw tasks.
10. Call notifiers (when the user space is back).
I will let you discover in the PM code which functions are invoked at each step of the resumption process. From within the driver, however, these steps are all transparent. The only thing the driver needs to do is to fill struct dev_pm_ops with the appropriate callbacks according to the steps it wishes to be involved in, as we will see in the next section.
## Implementing system sleep capability
System sleep and runtime PM are different things, though they are related to one another.
There are cases where, by doing it in different ways, they bring the system to the same physical state. Thus, it is generally not a good idea to replace one with the other.
We have seen how device drivers participate in the system sleep by populating some callbacks in the struct dev_pm_ops data structure according to the sleep state they need to participate in. Commonly provided callbacks, irrespective of the sleep state, are
.suspend, .resume, .freeze, .thaw, .poweroff, and .restore. They are quite generic callbacks and are defined as follows:
• .suspend: This is executed before the system is put into a sleep state in which the contents of the main memory are preserved.
Adding power management capabilities to device drivers 483
• .resume: This callback is invoked after waking the system up from a sleep state in which the contents of the main memory were preserved, and the state of the device at the time this callback is run depends on the platform and subsystem the device belongs to.
• .freeze: Hibernation-specific, this callback is executed before creating a hibernation image. It’s analogous to .suspend, but it should not enable the device to signal wakeup events or change its power state. Most device drivers implementing this callback only have to save the device setting in memory so that it can be used back during subsequent .resume from hibernation.
• .thaw: This callback is hibernation-specific, and it is executed after creating a hibernation image OR if the creation of an image has failed. It is also executed after a failed attempt to restore the contents of main memory from such an image.
It must undo the changes made by the preceding .freeze in order to make the device operate in the same way as immediately prior to the call to .freeze.
• .poweroff: Also hibernation-specific, this is executed after saving a hibernation image. It’s analogous to .suspend, but it need not save the device’s settings in memory.
• .restore: This is the last hibernation-specific callback, which is executed after restoring the contents of the main memory from a hibernation image. It’s analogous to .resume.
Most of the preceding callbacks are quite similar or perform roughly similar operations.
```c
While the .resume, .thaw, and .restore trio may perform similar tasks, the same is true for the other trio – ->suspend, ->freeze, and ->poweroff. Thus, in order to improve code readability or facilitate callback population, the PM core provides the
SET_SYSTEM_SLEEP_PM_OPS macro, which takes suspend and resume functions and populates system-related PM callbacks as follows:
#define SET_SYSTEM_SLEEP_PM_OPS(suspend_fn, resume_fn) \
```
.suspend = suspend_fn, \
.resume = resume_fn, \
.freeze = suspend_fn, \
.thaw = resume_fn, \
.poweroff = suspend_fn, \
.restore = resume_fn,
484 Linux Kernel Power Management
The same is true for _noirq()-related callbacks. In case the driver only needs to participate in the noirq phase of the system suspend, the SET_NOIRQ_SYSTEM_
SLEEP_PM_OPS macro can be used in order to automatically populate _noirq()-
related callbacks in the struct dev_pm_ops data structure. The following is a definition of the macro:
```c
#define SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(suspend_fn, resume_fn) \
```
.suspend_noirq = suspend_fn, \
.resume_noirq = resume_fn, \
.freeze_noirq = suspend_fn, \
.thaw_noirq = resume_fn, \
.poweroff_noirq = suspend_fn, \
.restore_noirq = resume_fn,
The preceding macro takes only two parameters, which represent, as in the former macro,
the suspend and resume callbacks, but for the noirq phase this time. You should remember that such callbacks are invoked with IRQs disabled on the system.
```c
Finally, there is the SET_LATE_SYSTEM_SLEEP_PM_OPS macro, which will point ->
suspend_late, -> freeze_late, and -> poweroff_late to the same function,
and vice versa for ->resume_early, ->thaw_early, and ->restore_early:
#define SET_LATE_SYSTEM_SLEEP_PM_OPS(suspend_fn, resume_fn) \
```
.suspend_late = suspend_fn, \
.resume_early = resume_fn, \
.freeze_late = suspend_fn, \
.thaw_early = resume_fn, \
.poweroff_late = suspend_fn, \
.restore_early = resume_fn,
In addition to reducing the coding effort, all the preceding macros are conditioned with the #ifdef CONFIG_PM_SLEEP kernel configuration option so that they are not built if the PM is not needed. Finally, if you want to use the same suspend and resume callbacks for suspension to RAM and hibernation, you can use the following command:
```c
#define SIMPLE_DEV_PM_OPS(name, suspend_fn, resume_fn) \
const struct dev_pm_ops name = { \
SET_SYSTEM_SLEEP_PM_OPS(suspend_fn, resume_fn) \
}
```
Being a source of system wakeup 485
In the preceding snippet, name represents the name with which the device PM ops structure will be instantiated. suspend_fn and resume_fn are the callbacks to be invoked when the system is entering a suspend state or when it resumes from a sleep state.
Now that we are able to implement system sleep capabilities in our driver code, let’s see how to behave a system wakeup source, which allows the sleep state to be exited.
## Being a source of system wakeup
The PM core allows the system to be awoken following a system suspend. A device capable of system wakeup is known as a wakeup source in PM language. For a wakeup source to operate normally, it needs a so-called wakeup event, which, most of time, is assimilated to an IRQ line. In other words, a wakeup source generates wakeup events. When a wakeup source generates a wakeup event, the wakeup source is set to the activated state through the interface provided by the wakeup event’s framework. When the event processing ends,
it is set to the deactivated state. The interval between activate and deactivate indicates that the event is being processed. In this section, we will see how to make your device be a source of system wakeup in driver code.
Wakeup sources work so that when there is any wakeup event being processed in the system, suspension is not allowed. If suspension is in progress, it is terminated. The kernel abstracts wakeup sources by means of struct wakeup_source, which is also used for collecting statistics related to them. The following is a definition of this data structure in include/linux/pm_wakeup.h:
```c
struct wakeup_source {
const char *name;
struct list_head entry;
```
spinlock_t lock;
```c
struct wake_irq *wakeirq;
struct timer_list timer;
unsigned long timer_expires;
```
ktime_t total_time;
ktime_t max_time;
ktime_t last_time;
ktime_t start_prevent_time;
ktime_t prevent_sleep_time;
```c
unsigned long event_count;
unsigned long active_count;
unsigned long relax_count;
```
486 Linux Kernel Power Management unsigned long expire_count;
```c
unsigned long wakeup_count;
```
bool active:1;
bool autosleep_enabled:1;
```c
};
```
This structure is absolutely useless for you in terms of code, but studying it will help you understand what wakeup source sysfs attributes mean:
• entry is used to track all wakeup sources in a linked list.
• timer goes hand in hand with timer_expires. When a wakeup source generates a wakeup event and that event is being processed, the wakeup source is said to be active, and this prevents system suspension. After the wakeup event is processed (the system is no longer required to be active for this purpose), it returns to being inactive. Both activate and deactivate operations can be performed by the driver, or the driver can decide otherwise by specifying a timeout during activation.
This timeout will be used by the PM wakeup core to configure a timer that will automatically set the event to the inactive state after it expires. timer and timer_
expires are used for this purpose.
• total_time is the total time this wakeup source has been active. It sums up the total amount of time the wakeup source spent in the active state. It is a good indicator of the busy level and power consumption level of the device corresponding to the wakeup source.
• max_time is the longest amount of time that the wakeup source remained (or was continuously) in the active state. The longer it is, the more abnormal it is.
• last_time indicates the start time of the last time this wakeup source was active.
• start_prevent_time is the point in time when the wakeup source started to prevent the system from autosleeping.
• prevent_sleep_time is the total time this wakeup source prevented the system from autosleeping.
• event_count represents the number of events reported by the wakeup source. In other words, it indicates the number of signaled wakeup events.
Being a source of system wakeup 487
• active_count represents the number of times the wakeup source was activated.
This value may not be relevant or coherent in certain situations. For example,
when a wakeup event occurs, the wakeup source needs to be switched to the active state. However, this is not always the case because the event may occur while the wakeup source is already activated. Therefore active_count may be less than event_count, in which case, it would mean it is likely that another wakeup event was generated before the previous wakeup event was processed until the end. This reflects the business of the equipment represented by the wakeup source to some extent.
• relax_count represents the number of times the wakeup source was deactivated.
• expire_count represents the number of times the wakeup source timeout has expired.
• wakeup_count is the number of times the wakeup source has terminated the suspend process. If the wakeup source generates a wakeup event during the suspend process, the suspend process will be aborted. This variable records the number of times the wakeup source has terminated the suspend process. This may be a good indicator for checking whether you established that the system always fails to suspend.
• active represents the activated state of the wakeup source.
• autosleep_enabled, for me, records the state of the system’s autosleep status,
whether it is enabled or not.
In order for a device to be a wakeup source, its driver must call device_init_
```c
wakeup(). This function sets the device’s power.can_wakeup flag (so that the device_can_wakeup() helper returns the current device’s capability of being a wakeup source) and adds its wakeup-related attributes to sysfs. Additionally, it creates a wakeup source object, registers it, and attaches it to the device (dev->power.wakeup).
```
However, device_init_wakeup() only turns the device into a wakeup-capable device without assigning a wakeup event to it.
Important note
Note that only devices with wake-up capability will have a power directory in sysfs to provide all wakeup information.
488 Linux Kernel Power Management
In order to assign a wakeup event, the driver must call enable_irq_wake(), giving as a parameter the IRQ line that will be used as a wakeup event. What enable_irq_
wake() does may be platform-specific (Among other things it invokes the irq_chip.
irq_set_wake callback exposed by the underlying irqchip driver). In addition to turning on the platform logic for handling the given IRQ as a system wakeup interrupt line, it instructs suspend_device_irqs() (which is invoked on the system suspend path: refer to the Suspend stages section, step 5) to treat the given IRQ differently. As a result, the IRQ will remain enabled for the next interrupt, after which it will be disabled,
marked as pending, and suspended so that it will be re-enabled by resume_device_
```c
irqs() during the subsequent system resumption. This makes the driver’s ->suspend method the right place to invoke enable_irq_wake(), so that the wakeup event is always rearmed at the right moment. On the other hand, the driver’s ->resume callback is the right place for invoking disable_irq_wake(), which would turn off that platform configuration for the system wakeup capability of the IRQ.
While the device’s capability of being a wakeup source is a matter of hardware, whether or not a wakeup-capable device should issue wakeup events is a policy decision and is managed by the user space through a sysfs attribute, /sys/devices/.../power/
```
wakeup. This file allows the user space to check or decide whether the device (through its wakeup event) is enabled to wake up the system from sleep states. This file can be read and written to. When read, either enabled or disabled can be returned. If enabled is returned, this would mean the device is able to issue the events; if instead disabled is returned, this would mean the device is not able to do so. Writing enabled or disabled strings to it will indicate whether or not, respectively, the device is supposed to signal system wakeup (the kernel device_may_wakeup() helper will return true or false, respectively). Do note that this file is not present for the devices that are not capable of generating system wakeup events.
Let’s see in an example how drivers make use of the wakeup capability of the device.
The following is an excerpt of the i.MX6 SNVS powerkey driver, in drivers/input/
keyboard/snvs_pwrkey.c:
```c
static int imx_snvs_pwrkey_probe(struct platform_device *pdev)
{
```
[...]
```c
error = devm_request_irq(&pdev->dev, pdata->irq,
imx_snvs_pwrkey_interrupt, 0, pdev->name, pdev);
pdata->wakeup = of_property_read_bool(np, “wakeup-source”);
```
[...]
```c
device_init_wakeup(&pdev->dev, pdata->wakeup);
return 0;
```
Being a source of system wakeup 489
```c
}
static int maybe_unused imx_snvs_pwrkey_suspend(struct device *dev)
{
```
[...]
```c
if (device_may_wakeup(&pdev->dev))
enable_irq_wake(pdata->irq);
return 0;
}
static int maybe_unused imx_snvs_pwrkey_resume(struct device *dev)
{
```
[...]
```c
if (device_may_wakeup(&pdev->dev))
disable_irq_wake(pdata->irq);
return 0;
}
```
In the preceding code excerpt, from top to bottom, we have the driver probe method,
```c
which first enables the device wakeup capability using the device_init_wakeup()
function. Then, in the PM resume callback, it checks whether the device is allowed to issue a wakeup signal thanks to the device_may_wakeup() helper, prior to enabling the wakeup event by calling enable_irq_wake(), with the associated IRQ number as a parameter. The reason for using device_may_wakeup() for conditioning wakeup event enabling/disabling is because the user space may have changed the wakeup policy for this device (thanks to the /sys/devices/.../power/wakeup sysfs file), in which case this helper will return the current enabled/disabled status. This helper enables coherence with the user space decision. The same is true for the resume method, which does the same checks prior to disabling the wakeup event’s IRQ line.
```
Next, at the bottom of the driver code, we can see the following:
```c
static SIMPLE_DEV_PM_OPS(imx_snvs_pwrkey_pm_ops,
```
imx_snvs_pwrkey_suspend,
imx_snvs_pwrkey_resume);
```c
static struct platform_driver imx_snvs_pwrkey_driver = {
.driver = {
```
490 Linux Kernel Power Management
.name = “snvs_pwrkey”,
.pm = &imx_snvs_pwrkey_pm_ops,
.of_match_table = imx_snvs_pwrkey_ids,
```c
},
```
.probe = imx_snvs_pwrkey_probe,
```c
};
```
The preceding shows the usage of the famous SIMPLE_DEV_PM_OPS macro, which means the same suspend callback (that is, imx_snvs_pwrkey_suspend) will be used for Suspend-to-RAM or hibernation sleep states, and the same resume callback (imx_
snvs_pwrkey_resume actually) will be used to resume from these states. The device
PM structure is named imx_snvs_pwrkey_pm_ops as we can see in the macro, and fed to the driver later. Populating PM ops is as simple as that.
Before ending this section, let’s pay attention to the IRQ handler in this device driver:
```c
static irqreturn_t imx_snvs_pwrkey_interrupt(int irq,
void *dev_id)
{
struct platform_device *pdev = dev_id;
struct pwrkey_drv_data *pdata = platform_get_drvdata(pdev);
pm_wakeup_event(pdata->input->dev.parent, 0);
```
[...]
```c
return IRQ_HANDLED;
}
```
The key function here is pm_wakeup_event(). Roughly speaking, it reports a wakeup event. Additionally, this will halt the current system state transition. For example, on the suspend path, it will abort the suspend operation and prevent the system from going to sleep. The following is the prototype of this function:
```c
void pm_wakeup_event(struct device *dev, unsigned int msec)
```
The first parameter is the device to which the wakeup source belongs, and msec, the second parameter, is the number of milliseconds to wait before the wakeup source is automatically switched to an inactive state by the PM wakeup core. If msec equals 0, then the wakeup source is immediately disabled after the event has been reported. If msec is different from 0, then the wakeup source deactivation is scheduled msec milliseconds later in the future.
Being a source of system wakeup 491
This is where the wakeup source’s timer and timer_expires field are used. Roughly speaking, wakeup event reporting consists of the following steps:
• It increments the wakeup source’s event_count counter and increments the wakeup source’s wakeup_count, which is the number of times the wakeup source might abort the suspend operation.
• If the wakeup source is not yet active (the following are the steps performed on the activation path):
– It marks the wakeup source as active and increments the wakeup source’s active_count element.
– It updates the wakeup source’s last_time field to the current time.
– It updates the wakeup source’s start_prevent_time field if the other field,
autosleep_enabled, is true.
Then, wakeup source deactivation consists of the following steps:
• It sets the wakeup source’s active field to false.
• It updates the wakeup source’s total_time field by adding the time spent in the active state to its old value.
• It updates the wakeup source’s max_time field with the duration spent in the active state if this duration is greater than the value of the old max_time field.
• It updates the wakeup source’s last_time field with the current time, deletes the wakeup source’s timer, and clears timer_expires.
• It updates the wakeup source’s prevent_sleep_time field if the other field,
prevent_sleep_time, is true.
Deactivation may occur either immediately if msec == 0, or scheduled msec milliseconds later in the future if different to zero. All the this should remind you of struct wakeup_source, which we introduced earlier, most of whose elements are updated by this function call. The IRQ handler is a good place for invoking it because the interrupt triggering also marks the wakeup event. You should also note that each property of any wakeup source can be inspected from the sysfs interface, as we will see in the next section.
492 Linux Kernel Power Management
## Wakeup source and sysfs (or debugfs)
There is something else that needs to be mentioned here, at least for debugging purposes.
The whole list of wakeup sources in the system can be listed by printing the content of /
sys/kernel/debug/wakeup_sources (assuming debugfs is mounted on the system):
```bash
# cat /sys/kernel/debug/wakeup_sources
```
This file also reports the statistics for each wakeup source, which may be gathered individually thanks to the device’s power-related sysfs attributes. Some of these sysfs file attributes are as follows:
```c
#ls /sys/devices/.../power/wake*
```
wakeup wakeup_active_count wakeup_last_time_ms autosuspend_
delay_ms wakeup_abort_count wakeup_count wakeup_max_time_ms wakeup_
active wakeup_expire_count wakeup_total_time_ms
I used the wake* pattern in order to filter out runtime PM-related attributes, which are also in this same directory. Instead of describing what each attribute is, it would be more worthwhile indicating in which fields in the struct wakeup_source structure the preceding attributes are mapped:
• wakeup is an RW attribute and has already been described earlier. Its content determines the return value of the device_may_wakeup() helper. Only this attribute is both readable and writable. The others here are all read-only.
```c
• wakeup_abort_count and wakeup_count are read-only attributes that point to the same field, that is, wakeup->wakeup_count.
• The wakeup_expire_count attribute is mapped to the wakeup->expire_
```
count field.
```c
• wakeup_active is read-only and mapped to the wakeup->active element.
```
• wakeup_total_time_ms is a read-only attribute that returns the wakeup-
>total_time value, and its unit is ms.
```c
• wakeup_max_time_ms returns the power.wakeup->max_time value in ms.
```
• wakeup_last_time_ms, a read-only attribute, corresponds to the wakeup-
>last_time value; the unit is ms.
```c
• wakeup_prevent_sleep_time_ms is also read-only and is mapped onto the wakeup ->prevent_sleep_time value, whose unit is ms.
```
Summary 493
Not all devices are wakeup capable, but those that are can roughly follow this guideline.
Now that we are done and familiar with wakeup source management from sysfs, we can introduce the special IRQF_NO_SUSPEND flag, which assists in helps in preventing an
IRQ from being disabled in the system suspend path.
## The IRQF_NO_SUSPEND flag
There are interrupts that need to be able to trigger even during the entire system suspendresume cycle, including the noirq phases of suspending and resuming devices, as well as during the time when non-boot CPUs are taken offline and brought back online. This is the case for timer interrupts, for example. This flag has to be set on such interrupts.
Although this flag helps to keep the interrupt enabled during the suspend phase, it does not guarantee that the IRQ will wake the system from a suspended state – for such cases, it is necessary to use enable_irq_wake(), which once again, is platform-specific. Thus,
you should not confuse or mix the usage of the IRQF_NO_SUSPEND flag and enable_
irq_wake().
If an IRQ with this flag is shared by several users, every user will be affected, not just the one that has set the flag. In other words, every handler registered with the interrupt will be invoked as usual, even after suspend_device_irqs(). This is probably not what you need. For this reason, you should avoid mixing IRQF_NO_SUSPEND and IRQF_SHARED
flags.
## Summary
In this chapter, we have learned to manage the power consumption of the system, both from within the code in the driver as from the user space with the command line), either at runtime by acting on individual devices, or by acting on the whole system by playing with sleep states. We have also learned how other frameworks can help to reduce the power consumption of the system (such as CPUFreq, Thermal, and CPUIdle).
In the next chapter, we will move onto PCI device drivers, which deal with the devices sitting on this famous bus that needs no introduction