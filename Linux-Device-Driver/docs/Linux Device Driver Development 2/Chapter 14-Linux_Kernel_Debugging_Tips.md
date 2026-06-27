```bash
# Chapter 14 - Linux Kernel Debugging Tips and Best Practices
```
Most of the time, as part of development, writing code is not the hardest part. Things are rendered difficult by the fact that the Linux kernel is a standalone software that is at the lowest layer of the operating system. This makes it challenging to debug the Linux kernel.
However, this is compensated by the fact that the majority of the time, we don't need additional tools to debug kernel code because most of the kernel debugging tools are part of the kernel itself. We will begin by familiarizing ourselves with the Linux kernel release model and you will learn the Linux kernel release process and steps. Then, we will look at the Linux kernel debugging-related development tips (especially debugging by printing)
and finally, we will focus on tracing the Linux kernel, ending with off-target debugging and learning to leverage kernel oops.
584 Linux Kernel Debugging Tips and Best Practices
This chapter will cover the following topics:
• Understanding the Linux kernel release process
• Linux kernel development tips
• Linux kernel tracing and performance analysis
• Linux kernel debugging tips
## Technical requirements
The following are prerequisites for this chapter:
• Advanced computer architecture knowledge and C programming skills
• Linux kernel v4.19.X sources, available at https://git.kernel.org/pub/
scm/linux/kernel/git/stable/linux.git/refs/tags
Understanding the Linux kernel release process
According to the Linux kernel release model, there are always three types of active kernel release: mainline, the stable release, and the Long-Term Support (LTS) release. First,
bug fixes and new features are gathered and prepared by subsystem maintainers and then submitted to Linus Torvalds in order for him to include them in his own Linux tree, which is called the mainline Linux tree, also known as the master Git repository. This is where every stable release originates from.
Before each new kernel version is released, it is submitted to the community through release candidate tags, so that developers can test and polish all the new features and,
most importantly, share feedback. During this cycle, Linus will rely on the feedback in order to decide whether the final version is ready to be released. When he is convinced that the new kernel is ready to go, he makes (tags it actually) the final release, and we call this release stable to indicate that it's no longer a release candidate: those releases are vX.Y
versions.
There is no strict timeline for making releases. However, new mainline kernels are generally released every 2–3 months. Stable kernel releases are based on Linus's releases,
that is, the mainline tree releases.
Understanding the Linux kernel release process 585
Once a mainline kernel is released by Linus, it also appears in the linux-stable tree
(available at https://git.kernel.org/pub/scm/linux/kernel/git/
stable/linux.git/), where it becomes a branch and from where it can receive bug fixes for a stable release. Greg Kroah-Hartman is responsible for maintaining this tree,
which is also referred to as the stable tree because it is used to track previously released stable kernels. That said, in order for a fix to be applied to this tree, this fix must first be incorporated in the Linus tree. Because the fix must go forth before coming back, it is said that this fix is back-ported. Once the bug is fixed in the mainline repository, it can then be applied to previously released kernels that are still maintained by the kernel development community. All fixes back-ported to stable releases must meet a set of mandatory acceptance criteria — and one of these criteria is that they must already exist in Linus's tree.
Important note
Bugfix kernel releases are considered stable.
For example, the 4.9 kernel is released by Linus, and then the stable kernel releases based on this kernel are numbered 4.9.1, 4.9.2, 4.9.3, and so on. Such releases are known as bugfix kernel releases, and the sequence is usually shortened with the number 4.9.y when referring to their branch in the stable kernel release tree. Each stable kernel release tree is maintained by a single kernel developer, who is responsible for picking the requisite patches for the release, and for performing the review/release process. There are usually only a few bugfix kernel releases until the next mainline kernel becomes available, unless it is designated a long-term maintenance kernel.
Every subsystem and kernel maintainer repository is hosted here: https://git.
kernel.org/pub/scm/linux/kernel/git/. There, we can also find either Linus or stable trees. In the Linus tree (https://git.kernel.org/pub/scm/linux/
kernel/git/torvalds/linux.git/), there is only one branch in Linus's tree,
that is, the master branch. Tags in there are either stable releases or release candidates.
In the stable tree (https://git.kernel.org/pub/scm/linux/kernel/git/
stable/linux.git/), there is one branch per stable kernel release (named <A.B>.y,
where <A.B> is the release version in the Linus tree) and each branch contains its bugfix kernel releases.
586 Linux Kernel Debugging Tips and Best Practices
Important note
There are a few links that you can keep to hand in order to follow the Linux kernel release. The first one is https://www.kernel.org/, from where you can download kernel archives, and then there is https://www.
kernel.org/category/releases.html, from where you can access the latest LTS kernel releases and their support timelines. You can also refer to this link, https://patchwork.kernel.org/, from where you can follow kernel patch submissions on a subsystem basis.
Now that we are familiar with the Linux kernel release model, we can delve into some development tips and best practices, which helps to consolidate and leverage other kernel developer experiences.
## Linux kernel development tips
The best Linux kernel development practices are inspired by existing kernel code. This way, you could certainly learn good practices. That said, we will not reinvent the wheel.
We will focus on what is necessary for this chapter, that is, debugging. The most frequently used debugging method involves logging and printing. In order to leverage this timetested debugging technique, the Linux kernel provides suitable logging APIs and exposes a kernel message buffer to store the logs. Though it may seem obvious, we will focus on the kernel logging APIs and learn how to manage the message buffer, either from within the kernel code or from user space.
## Message printing
Message printing and logging are inherent to development, irrespective of whether we are in kernel space or user space. In a kernel, the printk() function has long since been the de facto kernel message printing function. It is similar to printf() in the C library, but with the concept of log levels.
If you look at an example of actual driver code, you'll notice it is used as follows:
```c
printk(<LOG_LEVEL> "printf like formatted message\n");
```
Here, <LOG_LEVEL> is one of the eight different log levels defined in include/linux/
kern_levels.h and specifies the severity of the error message. You should also note that there is no comma between the log level and the format string (as the preprocessor concatenates both strings).
Linux kernel development tips 587
## Kernel log levels
The Linux kernel uses the concept of levels to determine how critical the message is. There are eight of them, each defined as a string, and they are described as follows:
• KERN_EMERG, defined as "0". It is to be used for emergency messages, meaning the system is about to crash or is unstable (unusable).
• KERN_ALERT, defined as "1", meaning that something bad happened and action must be taken immediately.
• KERN_CRIT, defined as "2", meaning that a critical condition occurred, such as a serious hardware/software failure.
• KERN_ERR, defined as "3" and used during an error condition, often used by drivers to indicate difficulties with the hardware or a failure to interact with a subsystem.
• KERN_WARNING, defined as "4" and used as a warning, meaning nothing serious by itself, but may indicate problems.
• KERN_NOTICE, defined as "5", meaning nothing serious, but notable nevertheless.
This is often used to report security events.
• KERN_INFO, defined as "6", used for informational messages, for example, startup information at driver initialization.
• KERN_DEBUG, defined as "7", used for debugging purposes, and active only if the
DEBUG kernel option is enabled. Otherwise, its content is simply ignored.
If you don't specify a log level in your message, it defaults to DEFAULT_MESSAGE_
```c
LOGLEVEL (usually "4" = KERN_WARNING), which can be set via the CONFIG_
```
DEFAULT_MESSAGE_LOGLEVEL kernel configuration option.
That said, for new drivers, you are encouraged to use more convenient printing APIs,
which embed the log level in their names. Those printing helpers are pr_emerg,
```c
pr_alert, pr_crit, pr_err, pr_warning, pr_warn, pr_notice, pr_info,
pr_debug, or pr_dbg. Besides being more concise than the equivalent printk() calls,
```
they can use a common definition for the format string through the pr_fmt() macro;
for instance, defining this at the top of a source file (before any #include directive):
```c
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
```
This would prefix every pr_*() message in that file with the module and function name that originated the message. pr_devel and pr_debug are replaced with printk(KERN_DEBUG …) if the kernel was compiled with DEBUG, otherwise they are replaced with an empty statement.
588 Linux Kernel Debugging Tips and Best Practices
The pr_*() family macros are to be used in core code. For device drivers, you should use the device-related helpers, which also accept the concerned device structure as a parameter. They also print the name of the relevant device in standard form, ensuring that it's always possible to associate a message with the device that generated it:
```c
dev_emerg(const struct device *dev, const char *fmt, ...);
dev_alert(const struct device *dev, const char *fmt, ...);
dev_crit(const struct device *dev, const char *fmt, ...);
dev_err(const struct device *dev, const char *fmt, ...);
dev_warn(const struct device *dev, const char *fmt, ...);
dev_notice(const struct device *dev, const char *fmt, ...);
dev_info(const struct device *dev, const char *fmt, ...);
dev_dbg(const struct device *dev, const char *fmt, ...);
```
While the concept of log levels is used by the kernel to determine the importance of a message, it is also used to decide whether this message should be presented to the user immediately, by printing it to the current console (where the console could also be a serial line or even a printer, not an xterm).
In order to decide, the kernel compares the log level of the message with the console_
loglevel kernel variable, and if the message log level importance is higher (that is,
a lower value) than console_loglevel, the message will be printed to the current console. Since the default kernel log level is usually "4", this is the reason why you don't see pr_info() or pr_notice() or even pr_warn() messages on the console, as they have higher or equal values (which means lower priority) than the default one.
To determine the current console_loglevel on your system, you can simply type the following:
```bash
$ cat /proc/sys/kernel/printk
```
4 4 1 7
The first integer (4) is the current console log level, the second number (4) is the default one, the third number (1) is the minimum console log level that can be set, and the fourth number (7) is the boot-time default console log level.
To change your current console_loglevel, simply write to the same file, that is, /
```c
proc/sys/kernel/printk. Hence, in order to get all messages printed to the console,
```
perform the following simple command:
```bash
# echo 8 > /proc/sys/kernel/printk
```
Linux kernel development tips 589
Every kernel message will appear on your console. You'll then have the following content:
```bash
# cat /proc/sys/kernel/printk
```
8 4 1 7
Another way to change the console log level is to use dmesg with the -n parameter:
```bash
# dmesg -n 5
```
With the preceding command, console_loglevel is set to print KERN_WARNING
(4) or more severe messages. You can also specify the console_loglevel at boot time using the loglevel boot parameter (refer to Documentation/kernelparameters.txt for more details).
Important note
There are also KERN_CONT and pr_cont, which are sort of special since they do not specify a level of urgency, but rather indicate a continued message.
They should only be used by core/arch code during early bootup (a continued line is not SMP-safe otherwise). This can be useful when part of a message line to be printed depends on the result of a computation, as in the following example:
[…]
```c
pr_warn("your last operation was ");
if (success)
pr_cont("successful\n");
else pr_cont("NOT successful\n");
```
You should keep in mind that only the final print statement has the trailing \n character.
590 Linux Kernel Debugging Tips and Best Practices
## Kernel log buffer
Whether they are immediately printed on the console or not, each kernel message is logged in a buffer. This kernel message buffer is a fixed-size circular buffer, which means that if the buffer fills up, it wraps around and you may lose a message. Thus, increasing the buffer size could be helpful. In order to change the kernel message buffer size, you can play with the LOG_BUF_SHIFT option, the value of which is used to left-shift by 1 in order to obtain the final size, the kernel log buffer size (for example, 16 => 1<<16 => 64KB, 17
=> 1 << 17 => 128KB). That said, it is a static size defined at compile time. This size can also be defined through kernel boot parameters, by using the log_buf_len parameter,
in other words, log_buf_len=1M (accept only power of 2 values).
## Adding timing information
```c
Sometimes, it is useful to add timing information to the printed messages, so you can see when a particular event occurred. The kernel includes a feature for doing this, called printk times, enabled through the CONFIG_PRINTK_TIME option. This option is found on the Kernel Hacking menu when configuring the kernel. Once enabled, this timing information prefixes each log message as follows:
bash
$ dmesg
```
[…]
[ 1.260037] loop: module loaded
[ 1.260194] libphy: Fixed MDIO Bus: probed
[ 1.260195] tun: Universal TUN/TAP device driver, 1.6
[ 1.260224] PPP generic driver version 2.4.2
[ 1.260260] ehci_hcd: USB 2.0 'Enhanced' Host Controller
(EHCI) Driver
[ 1.260262] ehci-pci: EHCI PCI platform driver
[ 1.260775] ehci-pci 0000:00:1a.7: EHCI Host Controller
[ 1.260780] ehci-pci 0000:00:1a.7: new USB bus registered,
assigned bus number 1
[ 1.260790] ehci-pci 0000:00:1a.7: debug port 1
[ 1.264680] ehci-pci 0000:00:1a.7: cache line size of 64 is not supported
[ 1.264695] ehci-pci 0000:00:1a.7: irq 22, io mem 0xf7ffa000
[ 1.280103] ehci-pci 0000:00:1a.7: USB 2.0 started, EHCI
1.00
[ 1.280146] usb usb1: New USB device found, idVendor=1d6b,
idProduct=0002
[ 1.280147] usb usb1: New USB device strings: Mfr=3,
Linux kernel tracing and performance analysis 591
Product=2, SerialNumber=1
[…]
The timestamps that are inserted into the kernel message output consist of seconds and microseconds (seconds.microseconds actually) as absolute values from the start of machine operation (or from the start of kernel timekeeping), which corresponds to the time when the bootloader passes control to the kernel (when you see something like [
0.000000] Booting Linux on physical CPU 0x0 on the console).
```c
Printk times can be controlled at runtime by writing to /sys/module/printk/
```
parameters/time in order to enable and disable printk timestamps. The following are examples:
```bash
# echo 1 >/sys/module/printk/parameters/time
# cat /sys/module/printk/parameters/time
```
N
```bash
# echo 1 >/sys/module/printk/parameters/time
# cat /sys/module/printk/parameters/time
```
Y
It does not control whether the timestamp is logged. It only controls whether it is printed while the kernel message buffer is being dumped, at boot time, or while using dmesg.
This may be an area for boot-time optimization. If disabled, it would take less time for logs to be printed.
We are now familiar with kernel printing APIs and their log buffer. We have seen how to tweak the message buffer, and add or remove information according to requirements.
Those skills can be used for debugging by printing. However, other debugging and tracing tools are shipped in the Linux kernel, and the following section will introduce some of them.
## Linux kernel tracing and performance analysis
Though debugging by printing covers most of the debugging needs, there are situations where we need to monitor the Linux kernel at runtime to track strange behavior, including latencies, CPU hogging, scheduling issues, and so on. In the Linux world, the most useful tool for achieving this is part of the kernel itself. The most important is ftrace, which is a Linux kernel internal tracing tool, and is the main topic of this section.
592 Linux Kernel Debugging Tips and Best Practices
## Using Ftrace to instrument the code
Function Trace, in short Ftrace, does much more than what its name says. For example,
it can be used to measure the time it takes to process interrupts, to track time-consuming functions, calculate the time to activate high-priority tasks, to track context switches, and much more.
Developed by Steven Rostedt, Ftrace has been included in the kernel since version 2.6.27 in 2008. This is the framework that provides a debugging ring buffer for recording data.
```c
This data is gathered by the kernel's integrated tracing programs. Ftrace works on top of the debugfs filesystem and is, most of the time, mounted in its own directory called tracing when it is enabled. In most modern Linux distributions, it is mounted by default in the /sys/kernel/debug/ directory (this is only available to the root user),
meaning that you can leverage Ftrace from within /sys/kernel/debug/tracing/.
```
The following are the kernel options to be enabled in order to support Ftrace on your system:
```c
CONFIG_FUNCTION_TRACER
CONFIG_FUNCTION_GRAPH_TRACER
CONFIG_STACK_TRACER
CONFIG_DYNAMIC_FTRACE
The preceding options depend on the architecture supporting tracing features by having the CONFIG_HAVE_FUNCTION_TRACER, CONFIG_HAVE_DYNAMIC_FTRACE, and
CONFIG_HAVE_FUNCTION_GRAPH_TRACER options enabled.
```
To mount the tracefs directory, you can add the following line to your /etc/fstab file:
```c
tracefs /sys/kernel/debug/tracing tracefs defaults 0 0
```
Or you can mount it at runtime with the help of the following command:
```bash
mount -t tracefs nodev /sys/kernel/debug/tracing
```
The contents of the directory should look like this:
```bash
# ls /sys/kernel/debug/tracing/
```
README set_event_pid available_events set_ftrace_filter available_filter_functions set_ftrace_notrace available_tracers set_ftrace_pid
Linux kernel tracing and performance analysis 593 buffer_size_kb set_graph_function buffer_total_size_kb set_graph_notrace current_tracer snapshot dyn_ftrace_total_info stack_max_size enabled_functions stack_trace events stack_trace_filter free_buffer trace function_profile_enabled trace_clock instances trace_marker max_graph_depth trace_options options trace_pipe per_cpu trace_stat printk_formats tracing_cpumask saved_cmdlines tracing_max_latency saved_cmdlines_size tracing_on set_event tracing_thresh
We won't describe all of these files and subdirectories, as this has already been covered in the official documentation. Instead, we'll just briefly describe the files relevant to our context:
• available_tracers: Available tracing programs.
• tracing_cpumask: This allows selected CPUs to be traced. The mask should be specified in a hex string format. For example, to trace only core 0, you should include a 1 in this file. To trace core 1, you should include a 2 in there. For core 3,
the number 8 should be included.
• current_tracer: The tracing program that is currently running.
• tracing_on: The system file responsible for enabling or disabling data writing to the ring buffer (to enable this, the number 1 has to be added to the file; to disable it,
the number 0 is added).
• trace: The file where tracing data is saved in a human-readable format.
Now that we have introduced Ftrace and described its functions, we can delve into its usage and learn how useful it can be for tracing and debugging purposes.
594 Linux Kernel Debugging Tips and Best Practices
## Available tracers
We can view the list of available tracers with the following command:
```bash
# cat /sys/kernel/debug/tracing/available_tracers blk function_graph wakeup_dl wakeup_rt wakeup irqsoff function nop
```
Let's take a quick look at the features of each tracer:
• function: A function call tracer without arguments.
• function_graph: A function call tracer with subcalls.
• blk: A call and event tracer related to block device I/O operations (this is what blktrace uses).
```c
• mmiotrace: A memory-mapped I/O operation tracer. It traces all the calls that a module makes to the hardware. It is enabled with CONFIG_ MMIOTRACE, which depends on CONFIG_HAVE_MMIOTRACE_SUPPORT.
• irqsoff: Traces the areas that disable interrupts and saves the trace with the longest maximum latency. This tracer depends on CONFIG_IRQSOFF_TRACER.
• preemptoff: Depends on CONFIG_PREEMPT_TRACER. It is similar to irqsoff, but traces and records the amount of time for which preemption is disabled.
```
• preemtirqsoff: Similar to irqsoff and preemptoff, but it traces and records the largest time for which irqs and/or preemption is disabled.
```c
• wakeup and wakeup_rt, enabled by CONFIG_SCHED_TRACER: The former traces and records the maximum latency that it takes for the highest priority task to get scheduled after it has been woken up, while the latter traces and records the maximum latency that it takes for just real-time (RT) tasks (in the same way as the current wakeup tracer does).
```
• nop: The simplest tracer, which, as the name suggests, doesn't do anything. The nop tracer simply displays the output of trace_printk() calls.
irqsoff, preemptoff, and preemtirqsoff are the so-called latency tracers. They measure how long interrupts are disabled for, how long preemption is disabled for, and how long interrupts and/or preemption are disabled for. Wakeup latency tracers measure how long it takes a process to run after it has been awoken for either all tasks or just RT
tasks.
Linux kernel tracing and performance analysis 595
## The function tracer
We'll begin our introduction to Ftrace with the function tracer. Let's look at a test script:
```bash
# cd /sys/kernel/debug/tracing
# echo function > current_tracer
# echo 1 > tracing_on
# sleep 1
# echo 0 > tracing_on
# less trace
```
This script is fairly straightforward, but there are a few things worth noting. We enable the current tracer by writing its name to the current_tracer file. Next, we write a 1 to tracing_on, which enables the ring buffer. The syntax requires a space between 1 and the > symbol; echo1> tracing_on will not work. One line later, we disable it (if 0 is written to tracing_on, the buffer won't clear and Ftrace won't be disabled).
Why would we do this? Between the two echo commands, we see the sleep 1 command. We enable the buffer, run this command, and then disable it. This lets the tracer include information relating to all of the system calls that occur while the command runs. In the last line of the script, we give the command to display tracing data in the console. Once the script has run, we'll see the following printout (this is just a small fragment):
Figure 14.1 – Ftrace function tracer snapshot
596 Linux Kernel Debugging Tips and Best Practices
The printout starts with information pertaining to the number of entries in the buffer and the total number of entries written. The difference between these two numbers is the number of events lost while filling the buffer. Then, there's a list of functions that includes the following information:
• The process name (TASK).
• The process identifier (PID).
• The CPU the process runs on (CPU#).
• The function start time (TIMESTAMP). This timestamp is the time since boot.
• The name of the function being traced (FUNCTION) and the parent function that was called following the <- symbol. For example, in the first line of our output, the irq_may_run function was called by handle_fasteoi_irq.
Now that we are familiar with the function tracer and its specificities, we can learn about the next tracer, which is more feature-rich and provides much more tracing information,
such as the call graph.
## The function_graph tracer
The function_graph tracer works just like a function, but in a more detailed manner:
the entry and exit point is shown for each function. With this tracer, we can trace functions with subcalls and measure the execution time of each function.
Let's edit the script from our previous example:
```bash
# cd /sys/kernel/debug/tracing
# echo function_graph > current_tracer
# echo 1 > tracing_on
# sleep 1
# echo 0 > tracing_on
# less trace
```
After running this script, we get the following printout:
```bash
# tracer: function_graph
```
#
```bash
# CPU DURATION FUNCTION CALLS
# | | | | | | |
```
5) 0.400 us | } /* set_next_buddy */
5) 0.305 us | __update_load_avg_se();
Linux kernel tracing and performance analysis 597
5) 0.340 us | __update_load_avg_cfs_rq();
```c
5) | update_cfs_group() {
5) | reweight_entity() {
5) | update_curr() {
```
5) 0.376 us | __calc_delta();
5) 0.308 us | update_min_vruntime();
5) 1.754 us | }
5) 0.317 us | account_entity_dequeue();
5) 0.260 us | account_entity_enqueue();
5) 3.537 us | }
5) 4.221 us | }
5) 0.261 us | hrtick_update();
5) + 16.852 us | } /* dequeue_task_fair */
5) + 23.353 us | } /* deactivate_task */
```c
5) | pick_next_task_fair() {
```
5) 0.286 us | update_curr();
5) 0.271 us | check_cfs_rq_runtime();
```c
5) | pick_next_entity() {
```
5) 0.441 us | wakeup_preempt_entity.isra.77();
5) 0.306 us | clear_buddies();
5) 1.645 us | }
------------------------------------------
5) SCTP ti-27174 => Composi-2089
------------------------------------------
5) 0.632 us | __switch_to_xtra();
5) 0.350 us | finish_task_switch();
5) ! 271.440 us | } /* schedule */
```c
5) | _cond_resched() {
```
5) 0.267 us | rcu_all_qs();
5) 0.834 us | }
5) ! 273.311 us | } /* futex_wait_queue_me */
598 Linux Kernel Debugging Tips and Best Practices
In this graph, DURATION shows the time spent running a function. Pay careful attention to the points marked by the + and ! symbols. The plus sign (+) means the function took more than 10 microseconds, while the exclamation point (!) means it took more than
100 microseconds. Under FUNCTION_CALLS, we find information pertaining to each function call. The symbols used to show the initiation and completion of each function are the same as in the C programming language: bracers ({}) demarcate functions, one at the start and one at the end; leaf functions that don't call any other function are marked with a semicolon (;).
Ftrace also allows tracing to be restricted just to functions that exceed a certain amount of time, using the tracing_thresh option. The time threshold at which the functions should be recorded must be written in that file in microsecond units. This can be used to find routines that are taking a long time in the kernel. It may be interesting to use this at kernel startup, to help optimize boot-up time. To set the threshold at startup, you can set it in the kernel command line as follows:
tracing_thresh=200 ftrace=function_graph
This traces all functions taking longer than 200 microseconds (0.2 ms). You can use any duration threshold you want.
At runtime, you can simply execute echo 200 > tracing_thresh.
## Function filters
Pick and choose what functions to trace. It goes without saying that fewer functions to trace equals less overhead. The Ftrace printout can be big, and finding exactly what you're looking for can be extremely difficult. However, we can use filters to simplify our search:
the printout will only display information about the functions we're interested in. To do this, we just have to write the name of our function in the set_ftrace_filter file, as follows:
```bash
# echo kfree > set_ftrace_filter
```
To disable the filter, we add an empty line to this file:
```bash
# echo > set_ftrace_filter
```
We run the following command:
```bash
# echo kfree > set_ftrace_notrace
```
Linux kernel tracing and performance analysis 599
The result is the opposite: the printout will give us information about every function except kfree(). Another useful option is set_ftrace_pid. This tool is for tracing functions that can be called on behalf of a particular process.
Ftrace has many more filtering options. For a more detailed look at these, you can read the official documentation available at https://www.kernel.org/doc/
Documentation/trace/ftrace.txt.
## Tracing events
Before introducing trace events, let's talk about tracepoints. Tracepoints are special code inserts that trigger system events. Tracepoints may be dynamic (meaning they have several checks attached to them) or static (no checks attached).
Static tracepoints do not affect the system in any way; they just add a few bytes for the function call at the end of the instrumented function and add a data structure in a separate section. Dynamic tracepoints call a trace function when the relevant code fragment is executed. Tracing data is written to the ring buffer. Tracepoints can be included anywhere in code. In fact, they can already be found in a lot of kernel functions.
Let's look at the kmem_cache_free function excerpt from mm/slab.c:
```c
void kmem_cache_free(struct kmem_cache *cachep, void *objp)
{
```
[...]
```c
trace_kmem_cache_free(_RET_IP_, objp);
}
```
kmem_cache_free is then itself a tracepoint. We can find countless more examples just by looking at the source code of other kernel functions.
```c
The Linux kernel has a special API for working with tracepoints from the user space. In the /sys/kernel/debug/tracing directory, there is an events directory where system events are saved. These are available for tracing. System events in this context can be understood as the tracepoints included in the kernel.
```
A list of these can be viewed by running the following command:
```bash
# cat /sys/kernel/debug/tracing/available_events mac80211:drv_return_void mac80211:drv_return_int mac80211:drv_return_bool mac80211:drv_return_u32
```
600 Linux Kernel Debugging Tips and Best Practices mac80211:drv_return_u64 mac80211:drv_start mac80211:drv_get_et_strings mac80211:drv_get_et_sset_count mac80211:drv_get_et_stats mac80211:drv_suspend
[...]
A long list will be printed out in the console with the <subsystem>:<tracepoint>
pattern. This is slightly inconvenient. We can print out a more structured list by using the following command:
```bash
# ls /sys/kernel/debug/tracing/events block gpio napi regmap syscalls cfg80211 header_event net regulator task clk header_page oom rpm timer compaction i2c pagemap sched udp enable irq power signal vmscan fib kmem printk skb workqueue filelock mac80211 random sock writeback filemap migrate raw_syscalls spi ftrace module rcu swiotlb
```
All possible events are combined in the subdirectory by subsystem. Before we can start tracing events, we will make sure we've enabled writing to the ring buffer.
```c
In Chapter 1, Linux Kernel Concepts for Embedded Developers, we introduced hrtimers. By listing the content of /sys/kernel/debug/tracing/events/timer, we will have timer-related tracepoints, including hrtimer-related ones, as follows:
bash
# ls /sys/kernel/debug/tracing/events/timer enable hrtimer_init timer_cancel filter hrtimer_start timer_expire_entry hrtimer_cancel itimer_expire timer_expire_exit hrtimer_expire_entry itimer_state timer_init hrtimer_expire_exit tick_stop timer_start
```
#
Linux kernel tracing and performance analysis 601
Let's now trace the access to hrtimer-related kernel functions. For our tracer, we'll use nop because function and function_graph record too much information,
including event information that we're just not interested in. The following is the script we will use:
```bash
# cd /sys/kernel/debug/tracing/
# echo 0 > tracing_on
# echo > trace
# echo nop > current_tracer
# echo 1 > events/timer/enable
# echo 1 > tracing_on;
# sleep 1;
# echo 0 > tracing_on;
# echo 0 > events/timer/enable
# less trace
```
We first disable tracing in case it was already running. Then we clear the ring buffer data before setting the current tracer to nop. Next, we enable timer-related tracepoints, or should we say, we enable timer event tracing. Finally, we enable tracing and dump the ring buffer content, which looks like the following:
Figure 14.2 – Ftrace event tracing with the nop tracer snapshot
At the end of the printout, we'll find information about hrtimer function calls (here is a small section). More detailed information about configuring event tracing can be found here: https://www.kernel.org/doc/Documentation/trace/events.txt.
602 Linux Kernel Debugging Tips and Best Practices
## Tracing a specific process with the Ftrace interface
Using Ftrace as is lets you have tracing-enabled kernel tracepoints/functions irrespective of the process those functions run on behalf of. To trace just the kernel functions executed on behalf of a particular function, you should set the pseudo set_ftrace_pid variable to the Process ID (PID) of the process, which can be obtained using pgrep, for example.
If the process is not already running, you can use a wrapper shell script and the exec command to execute a command as a known PID, as follows:
#!/bin/sh echo $$ > /debug/tracing/set_ftrace_pid
```bash
# [can set other filtering here]
echo function_graph > /debug/tracing/current_tracer exec $*
```
In the preceding example, $$ is the PID of the currently executing process (the shell script itself). This is set in the set_ftrace_pid variable, and then the function_graph tracer is enabled, after which this script executes the command (specified by the first argument to the script).
Assuming the script name is trace_process.sh, an example of usage could be the following:
```bash
sudo ./trace_command ls
```
Now we are familiar with tracing events and tracepoints. We are able to track and trace specific kernel events or subsystems. While tracing is a must in terms of kernel development, there are situations, which, sadly, affect the stability of the kernel. Such cases may require off-target analysis, which is addressed in debugging, and is discussed in the next section.
## Linux kernel debugging tips
Writing the code is not always the hardest aspect of kernel development. Debugging is the real bottleneck, even for experienced kernel developers. That said, most kernel debugging tools are part of the kernel itself. Sometimes, finding where the fault originated is assisted by the kernel via messages called Oops. Debugging then comes down to analyzing the message.
Linux kernel debugging tips 603
## Oops and panic analysis
Oops are messages printed by the Linux kernel when an error or an unhandled exception occurs. It tries its best to describe the exception and dumps the callstack just before the error or the exception occurs.
Take the following kernel module, for example:
```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
static void __attribute__ ((__noinline__)) create_oops(void) {
```
*(int *)0 = 0;
```c
}
static int __init my_oops_init(void) {
printk("oops from the module\n");
create_oops();
return 0;
}
static void __exit my_oops_exit(void) {
printk("Goodbye world\n");
}
module_init(my_oops_init);
module_exit(my_oops_exit);
MODULE_LICENSE("GPL");
```
In the preceding module code, we try to dereference a null pointer in order to panic the kernel. Moreover, we use the __noinline__ attribute in order for create_oops()
not to be inlined, allowing it to appear as a separate function during disassembly and in the callstack. This module has been built and tested on both the ARM and x86 platforms.
Oops messages and content will vary from machine to machine:
```bash
# insmod /oops.ko
```
[29934.977983] Unable to handle kernel NULL pointer dereference at virtual address 00000000
[29935.010853] pgd = cc59c000
[29935.013809] [00000000] *pgd=00000000
604 Linux Kernel Debugging Tips and Best Practices
[29935.017425] Internal error: Oops - BUG: 805 [#1] PREEMPT ARM
[...]
[29935.193185] systime: 1602070584s
[29935.196435] CPU: 0 PID: 20021 Comm: insmod Tainted: P
O 4.4.106-ts-armv7l #1
[29935.204629] Hardware name: Columbus Platform
[29935.208916] task: cc731a40 ti: cc66c000 task.ti: cc66c000
[29935.214354] PC is at create_oops+0x18/0x20 [oops]
[29935.219082] LR is at my_oops_init+0x18/0x1000 [oops]
[29935.224068] pc : [<bf2a8018>] lr : [<bf045018>] psr:
60000013
[29935.224068] sp : cc66dda8 ip : cc66ddb8 fp : cc66ddb4
[29935.235572] r10: cc68c9a4 r9 : c08058d0 r8 : c08058d0
[29935.240813] r7 : 00000000 r6 : c0802048 r5 : bf045000 r4
: cd4eca40
[29935.247359] r3 : 00000000 r2 : a6af642b r1 : c05f3a6a r0
: 00000014
[29935.253906] Flags: nZCv IRQs on FIQs on Mode SVC_32 ISA
ARM Segment none
[29935.261059] Control: 10c5387d Table: 4c59c059 DAC:
00000051
[29935.266822] Process insmod (pid: 20021, stack limit =
0xcc66c208)
[29935.272932] Stack: (0xcc66dda8 to 0xcc66e000)
[29935.277311] dda0: cc66ddc4 cc66ddb8 bf045018 bf2a800c cc66de44 cc66ddc8
[29935.285518] ddc0: c01018b4 bf04500c cc66de0c cc66ddd8 c01efdbc a6af642b cff76eec cff6d28c
[29935.293725] dde0: cf001e40 cc24b600 c01e80b8 c01ee628 cf001e40 c01ee638 cc66de44 cc66de08
[...]
[29935.425018] dfe0: befdcc10 befdcc00 004fda50 b6eda3e0 a0000010 00000003 00000000 00000000
[29935.433257] Code: e24cb004 e52de004 e8bd4000 e3a03000
(e5833000)
[29935.462814] ---[ end trace ebc2c98aeef9342e ]---
[29935.552962] Kernel panic - not syncing: Fatal exception
Linux kernel debugging tips 605
Let's have a closer look at the preceding dump to understand some of the important bits of information:
[29934.977983] Unable to handle kernel NULL pointer dereference at virtual address 00000000
The first line describes the bug and its nature, which in this case states that the code tried to dereference a NULL pointer:
[29935.214354] PC is at create_oops+0x18/0x20 [oops]
PC stands for program counter, which denotes the currently executed instruction address in memory. Here, we see that we were in the create_oops function, which is located in the oops module (which is listed in square brackets). The hex numbers indicate that the instruction pointer was 24 (0x18 in hex) bytes into the function, which appears to be 32
(0x20 in hex) bytes long:
[29935.219082] LR is at my_oops_init+0x18/0x1000 [oops]
LR is the link register, which contains the address to which the program counter should be set when it reaches a "return from subroutine" instruction. In other words, LR holds the address of the function that called the currently executing function (the one where PC is located). First, this means my_oops_init is the function that called the executing code.
It also means that if the function in PC had returned, the next line to be executed would be my_oops_init+0x18, which means the CPU would branch at the 0x18 offset from the start address of my_oops_init:
[29935.224068] pc : [<bf2a8018>] lr : [<bf045018>] psr:
60000013
In the preceding line of code, pc and lr are the real hexadecimal content of PC and
```c
LR, with no symbol name shown. Those addresses can be used with the addr2line program, which is another tool we can use to find a faulty line. This is what we would see in the printout if the kernel was built with the CONFIG_KALLSYMS option disabled.
```
We can then deduce that the addresses of create_oops and my_oops_init are
0xbf2a8000 and 0xbf045000, respectively:
[29935.224068] sp : cc66dda8 ip : cc66ddb8 fp : cc66ddb4
606 Linux Kernel Debugging Tips and Best Practices sp stands for stack pointer and holds the current position in the stack, while fp stands for frame pointer and points to the currently active frame in the stack. When a function returns, the stack pointer is restored to the frame pointer, which is the value of the stack pointer just before the function was called. The following example from Wikipedia explains it quite well:
For example, the stack frame of DrawLine would have a memory location holding the frame pointer value that DrawSquare uses. The value is saved upon entry to the subroutine and restored upon return:
[29935.235572] r10: cc68c9a4 r9 : c08058d0 r8 : c08058d0
[29935.240813] r7 : 00000000 r6 : c0802048 r5 : bf045000 r4
: cd4eca40
[29935.247359] r3 : 00000000 r2 : a6af642b r1 : c05f3a6a r0
: 00000014
The preceding is the dump of a number of CPU registers:
[29935.266822] Process insmod (pid: 20021, stack limit =
0xcc66c208)
The preceding line shows the process on behalf of which the panic occurred, which is insmod in this case, and its PID was 20021.
```c
There are also oops where the backtrace is present, a bit like the following, which is an excerpt from the oops generated by typing echo c > /proc/sysrq-trigger:
```
Figure 14.3 – Backtrace excerpt in a kernel oops
The backtrace traces the function call history before the one that generated the oops:
[29935.433257] Code: e24cb004 e52de004 e8bd4000 e3a03000
(e5833000)
Code is a hex-dump of the section of machine code that was being run at the time the oops occurred.
Linux kernel debugging tips 607
## Trace dump on oops
When the kernel crashes, it is possible to use kdump/kexec with the crash utility in order to examine the state of the system at the point of the crash. However, this technique does not let you see what has happened prior to the event that caused the crash, which may be a good input for understanding or fixing the bug.
```c
Ftrace is shipped with a feature that tries to address this issue. In order to enable it, you can either echo a 1 into /proc/sys/kernel/ftrace_dump_on_oops or enable ftrace_dump_on_oops in the kernel boot parameters. Having Ftrace configured along with this feature enabled will instruct Ftrace to dump the entire trace buffer to the console in ASCII format on oops or panic. Having the console output to a serial line makes debugging crashes much easier. This way, you can set everything up and just wait for the crash. Once it occurs, you'll see the trace buffer on the console. You'll then be able to trace back the events that led up to the crash. How far back you can go in tracing events depends on the size of the trace buffer, since this is what stores the event history data.
That said, dumping to the console may take a long time and it is common to shrink the trace buffer before putting everything in place, since the default Ftrace ring buffer is in excess of 1 megabyte per CPU. You can use /sys/kernel/debug/tracing/
```
buffer_size_kb in order to reduce the trace buffer size by writing in that file the number of kilobytes you want the ring buffer to be. Note that the value is per CPU, and not the total size of the ring buffer.
The following is an example of modifying the trace buffer size:
```bash
# echo 3 > /sys/kernel/debug/tracing/buffer_size_kb
```
The preceding command will shrink the Ftrace ring buffer down to 3 kilobytes per CPU (1 kb might be enough; it depends on how far you need to go back prior to the crash).
Using objdump to identify the faulty code line in the kernel module
We can use objdump to disassemble the object file and identify the line that generated the oops. We use the disassembled code to play with the symbol name and offset in order to point to the exact faulty line.
The following line will disassemble the kernel module in the oops.as file:
arm-XXXX-objdump -fS oops.ko > oops.as
608 Linux Kernel Debugging Tips and Best Practices
The generated output file will have content similar to the following:
[...]
architecture: arm, flags 0x00000011:
HAS_RELOC, HAS_SYMS
start address 0x00000000
Disassembly of section .text.unlikely:
00000000 <create_oops>:
0: e1a0c00d mov ip, sp
4: e92dd800 push {fp, ip, lr, pc}
8: e24cb004 sub fp, ip, #4 c: e52de004 push {lr} ; (str lr, [sp, #-4]!)
10: ebfffffe bl 0 <__gnu_mcount_nc>
14: e3a03000 mov r3, #0
18: e5833000 str r3, [r3]
1c: e89da800 ldm sp, {fp, sp, pc}
Disassembly of section .init.text:
00000000 <init_module>:
0: e1a0c00d mov ip, sp
4: e92dd800 push {fp, ip, lr, pc}
8: e24cb004 sub fp, ip, #4 c: e59f000c ldr r0, [pc, #12] ; 20
<init_module+0x20>
10: ebfffffe bl 0 <printk>
14: ebfffffe bl 0 <init_module>
18: e3a00000 mov r0, #0
1c: e89da800 ldm sp, {fp, sp, pc}
20: 00000000 .word 0x00000000
Disassembly of section .exit.text:
00000000 <cleanup_module>:
0: e1a0c00d mov ip, sp
Summary 609
4: e92dd800 push {fp, ip, lr, pc}
8: e24cb004 sub fp, ip, #4 c: e59f0004 ldr r0, [pc, #4] ; 18
<cleanup_module+0x18>
10: ebfffffe bl 0 <printk>
14: e89da800 ldm sp, {fp, sp, pc}
18: 00000016 .word 0x00000016
Important note
Enabling the debug option while compiling the module would make the debug info available in the .ko object. In this case, objdump -S would interpose the source code and assembly for a better view.
From the oops, we have seen that the PC is at create_oops+0x18, which is at the
0x18 offset from the address of create_oops. This leads us to the 18: e5833000 str r3, [r3] line. In order to understand the line of interest to us, let's describe the line before it, mov r3, #0. After this line, we have r3 = 0. Back to our line of interest, for people familiar with ARM assembly language, it means writing r3 to the original address pointed to by r3 (the C equivalent of [r3] is *r3). Remember, this corresponds to *(int *)0 = 0 in our code.
## Summary
This chapter introduced a number of kernel debugging tips, and explained how to use
Ftrace to trace the code in order to identify strange behavior, such as time-consuming functions and irq latencies. We covered the printing of APIs, either for core- or device driver-related code. Finally, we learned how to analyze and debug kernel oops.
This chapter marks the end of this book and I hope you have enjoyed the journey through this book while reading it as much as I did while writing it. I also hope that my best efforts in imparting my knowledge throughout this book will prove useful to you