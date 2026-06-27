# Chapter 1 - Introduction to Kernel Development 

Linux started as a hobby project in 1991 for a Finnish student, Linus Torvalds. The project
has gradually grown and continues to do so, with roughly 1,000 contributors around the
world. Nowadays, Linux is a must, in embedded systems as well as on servers. A kernel is
a central part of an operating system, but its development is not so obvious.
Linux offers many advantages over other operating systems:
It is free of charge
Well-documented with a large community
It is portable across different platforms
It provides access to the source code
There is lots of free, open source software
This book tries to be as generic as possible. There is a special topic, regarding device trees,
which are not a full x86 feature yet. That topic will then be dedicated to ARM processors,
and all those fully supporting the device tree. Why those architectures? Because they are most
used on desktops and servers (for x86), and on embedded systems (ARM).
This chapter deals, among other things, with the following:
Development environment setup
Getting, configuring, and building kernel sources
Kernel source code organization
Introduction to kernel coding styleIntroduction to Kernel Development Chapter 1
[ 9 ]
## Environment setup
Before you start any development, you need to set an environment up. The environment
dedicated to Linux development is quite simple, at least on Debian-based systems:
```bash
$ sudo apt-get update
$ sudo apt-get install gawk wget git diffstat unzip texinfo \
```
gcc-multilib build-essential chrpath socat libsdl1.2-dev \
xterm ncurses-dev lzop
There are parts of the code in this book that are compatible with ARM system on chip
(SoC) solutions. You should install gcc-arm as well:
```bash
sudo apt-get install gcc-arm-linux-gnueabihf
```
I'm running Ubuntu 16.04 on an ASUS ROG, with an Intel Core i7 (eight physical cores), 16
GB of RAM, 256 GB of SSD, and 1 TB of magnetic hard drive. My favorite editor is Vim, but
you are free to use the one you are most comfortable with.
## Getting the sources
In the early kernel days (until 2003), odd-even versioning styles were used, where odd
numbers were stable and even numbers were unstable. When the 2.6 version was released,
the versioning scheme switched to X.Y.Z, where:
X: This was the actual kernel version, also called major; it incremented when
there were backwards-incompatible API changes
Y: This was the minor revision; it incremented after adding a functionality in a
backwards-compatible manner
Z: This was also called PATCH, representing the version relative to bug fixes
This is called semantic versioning, and it was used until the 2.6.39 version; when Linus
Torvalds decided to bump the version to 3.0, which also meant the end of semantic
versioning in 2011, and then an X.Y scheme was adopted.
When it came to the 3.20 version, Linus argued that he could no longer increase Y, and
decided to switch to an arbitrary versioning scheme, incrementing X whenever Y got so
large that he had run out of fingers and toes to count it. This is the reason why the version
has moved from 3.20 to 4.0 directly. Have a look
at https://plus.google.com/+LinusTorvalds/posts/jmtzzLiiejc.Introduction to Kernel Development Chapter 1
[ 10 ]
Now, the kernel uses an arbitrary X.Y versioning scheme, which has nothing to do with
semantic versioning.
## Source organization
For the needs of this book, you must use Linus Torvald's GitHub repository:
```bash
git clone https://github.com/torvalds/linux
git checkout v4.1
```
ls
- `arch/:` The Linux kernel is a fast-growing project that supports more and more
architectures. That being said, the kernel wants to be as generic as possible.
Architecture-specific code is separated from the rest, and falls into this directory.
This directory contains processor-specific subdirectories such as alpha/, arm/,
mips/, blackfin/, and so on.
- `block/:` This directory contains code for block storage devices, actually the
scheduling algorithm.
- `crypto/:` This directory contains the cryptographic API and the encryption
algorithms code.
- `Documentation/:` This should be your favorite directory. It contains the
descriptions of APIs used for different kernel frameworks and subsystems. You
should look here prior to asking any questions on forums.
- `drivers/:` This is the heaviest directory, continuously growing as device drivers
get merged. It contains every device driver organized in various subdirectories.
- `fs/:` This directory contains the implementation of different filesystems that the
kernel actually supports, such as NTFS, FAT, ETX{2,3,4}, sysfs, procfs, NFS, and
so on.
- `include/:` This contains kernel header files.
- `init/:` This directory contains the initialization and start up code.
- `ipc/:` This contains implementations of the inter-process communication (IPC)
mechanisms, such as message queues, semaphores, and shared memory.
- `kernel/:` This directory contains architecture-independent portions of the base
kernel.
- `lib/:` Library routines and some helper functions live here. They are generic
kernel object (kobject) handlers, cyclic redundancy Code (CRC) computation
functions, and so on.Introduction to Kernel Development Chapter 1
[ 11 ]
- `mm/:` This contains memory management code.
- `net/:` This contains networking (whatever network type it is) protocols code.
- `scripts/:` This contains scripts and tools used during kernel development.
There are other useful tools here.
- `security/:` This directory contains the security framework code.
- `sound/:` Audio subsystems code is here.
- `usr/:` This currently contains the initramfs implementation.
The kernel must remain portable. Any architecture-specific code should be located in the
arch directory. Of course, the kernel code related to the user space API does not change
(system calls, /proc, /sys), as it would break the existing programs.
This book deals with version 4.1 of the kernel. Therefore, any changes
made until v4.11 are covered too, at least the frameworks and
subsystemsare concerned.
## Kernel configuration
The Linux kernel is a makefile-based project, with thousands of options and drivers. To
configure your kernel, either use make menuconfig for an ncurse-based interface or make
xconfig for an X-based interface. Once chosen, options will be stored in a .config file, at
the root of the source tree.
In most cases, there will be no need to start a configuration from scratch. There are default
and useful configuration files available in each arch directory, which you can use as a
starting point:
ls arch/<you_arch>/configs/
For ARM-based CPUs, these config files are located in arch/arm/configs/, and for an
i.MX6 processor, the default file config is arch/arm/configs/imx_v6_v7_defconfig.
Similarly, for x86 processors we find the files in arch/x86/configs/, with only two
default configuration files, i386_defconfig and x86_64_defconfig, for 32- and 64-bit
versions respectively. It is quite straightforward for an x86 system:
```bash
make x86_64_defconfig
make zImage -j16
make modules
make INSTALL_MOD_PATH </where/to/install> modules_installIntroduction to Kernel Development Chapter 1
```
[ 12 ]
Given an i.MX6-based board, you can start with ARCH=arm make imx_v6_v7_defconfig,
and then ARCH=arm make menuconfig. With the former command, you will store the
default option in the .config file, and with the latter, you can update add/remove options,
depending on the need.
You may run into a Qt4 error with xconfig. In such a case, you should just use the
following command:
```bash
sudo apt-get install qt4-dev-tools qt4-qmake
```
## Building your kernel
Building the kernel requires you to specify the architecture for which it is built, as well as
the compiler. That said, it is not necessary for a native build:
```bash
ARCH=arm make imx_v6_v7_defconfig
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make zImage -j16
```
After that, you will see something like:
[...]
LZO arch/arm/boot/compressed/piggy_data
CC arch/arm/boot/compressed/misc.o
CC arch/arm/boot/compressed/decompress.o
CC arch/arm/boot/compressed/string.o
SHIPPED arch/arm/boot/compressed/hyp-stub.S
SHIPPED arch/arm/boot/compressed/lib1funcs.S
SHIPPED arch/arm/boot/compressed/ashldi3.S
SHIPPED arch/arm/boot/compressed/bswapsdi2.S
AS arch/arm/boot/compressed/hyp-stub.o
AS arch/arm/boot/compressed/lib1funcs.o
AS arch/arm/boot/compressed/ashldi3.o
AS arch/arm/boot/compressed/bswapsdi2.o
AS arch/arm/boot/compressed/piggy.o
LD arch/arm/boot/compressed/vmlinux
OBJCOPY arch/arm/boot/zImage
Kernel: arch/arm/boot/zImage is ready
From the kernel build, the result will be a single binary image located in arch/arm/boot/.
Modules are built with the following command:
```bash
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make modules
```
You can install them using the following command:
```bash
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make modules_installIntroduction to Kernel Development Chapter 1
```
[ 13 ]
The modules_install target expects an environment variable, INSTALL_MOD_PATH,
which specifies where you should install the modules. If not set, the modules will be
installed at /lib/modules/$(KERNELRELEASE)/kernel/. This is discussed in Chapter
2, Device Driver Basis.
i.MX6 processors support device trees, which are files you use to describe the hardware.
(This is discussed in detail in Chapter 6, The Concept of a Device Tree). To compile every
ARCH device tree, you can run the following command:
```bash
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make dtbs
```
However, the dtbs option is not available on all platforms that support device trees. To
build a standalone device tree blob (DTB), you should use:
```bash
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make imx6d-sabrelite.dtb
```
## Kernel habits
The kernel code tried to follow standard rules throughout its evolution. In this chapter, we
will just be introduced to them. They are all discussed in a dedicated chapter; starting from
Chapter 3, Kernel Facilities and Helper Functions, we get a better overview of the kernel
development process and tips, up to Chapter 13, Linux Device Model.
## Coding style
Before going deep into this section, you should always refer to the kernel coding style
manual, at Documentation/CodingStyle in the kernel source tree. This coding style is a
set of rules you should respect, at least if you need to get patches accepted by kernel
developers. Some of these rules concern indentation, program flow, naming conventions,
and so on.
The most popular ones are:
Always use a tab indentation of eight characters, and the line should be 80
columns long. If the indentation prevents you from writing your function, it is
because this one has too many nesting levels. One can size the tabs and verify the
line size using a scripts/cleanfile script from the kernel source:
```c
scripts/cleanfile my_module.cIntroduction to Kernel Development Chapter 1
```
[ 14 ]
You can also indent the code correctly using the indent tool:
```bash
sudo apt-get install indent
scripts/Lindent my_module.c
```
Every function/variable that is not exported should be declared as static.
No spaces should be added around (inside) parenthesised expressions. s =
sizeof (struct file); is accepted, whereas s = sizeof( struct file );
is not.
Using typedef is forbidden.
Always use /* this */ comment style, not // this:
```c
BAD: // do not use this please
GOOD: /* Kernel developers like this */
```
You should capitalise macros, but functional macros can be in lowercase.
A comment should not replace code that is not illegible. Prefer rewriting the code
rather than adding a comment.
## Kernel structure allocation/initialization
The kernel always offers two possible allocation mechanisms for its data structures and
facilities.
Some of these structures are:
Workqueue
List
Waitqueue
Tasklet
Timer
Completion
mutex
spinlock
Dynamical initializers are all macros, which means they are always capitalized:
INIT_LIST_HEAD(), DECLARE_WAIT_QUEUE_HEAD(), DECLARE_TASKLET( ), and so on.Introduction to Kernel Development Chapter 1
[ 15 ]
These are all discussed in Chapter 3, Kernel Facilities and Helper Functions. Data structures
that represent framework devices are always allocated dynamically, each having its own
allocation and deallocation API. These framework device types are:
Network
Input device
Char device
IIO device
Class
Framebuffer
Regulator
PWM device
RTC
The scope of the static objects is visible in the whole driver, and by every device this driver
manages. Dynamically allocated objects are visible only by the device that is actually using
a given instance of the module.
## Classes, objects, and OOP
The kernel implements object-oriented programming (OOP) by means of a device and a
class. Kernel subsystems are abstracted by means of classes. There are almost as many
subsystems as there are directories under /sys/class/. The struct kobject structure is
the center piece of this implementation. It even brings in a reference counter, so that the
kernel may know how many users actually use the object. Every object has a parent, and
has an entry in sysfs (if mounted).
```c
Every device that falls into a given subsystem has a pointer to an operations (ops)
```
structure, which exposes operations that can be executed on this device.
## Summary
This chapter explained in a simple manner how you should download the Linux source
and process a first build. It also dealt with some common concepts. That said, this chapter is
quite brief and may not be enough, but never mind, it is just an introduction. That is why
the next chapter gets more into the details of the kernel-building process; how to actually
compile a driver, either externally or as a part of the kernel; as well as some basics that you
should learn before starting the long journey that kernel development represents