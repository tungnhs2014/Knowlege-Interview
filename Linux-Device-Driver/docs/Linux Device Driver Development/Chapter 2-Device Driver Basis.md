```bash
# Chapter 2 - Device Driver Basis 

A driver is a piece of software whose aim is to control and manage a particular hardware
```
device, hence the name device driver. From an operating system point of view, drivers can
be either in the kernel space (running in privileged mode) or in the user space (with lower
privileges). This book only deals with kernel space drivers, especially Linux kernel drivers.
Our definition is that a device driver exposes the functionality of the hardware to user
programs.
This book's aim is not to teach you how to become a Linux guru—I'm not even one at
all—but there are some concepts you should understand prior to writing a device driver. C
programming skills are mandatory; you should at least be familiar with pointers. You
should also be familiar with some manipulating functions. Some hardware skills are
required too. So, this chapter essentially discusses:
Module building processes, as well as their loading and unloading
Driver skeletons and debugging message management
Error handling in the driver
## User space and kernel space
The concepts of kernel space and user space are a bit abstract. It is all about memory and
access rights. One may consider the kernel to be privileged, whereas user apps are
restricted. This is a feature of a modern CPU, allowing it to operate either in privileged or
unprivileged mode. This concept will be clearer to you in Chapter 11, Kernel Memory
- `Management:
`## User space and kernel space
The preceding diagram introduces the separation between kernel and user space, and
highlights the fact that system calls represent a bridge between them (we discuss this later
in this chapter). We can describe each space as follows:
Kernel space: This is a set of addresses where the kernel is hosted and where it
runs. Kernel memory (or kernel space) is a memory range, owned by the kernel,
protected by access flags, preventing any user apps from messing with the kernel
(un)knowingly. On the other hand, the kernel can access the whole system
memory, since it runs with the highest priority on the system. In kernel mode,
the CPU can access the whole memory (both kernel space and user space).
User space: This is a set of addresses (locations) where normal programs (such as
gedit and so on) are restricted to run. You may consider it a sandbox or a jail, so
that a user program can't mess with memory or any other resource owned by
another program. In user mode, the CPU can only access memory tagged with
user space access rights. The only way for a user app to run in the kernel space is
through system calls. Examples of these are read, write, open, close, mmap,
and so on. User space code runs with a lower priority. When a process performs
a system call, a software interrupt is sent to the kernel, which turns on privileged
mode so that the process can run in kernel space. When the system call returns,
the kernel turns off the privileged mode and the process is jailed again.
## The concept of modules
A module is to the Linux kernel what a plugin (add-on) is to user software (Firefox is an
example). It dynamically extends the kernel functionalities without the need to even restart
the computer. Most of the time, kernel modules are plug and play. Once inserted, they are
ready to be used. In order to support modules, the kernel must have been built with the
following option enabled:
```bash
CONFIG_MODULES=y
```
## Module dependencies
In Linux, a module can provide functions or variables, exporting them using the
EXPORT_SYMBOL macro, which makes them available for other modules. These are called
symbols. A dependency of module B on module A means that module B is using one of the
symbols exported by module A.
## depmod utility
depmod is a tool that you run during the kernel build process to generate module
dependency files. It does that by reading each module in
/lib/modules/<kernel_release>/ to determine what symbols it should export and
what symbols it needs. The result of that process is written to the modules.dep file and its
binary version modules.dep.bin. It is a kind of module indexing.
## Module loading and unloading
For a module to be operational, you should load it into the kernel, either by using insmod
given the module path as an argument, which is the preferred method during
development, or by using modprobe, a clever command and one preferred in production
systems.
## Manual loading
Manual loading needs the intervention of a user, who should have root access. The two
classic methods to achieve this are described as follows.
## modprobe and insmod
During development, you usually use insmod to load a module and it should be given the
path of the module to load:
```bash
insmod /path/to/mydrv.ko
```
It is low-level form of module loading, that forms the base of other module loading
methods, and is the one we will use in this book. On the other hand, there is modprobe,
mostly used by sysadmins or in a production system. modprobe is a clever command that
parses the modules.dep file in order to load dependencies first, prior to loading the given
module. It automatically handles module dependencies, as a package manager does:
```bash
modprobe mydrv
```
Whether you can use modprobe or not depends on depmod being aware of module
installation.
/etc/modules-load.d/<filename>.conf
If you want a module to be loaded at boot time, just create the file /etc/modulesload.d/<filename>.conf, and add the module's name that should be loaded, one per
line. <filename> should be meaningful to you, and people usually use modules:
/etc/modules-load.d/modules.conf. You may create as many .conf files as you need:
An example of /etc/modules-load.d/mymodules.conf is as follows:
#this line is a comment
uio
iwlwifi
## Auto-loading
The depmod utility doesn't only build modules.dep and modules.dep.bin files. It does
more than that. When a kernel developer actually writes a driver, they know exactly what
hardware the driver will support. They are then responsible for feeding the driver with the
product and vendor IDs of all devices supported by the driver. depmod also processes
module files in order to extract and gather that information, and generates a
modules.alias file, located in /lib/modules/<kernel_release>/modules.alias,
which will map devices to their drivers.
An excerpt from modules.alias is as follows:
alias usb:v0403pFF1Cd*dc*dsc*dp*ic*isc*ip*in* ftdi_sio
alias usb:v0403pFF18d*dc*dsc*dp*ic*isc*ip*in* ftdi_sio
alias usb:v0403pDAFFd*dc*dsc*dp*ic*isc*ip*in* ftdi_sio
alias usb:v0403pDAFEd*dc*dsc*dp*ic*isc*ip*in* ftdi_sio
alias usb:v0403pDAFDd*dc*dsc*dp*ic*isc*ip*in* ftdi_sio
alias usb:v0403pDAFCd*dc*dsc*dp*ic*isc*ip*in* ftdi_sio
alias usb:v0D8Cp0103d*dc*dsc*dp*ic*isc*ip*in* snd_usb_audio
alias usb:v*p*d*dc*dsc*dp*ic01isc03ip*in* snd_usb_audio
alias usb:v200Cp100Bd*dc*dsc*dp*ic*isc*ip*in* snd_usb_au
In this step, you'll need a user space hot-plug agent (or device manager), usually udev (or
mdev) that will register with the kernel in order to get notified when a new device appears.
The notification is done by the kernel, sending the device's description (pid, vid, class,
device class, device subclass, interface, and all other information that may identify a device)
to the hot-plug daemon, which in turn calls modprobe with this information. modprobe
then parses the modules.alias file in order to match the driver associated with the device.
Before loading the module, modprobe will look for its dependencies in module.dep. If it
finds any, the dependencies will be loaded prior to the associated module loading;
otherwise, the module is loaded directly.
## Module unload
The usual command to unload a module is rmmod. You should prefer using this to unload a
module loaded with the insmod command. The command should be given the module
name to unload as a parameter. Module unloading is a kernel feature that you can enable or
disable, according to the value of the CONFIG_MODULE_UNLOAD config option. Without this
option, you will not be able to unload any modules. Let's enable module unloading
- `support:
`CONFIG_MODULE_UNLOAD=y
At runtime, the kernel will prevent you from unloading modules that may break things,
even if you ask it to do so. This is because the kernel keeps a reference count on module
usage, so that it knows whether a module is actually in use or not. If the kernel believes it is
unsafe to remove a module, it will not. Obviously, you can change this behavior:
```c
MODULE_FORCE_UNLOAD=y
```
The preceding option should be set in the kernel config in order to force unload a module:
```bash
rmmod -f mymodule
```
On the other hand, a higher-level command to unload a module in a smart manner is
modeprobe -r, which automatically unloads unused dependencies:
modeprobe -r mymodule
As you may have guessed, this is a really helpful option for developers. Finally, you can
check whether a module is loaded or not with the following command:
```bash
lsmod
```
## Driver skeletons
Let's consider the following helloworld module. It will be the basis for our work during
the rest of this chapter.
The helloworld.c file is as follows:
```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
static int __init helloworld_init(void) {
pr_info("Hello world!\n");
return 0;
}
static void __exit helloworld_exit(void) {
pr_info("End of the world\n");
}
module_init(helloworld_init);
module_exit(helloworld_exit);
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_LICENSE("GPL");
```
## Module entry and exit point
Kernel drivers all have entry and exit points: the former correspond to the function called
when the module is loaded (modprobe, insmod) and the latter are the function executed at
module unloading (at rmmod or modprobe -r).
We all remember the main() function, which is the entry point for every user space
program written in C/C++; it exits when that same function returns. With kernel modules,
things are different. The entry point can have any name you want, and unlike a user space
program that exits when main() returns, the exit point is defined in another function. All
you need to do is to inform the kernel which functions should be executed as an entry or
exit point. The actual functions helloworld_init and helloworld_exit could be given
any name. The only thing that is actually mandatory is to identify them as the
corresponding loading and removing functions, giving them as parameters to the
```c
module_init() and module_exit() macros.
```
To sum up, module_init() is used to declare the function that should be called when the
module is loaded (with insmod or modprobe). What is done in the initialization function
will define the behavior of the module. module_exit() is used to declare the function that
should be called when the module is unloaded (with rmmod).
Either the init function or the exit function is run once, right after the
module is loaded or unloaded.
## __init and __exit attributes
__init and __exit are actually kernel macros, defined in include/linux/init.h,
shown as follows:
```c
#define __init __section(.init.text)
#define __exit __section(.exit.text)
```
The __init keyword tells the linker to place the code in a dedicated section into the kernel
object file. This section is known in advance to the kernel, and freed when the module is
loaded and the init function finished. This applies only to built-in drivers, not to loadable
modules. The kernel will run the init function of the driver for the first time during its
boot sequence.
Since the driver cannot be unloaded, its init function will not be called again until the next
reboot. There is no need to keep references on its init function anymore. The same goes for
the __exit keyword, whose corresponding code is omitted when the module is compiled
statically into the kernel or when module unloading support is not enabled because, in both
cases, the exit function is never called. __exit has no effect on loadable modules.
Let's spend more time understanding how such attributes work. It is all about object files
called Executable and Linkable Format (ELF). An ELF object file is made up of various
named sections. Some of these are mandatory and form the basis of the ELF standard, but
you can make up any section you wants and have it used by special programs. This is what
the kernel does. You can run objdump -h module.ko in order to print out different
sections that constitute the given module.ko kernel module:
List of sections of helloworld-params.ko module
Only a few of the sections in the caption are standard ELF sections:
.text, also called code, which contains program code
.data, which contains initialized data, and is also called the data segment
.rodata, for read-only data
.comment
Uninitialized data segment, also called block started by symbol (bss)
Other sections are added on demand for the kernel's purposes. The most important for this
chapter are .modeinfo sections, which store information about the modules, and
.init.text sections, which store code prefixed with the __init macro.
The linker (ld on Linux systems), which is a part of binutils, is responsible for the
placement of symbols (data, code, and so on) in the appropriate section in the generated
binary in order to be processed by the loader when the program is executed. You may
customize these sections, change their default location, or even add additional sections by
providing a linker script, called a linker definition file (LDF) or linker definition script
(LDS). Now, all you have to do is to inform the linker of the symbol placement through
compiler directives. The GNU C compiler provides attributes for that purpose. In the case
of the Linux kernel, there is a custom LDS file provided, located in
arch/<arch>/kernel/vmlinux.lds.S. __init and __exit are then used to mark
symbols to be placed onto dedicated sections mapped in the kernel's LDS files.
In conclusion, __init and __exit are Linux directives (actually macros), which wrap the
C compiler attribute used for symbol placement. They instruct the compiler to put the code
they prefix respectively in the .init.text and .exit.text sections, even though the
kernel can access different object sections.
## Module information
Even without having to read its code, you should be able to gather some information (for
example, the author(s), parameter(s) description, license) about a given module. A kernel
module uses its .modinfo section to store information about the module. Any MODULE_*
macro will update the content of that section with the values passed as parameters. Some of
these macros are MODULE_DESCRIPTION(), MODULE_AUTHOR(), and MODULE_LICENSE().
The real underlying macro provided by the kernel to add an entry in the module info
section is MODULE_INFO(tag, info), which adds generic info in the form tag = info.
This means a driver author could add any free form info he/she wants, such as the
- `following:
`MODULE_INFO(my_field_name, "What easy value");
You can dump the contents of the .modeinfo section of a kernel module using the
objdump -d -j .modinfo command on the given module:
Content of .modeinfo section of the helloworld-params.ko module
The modinfo section can be seen as the data sheet of the module. The user space tool that
actually prints information in a stylized manner is modinfo:
modinfo output
Apart from the custom info you define, there is standard info you should provide, and
which the kernel provides macros for; these are license, module author, parameter
description, module version, and module description.
## Licensing
The license is defined in a given module by the MODULE_LICENSE() macro:
```c
MODULE_LICENSE ("GPL");
```
The license will define how your source code should be shared (or not) with other
developers. MODULE_LICENSE() tells the kernel what license our module is under. It has an
effect on your module behavior, since a non GPL-compatible license will result in your
module not being able to see/use services/functions exported by the kernel through the
EXPORT_SYMBOL_GPL() macro, which shows the symbols to GPL-compatible modules.
This is the opposite of EXPORT_SYMBOL(), which exports functions for modules with any
license. Loading a non GPL-compatible module will also result in a tainted kernel; that
means a non-open source or untrusted code has been loaded, and you will likely have no
support from the community. Remember that the module without MODULE_LICENSE() is
not considered open source and will taint the kernel too. The following is an excerpt of
include/linux/module.h, describing the license supported by the kernel:
/*
* The following license indents are currently accepted as indicating free
* software modules
*
* "GPL" [GNU Public License v2 or later]
* "GPL v2" [GNU Public License v2]
* "GPL and additional rights" [GNU Public License v2 rights and more]
* "Dual BSD/GPL" [GNU Public License v2
* or BSD license choice]
* "Dual MIT/GPL" [GNU Public License v2
* or MIT license choice]
* "Dual MPL/GPL" [GNU Public License v2
* or Mozilla license choice]
*
* The following other idents are available
*
* "Proprietary" [Non free products]
*
* There are dual licensed components, but when running with Linux it is
the
* GPL that is relevant so this is a non issue. Similarly LGPL linked with
GPL
* is a GPL combined work.
*
* This exists for several reasons
* 1. So modinfo can show license info for users wanting to vet their
setup
* is free
* 2. So the community can ignore bug reports including proprietary
modules
* 3. So vendors can do likewise based on their own policies
*/
It is mandatory for your module to be at least GPL-compatible in order for
you to enjoy full kernel services.
## Module author(s)
```c
MODULE_AUTHOR() declares the module's author(s):
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
```
It is possible to have more than one author. In this case, each author must be declared with
```c
MODULE_AUTHOR():
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_AUTHOR("Lorem Ipsum <l.ipsum@foobar.com>");
```
## Module description
```c
MODULE_DESCRIPTION() briefly describes what the module does:
MODULE_DESCRIPTION("Hello, world! Module");
```
## Errors and message printing
Error codes are interpreted either by the kernel or by the user space application (through
the errno variable). Error handling is very important in software development, more than
it is in kernel development. Fortunately, the kernel provides a couple of errors that cover
almost every error you'll encounter, and sometimes you will need to print them out in
order to help you debug.
## Error handling
Returning the wrong error code for a given error will result in either the kernel or user
space app producing unwanted behavior and making a wrong decision. To keep things
clear, there are predefined errors in the kernel tree that cover almost every case you may
face. Some of the errors (with their meanings) are defined in include/uapi/asmgeneric/errno-base.h, and the rest of the list can be found in include/uapi/asmgeneric/errno.h. The following is an excerpt from the list of errors, from
include/uapi/asm-generic/errno-base.h:
```c
#define EPERM 1 /* Operation not permitted */
#define ENOENT 2 /* No such file or directory */
#define ESRCH 3 /* No such process */
#define EINTR 4 /* Interrupted system call */
#define EIO 5 /* I/O error */
#define ENXIO 6 /* No such device or address */
#define E2BIG 7 /* Argument list too long */
#define ENOEXEC 8 /* Exec format error */
#define EBADF 9 /* Bad file number */
#define ECHILD 10 /* No child processes */
#define EAGAIN 11 /* Try again */
#define ENOMEM 12 /* Out of memory */
#define EACCES 13 /* Permission denied */
#define EFAULT 14 /* Bad address */
#define ENOTBLK 15 /* Block device required */
#define EBUSY 16 /* Device or resource busy */
#define EEXIST 17 /* File exists */
#define EXDEV 18 /* Cross-device link */
#define ENODEV 19 /* No such device */
#define ENOTDIR 20 /* Not a directory */
#define EISDIR 21 /* Is a directory */
#define EINVAL 22 /* Invalid argument */
#define ENFILE 23 /* File table overflow */
#define EMFILE 24 /* Too many open files */
#define ENOTTY 25 /* Not a typewriter */
#define ETXTBSY 26 /* Text file busy */
#define EFBIG 27 /* File too large */
#define ENOSPC 28 /* No space left on device */
#define ESPIPE 29 /* Illegal seek */
#define EROFS 30 /* Read-only file system */
#define EMLINK 31 /* Too many links */
#define EPIPE 32 /* Broken pipe */
#define EDOM 33 /* Math argument out of domain of func */
#define ERANGE 34 /* Math result not representable */
```
Most of the time, the classic way to return an error is to do so in the form return -ERROR,
especially when it comes to answering system calls. For example, for an I/O error, the error
code is EIO and you should return -EIO:
dev = init(&ptr);
```c
if(!dev)
return -EIO;
```
Errors sometimes cross the kernel space and propagate themselves to the user space. If the
returned error is an answer to a system call (open, read, ioctl, mmap), the value will be
automatically assigned to the user space errno global variable, on which you can use
strerror(errno) to translate the error into a readable string:
```c
#include <errno.h> /* to access errno global variable */
#include <string.h>
```
[...]
if(write(fd, buf, 1) < 0) {
printf("something gone wrong! %s\n", strerror(errno));
```c
}
```
[...]
When you face an error, you must undo everything that was set before the error occurred.
The usual way to do this is to use the goto statement:
ptr = kmalloc(sizeof (device_t));
```c
if(!ptr) {
```
ret = -ENOMEM;
goto err_alloc;
```c
}
```
dev = init(&ptr);
```c
if(dev) {
```
ret = -EIO
goto err_init;
```c
}
return 0;
```
- `err_init:
`free(ptr);
- `err_alloc:
`return ret;
The reason why you uses the goto statement is simple. When it comes to handling errors,
let's say in step 5, you have to clean the previous operations (steps 4, 3, 2, 1), instead of
doing a lot of nested checking operations, shown as follows:
```c
if (ops1() != ERR) {
if (ops2() != ERR) {
if ( ops3() != ERR) {
if (ops4() != ERR) {
```
This may be confusing, and may lead to indentation issues. Prefer using goto in order to
have a straight control flow, shown as follows:
```c
if (ops1() == ERR) // |
```
goto error1; // |
```c
if (ops2() == ERR) // |
```
goto error2; // |
```c
if (ops3() == ERR) // |
```
goto error3; // |
```c
if (ops4() == ERR) // V
```
goto error4;
- `error5:
`[...]
- `error4:
`[...]
- `error3:
`[...]
- `error2:
`[...]
- `error1:
`[...]
This means you should only use goto to move forward in a function.
## Handling null pointer errors
When it comes to returning an error from functions that are supposed to return a pointer,
functions often return the NULL pointer. It is a working but quite meaningless approach,
since you do not exactly know why this null pointer is returned. For that purpose, the
kernel provides three functions, ERR_PTR, IS_ERR, and PTR_ERR:
```c
void *ERR_PTR(long error);
long IS_ERR(const void *ptr);
long PTR_ERR(const void *ptr);
```
The first actually returns the error value as a pointer. Given a function that is likely to
```c
return -ENOMEM after a failed memory allocation, we have to do something such
```
as return ERR_PTR(-ENOMEM);. The second is used to check whether the returned value
is a pointer error or not, if (IS_ERR(foo)). The last returns the actual error code, return
PTR_ERR(foo);. The following is an example:
This is how to use ERR_PTR, IS_ERR, and PTR_ERR:
```c
static struct iio_dev *indiodev_setup(){
```
[...]
```c
struct iio_dev *indio_dev;
```
indio_dev = devm_iio_device_alloc(&data->client->dev, sizeof(data));
```c
if (!indio_dev)
return ERR_PTR(-ENOMEM);
```
[...]
```c
return indio_dev;
}
static int foo_probe([...]){
```
[...]
```c
struct iio_dev *my_indio_dev = indiodev_setup();
if (IS_ERR(my_indio_dev))
return PTR_ERR(data->acc_indio_dev);
```
[...]
```c
}
```
This is a plus on error handling; it is also an excerpt of the kernel coding
style that says if the name of a function is an action or an imperative
command, the function should return an error code integer. If the name is
a predicate, the function should return a succeeded Boolean. For
example, add work is a command, and the add_work() function returns
0 for success or -EBUSY for failure. In the same way, PCI device
present is a predicate, and the pci_dev_present() function returns 1
if it succeeds in finding a matching device, or 0 if it doesn't.
## Message printing – printk()
```c
printk() is to the kernel what printf() is to the user space. Lines written by printk()
```
can be displayed through the dmesg command. Depending on how important the message
you need to print is, you can choose between eight log-level messages, defined in
include/linux/kern_levels.h, along with their meaning:
The following is a list of kernel log levels. Each of these levels corresponds to a number in a
string, whose priority is inversely proportional to the value of the number. For example, 0
is higher-priority:
```c
#define KERN_SOH "\001" /* ASCII Start Of Header */
#define KERN_SOH_ASCII '\001'
#define KERN_EMERG KERN_SOH "0" /* system is unusable */
#define KERN_ALERT KERN_SOH "1" /* action must be taken immediately
```
*/
```c
#define KERN_CRIT KERN_SOH "2" /* critical conditions */
#define KERN_ERR KERN_SOH "3" /* error conditions */
#define KERN_WARNING KERN_SOH "4" /* warning conditions */
#define KERN_NOTICE KERN_SOH "5" /* normal but significant condition
```
*/
```c
#define KERN_INFO KERN_SOH "6" /* informational */
#define KERN_DEBUG KERN_SOH "7" /* debug-level messages */
```
The following code shows how you can print a kernel message along with a log level:
```c
printk(KERN_ERR "This is an error\n");
```
If you omit the debug level (printk("This is an error\n")), the kernel will provide
one to the function, depending on the CONFIG_DEFAULT_MESSAGE_LOGLEVEL config
option, which is the default kernel log level. One may actually use one of the following,
much more meaningful macros, which are wrappers around those defined previously:
```c
pr_emerg, pr_alert, pr_crit, pr_err, pr_warning, pr_notice, pr_info, and
```
- `pr_debug:
`pr_err("This is the same error\n");
For new drivers, it is recommended to use these wrappers. The reality of printk() is that
whenever it is called, the kernel compares the message log level with the current console
log level; if the former is higher (a lower value) than the latter, the message will be
immediately printed to the console. You can check your log level parameters with the
- `following:
`cat /proc/sys/kernel/printk
4 4 1 7
In this code, the first value is the current log level (4), and the second is the default one
according to the CONFIG_DEFAULT_MESSAGE_LOGLEVEL option. Other values are not
relevant for the purposes of this chapter, so let's ignore these.
A list of kernel log levels is as follows:
/* integer equivalents of KERN_<LEVEL> */
```c
#define LOGLEVEL_SCHED -2 /* Deferred messages from sched code
```
* are set to this special level */
```c
#define LOGLEVEL_DEFAULT -1 /* default (or last) loglevel */
#define LOGLEVEL_EMERG 0 /* system is unusable */
#define LOGLEVEL_ALERT 1 /* action must be taken immediately
```
*/
```c
#define LOGLEVEL_CRIT 2 /* critical conditions */
#define LOGLEVEL_ERR 3 /* error conditions */
#define LOGLEVEL_WARNING 4 /* warning conditions */
#define LOGLEVEL_NOTICE 5 /* normal but significant condition
```
*/
```c
#define LOGLEVEL_INFO 6 /* informational */
#define LOGLEVEL_DEBUG 7 /* debug-level messages */
```
The current log level can be changed with the following:
```bash
# echo <level> > /proc/sys/kernel/printk
c
printk() never blocks and is safe enough to be called even from atomic
```
contexts. It tries to lock the console and print the message. If locking fails,
the output will be written into a buffer and the function will return, never
blocking. The current console holder will then be notified about new
messages and will print them before releasing the console.
The kernel supports other debug methods too, either dynamically or by using #define
DEBUG on top of the file. People interested in such debugging style can refer to the kernel
documentation in the Documentation/dynamic-debug-howto.txt file.
## Module parameters
As a user program does, a kernel module can accept arguments from the command line.
This allows dynamically changing the behavior of the module according to the given
parameters, and can avoid the developer having to indefinitely change/compile the module
during a test/debug session. In order to set this up, you should first declare the variables
that will hold the values of command line arguments, and use the module_param() macro
on each of these. The macro is defined in include/linux/moduleparam.h (this should be
included in the code too: #include <linux/moduleparam.h>), shown as follows:
```c
module_param(name, type, perm);
```
This macro contains the following elements:
- `name: `The name of the variable used as the parameter
- `type: `The parameter's type (bool, charp, byte, short, ushort, int, uint, long,
ulong), where charp stands for char pointer
- `perm: `This represents the /sys/module/<module>/parameters/<param> file
permissions. Some of them are S_IWUSR, S_IRUSR, S_IXUSR, S_IRGRP, S_WGRP,
and S_IRUGO, where:
S_I is just a prefix
- `R: `read, W: write, X: execute
- `USR: `user, GRP: group, UGO: user, group, others
One can eventually use a | (OR operation) to set multiple permissions. If perm is 0, the file
parameter in sysfs will not be created. You should use only S_IRUGO read-only
parameters, which I highly recommend; by making a | (OR) with other properties, you can
obtain fine-grained properties.
When using module parameters, you should use MODULE_PARM_DESC in order to describe
each of them. This macro will populate the module info section with each parameter's
description. The following is a sample from the helloworld-params.c source file
provided with the code repository of the book:
```c
#include <linux/moduleparam.h>
```
[...]
```c
static char *mystr = "hello";
static int myint = 1;
static int myarr[3] = {0, 1, 2};
module_param(myint, int, S_IRUGO);
module_param(mystr, charp, S_IRUGO);
module_param_array(myarr, int,NULL, S_IWUSR|S_IRUSR); /* */
MODULE_PARM_DESC(myint,"this is my int variable");
MODULE_PARM_DESC(mystr,"this is my char pointer variable");
MODULE_PARM_DESC(myarr,"this is my array of int");
static int foo()
{
pr_info("mystring is a string: %s\n", mystr);
pr_info("Array elements: %d\t%d\t%d", myarr[0], myarr[1], myarr[2]);
return myint;
}
```
To load the module and feed our parameter, we do the following:
```bash
# insmod hellomodule-params.ko mystring="packtpub" myint=15 myArray=1,2,3
```
You could have used modinfo prior to loading the module in order to display descriptions
of parameters supported by the module:
```bash
$ modinfo ./helloworld-params.ko
```
- `filename: `/home/jma/work/tutos/sources/helloworld/./helloworld-params.ko
- `license: `GPL
- `author: `John Madieu <john.madieu@gmail.com>
- `srcversion: `BBF43E098EAB5D2E2DD78C0
- `depends:
`- `vermagic: `4.4.0-93-generic SMP mod_unload modversions
- `parm: `myint:this is my int variable (int)
- `parm: `mystr:this is my char pointer variable (charp)
- `parm: `myarr:this is my array of int (array of int)
## Building your first module
There are two places to build a module. It depends on whether you want people to enable
the module by themselves or not using the kernel config interface.
## The module's makefile
A makefile is a special file used to execute a set of actions, among which the most important
is the compilation of programs. There is a dedicated tool to parse makefiles, called make.
Prior to jumping to the description of the whole make file, let's introduce the obj-<X>
kbuild variable.
In almost every kernel makefile, you will see at least one instance of an obj-<X> variable.
This actually corresponds to the obj-<X> pattern, where <X> should be either y, m, left
blank, or n. This is used by the kernel makefile from the head of the kernel build system in
a general manner. These lines define the files to be built, any special compilation options,
and any subdirectories to be entered recursively. A simple example is:
```c
obj-y += mymodule.o
```
This tells kbuild that there is one object in the current directory named mymodule.o.
mymodule.o will be built from mymodule.c or mymodule.S. How and
whether mymodule.o will be built or linked depends on the value of <X>:
If <X> is set to m, the obj-m variable is used, and mymodule.o will be built as a
module.
If <X> is set to y, the obj-y variable is used, and mymodule.o will be built as
part of the kernel. You'd then say foo is a built-in module.
If <X> is set to n, the obj-m variable is used, and mymodule.o will not be built at
all.
Therefore, the obj-$(CONFIG_XXX) pattern is often used, where CONFIG_XXX is a kernel
```c
config option, set or not during the kernel configuration process. An example is:
obj-$(CONFIG_MYMODULE) += mymodule.o
```
$(CONFIG_MYMODULE) evaluates to either y or m according to its value during the kernel
configuration (remember make menuconfig). If CONFIG_MYMODULE is neither y nor m, then
the file will not be compiled nor linked. y means built-in (it stands for yes in the kernel
```c
config process), and m stands for module. $(CONFIG_MYMODULE) pulls the right answer
```
from the normal config process. This is explained in the next section.
The last use case is:
```c
obj-<X> += somedir/
```
This means that kbuild should go into the directory named somedir, look for any makefile
inside, and process it in order to decide what objects should be built.
Back to the makefile; the following is the content makefile we will use to build each of the
modules introduced in the book:
```c
obj-m := helloworld.o
```
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
all default: modules
- `install: `modules_install
modules modules_install help clean:
$(MAKE) -C $(KERNELDIR) M=$(shell pwd) $@
```c
obj-m := helloworld.o: obj-m lists modules we want to build. For each
```
<filename>.o, the build system will look for a <filename>.c to build. obj-m
is used to build a module, whereas obj-y will result in a built-in object.
KERNELDIR := /lib/modules/$(shell uname -r)/build: KERNELDIR is
the location of the prebuilt kernel source. As we said earlier, we need a prebuilt
kernel in order to build any module. If you have built your kernel from the
source, you should set this variable with the absolute path of the built source
directory. -C instructs the make utility to change into the specified directory
prior to reading the makefiles or doing anything else.
M=$(shell pwd): This is relevant to the kernel build system. The kernel
makefile uses this variable to locate the directory of the external module to build.
Your .c files should be placed there.
all default: modules: This line instructs the make utility to execute the
modules target, whether all or default targets, which are classic targets when
it comes to building user apps. In other words, make default or make all, or
simply make commands, will be translated into make modules.
modules modules_install help clean:: This line represents the list target
valid in this makefile.
$(MAKE) -C $(KERNELDIR ) M=$(shell pwd) $@: This is the rule to be
executed for each target enumerated previously. $@ will be replaced with the
name of the target that caused the rule to run. In other words, if you call make
modules, $@ will be replaced with modules, and the rule will become $(MAKE)
-C $(KERNELDIR ) M=$(shell pwd) module.
## In the kernel tree
Before you can build your driver in the kernel tree, you should first identify which
directory in drivers should host your .c file. Given your file name mychardev.c, which
contains the source code of your special character driver, it should be placed in the
drivers/char directory in the kernel source. Every subdirectory in drivers has both
Makefile and Kconfig files.
Add the following content to the Kconfig of that directory:
```c
config PACKT_MYCDEV
```
tristate "Our packtpub special Character driver"
default m
help
Say Y here if you want to support the /dev/mycdev device.
The /dev/mycdev device is used to access packtpub.
In the makefile of that same directory, add the following:
```c
obj-$(CONFIG_PACKT_MYCDEV) += mychardev.o
```
Be careful when updating the Makefile; the .o file name must match the exact name of
your .c file. If your source file is foobar.c, you must use foobar.o in the Makefile. In
order to have your driver built as a module, add the following line in your board defconfig
in the arch/arm/configs directory:
```bash
CONFIG_PACKT_MYCDEV=m
```
You may also run make menuconfig to select it from the UI, run make to build the kernel,
then run make modules to build modules (including yours). To make the driver built-in,
just replace m with y:
```bash
CONFIG_PACKT_MYCDEV=m
```
Everything described here is what embedded board manufacturers do in order to provide a
Board Support Package (BSP) with their board, with a kernel that already contains their
custom drivers:
packt_dev module in the kernel tree
Once configured, you can build the kernel with make and build modules with make
modules.
Modules included in the kernel source tree are installed in
/lib/modules/$(KERNELRELEASE)/kernel/. On your Linux system, it is
/lib/modules/$(uname -r)/kernel/. Run the following command in order to install
the modules:
```bash
make modules_install
```
## Out of the tree
Before you can build an external module, you need to have a complete and precompiled
kernel source tree. The kernel source tree version must be the same as the kernel you'll load
and use your module with. There are two ways to obtain a prebuilt kernel version:
```c
Build it by yourself (discussed earlier)
```
Install the linux-headers-* package from your distribution repository
```bash
sudo apt-get update
sudo apt-get install linux-headers-$(uname -r)
```
This will install only headers, not the whole source tree. Headers will then be installed in
/usr/src/linux-headers-$(uname -r). On my computer, it is /usr/src/linuxheaders-4.4.0-79-generic/. There will be a symlink, /lib/modules/$(uname -
r)/build, pointing to the previously installed headers. It is the path you should specify as
your kernel directory in your Makefile. It is all you have to do for a prebuilt kernel.
## Building the module
Now, when you are done with your makefile, just change to your source directory and run
the make command, or make modules:
jma@jma:~/work/tutos/sources/helloworld$ make
```bash
make -C /lib/modules/4.4.0-79-generic/build \
```
M=/media/jma/DATA/work/tutos/sources/helloworld modules
make[1]: Entering directory '/usr/src/linux-headers-4.4.0-79-
generic'
CC [M]
/media/jma/DATA/work/tutos/sources/helloworld/helloworld.o
Building modules, stage 2.
MODPOST 1 modules
CC
/media/jma/DATA/work/tutos/sources/helloworld/helloworld.mod.o
LD [M]
/media/jma/DATA/work/tutos/sources/helloworld/helloworld.ko
make[1]: Leaving directory '/usr/src/linux-headers-4.4.0-79-
generic'
jma@jma:~/work/tutos/sources/helloworld$ ls
helloworld.c helloworld.ko helloworld.mod.c helloworld.mod.o
helloworld.o Makefile modules.order Module.symvers
jma@jma:~/work/tutos/sources/helloworld$ sudo insmod helloworld.ko
jma@jma:~/work/tutos/sources/helloworld$ sudo rmmod helloworld
jma@jma:~/work/tutos/sources/helloworld$ dmesg
[...]
[308342.285157] Hello world!
[308372.084288] End of the world
The preceding example only dealt with native builds, compiling on an x86 machine for an
x86 machine. What about cross-compilation? This is the process by which one compiles on
machine A, called the host, code that is intended to run on machine B, called the target; the
host and target have different architectures. The classic use case is to build on an x86
machine code that should run on an ARM architecture, which is exactly our situation.
When it comes to cross-compiling a kernel module, there are essentially two variables the
kernel makefile needs to be aware of; these are ARCH and CROSS_COMPILE, which
respectively represent the target architecture and the compiler prefix name. So, what
changes between the native compilation and cross-compilation of a kernel module is the
```bash
make command. The following is the line to build for ARM:
make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf
```
## Summary
This chapter showed you the basics of driver development and explained the concept of
modules/built-in devices, as well as their loading and unloading. Even if you are not able to
interact with the user space, you are ready to write a complete driver, print a formatted
message, and understand the concept of init/exit. The next chapter will deal with
character devices, with which you will be able to target enhanced features, write code
accessible from the user space, and have a significant impact on the system