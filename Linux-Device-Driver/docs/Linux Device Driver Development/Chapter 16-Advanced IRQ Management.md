```bash
# Chapter 16 - Advanced IRQ Management
```
On a Linux system, devices notify the kernel about particular events by means of IRQs. The
CPU exposes shared or unshared IRQ lines that are used by connected devices, so that when a device needs the CPU, it sends a request to it. When the CPU gets this request, it stops its actual job and saves its context, in order to serve the request issued by the device.
After serving the device, the CPU's state is restored back to exactly where it stopped when the interruption occurred. There are so many IRQ lines that another device is responsible for them to the CPU. That device is the interrupt controller, illustrated as follows:
Interrupt controller and IRQ lines
Not only devices can raise interrupts; some processor operations can do it too. There are two different kinds of interrupts:
1. Synchronous interrupts, called exceptions, are produced by the CPU while processing instructions. These are non-maskable interrupts (NMI), and they result from a critical malfunction, such as hardware failure. They are always processed by the CPU.
2. Asynchronous interrupts, called interrupts, are issued by other hardware devices. These are normal and maskable interrupts. We will discuss them in the next sections of this chapter. Therefore, let's go a bit deeper into exceptions.
Exceptions are the consequences of programming errors handled by the kernel, which sends a signal to the program and tries to recover from the error. Exceptions are classified into two categories, enumerated as follows:
Processor-detected exceptions: The CPU generates these in response to an anomalous condition; they are divided into three groups:
Faults, which can generally be corrected (bogus instructions).
Traps, which occur in a user process (invalid memory access,
division by zero, and so on), are also a mechanism to switch to the kernel mode in response to a system call. If the kernel code does cause a trap, it immediately panics.
Aborts are the serious errors.
Programmed exceptions: These are requested by the programmer and are handled like a trap.
The following array lists unmaskable interrupts (for more details, refer to http://wiki.osdev.org/Exceptions):
Interrupt number Description
0 Divide-by-zero error
1 Debug exception
2 NMI interrupt
3 Breakpoint
4 INTO detected overflow
5 BOUND range exceeded
6 Invalid opcode
7 Coprocessor (device) not available
8 Double fault
9 Coprocessor segment overrun
10 Invalid task state segment
11 Segment not present
12 Stack fault
13 General protection fault
14 Page fault
15 Reserved
16 Coprocessor error
17 - 31 Reserved
32 - 255 Maskable interrupts
NMIs are enough to cover the whole exception list. Going back to maskable interrupts, their number depends on the number of devices connected and how they actually share the IRQ
lines. Sometimes, they are not enough and some of them need multiplexing. A commonly used method is the use of a GPIO controller, which also acts as an interrupt controller. In this chapter, we will deal with the API that the kernel offers to manage IRQ and the ways in which multiplexing can be done, and we will get deeper into interrupt controller driver writing.
In this chapter, the following topics will be covered:
Interrupt controllers and interrupt multiplexing
## Advanced peripheral IRQ management
```c
Interrupt requests and propagations (chained or nested)
```
GPIOLIB irqchip API
Handling interrupt controllers from DT
Multiplexing interrupts and interrupt controllers
Usually having a single interrupt from the CPU is not enough. Most systems have hundreds of them. That is where the interrupt controller comes in, allowing them to be multiplexed. Very often, architecture or platform-specific interrupt controllers offer specific facilities, such as:
Masking/unmasking individual interrupts
Setting priorities
SMP affinity
Exotic things, such as wake-up interrupts
IRQ management and interrupt controller drivers both rely on the IRQ domain, which is, in turn, built on top of the following structures:
```c
struct irq_chip: This structure implements a set of methods describing how to drive the interrupt controller, which are directly called by the core IRQ code.
struct irqdomain: This structure provides the following:
```
A pointer to the firmware node for a given interrupt controller
(fwnode)
A method to convert a firmware description of an IRQ into an ID
```c
local to the interrupt controller (hwirq)
```
A way to retrieve the Linux view of an IRQ from hwirq struct irq_desc: This structure is the Linux view of an interrupt, containing all of the core stuff and one-to-one mapping to the Linux interrupt number struct irq_action: The structure Linux uses to describe an IRQ handler struct irq_data: This is embedded in the struct irq_desc structure, and contains:
The data that is relevant to the irq_chip managing this interrupt
Both the Linux IRQ number and the hwirq
A pointer to the irq_chip
Almost every irq_chip call is given an irq_data as a parameter, from which you can obtain the corresponding irq_desc.
All of the preceding structures are part of the IRQ domain API. An interrupt controller is represented in the kernel by an instance of the struct irq_chip structure, which describes the actual hardware device and some methods used by the IRQ core:
```c
struct irq_chip {
struct device *parent_device;
```
const char *name;
```c
void (*irq_enable)(struct irq_data *data);
void (*irq_disable)(struct irq_data *data);
void (*irq_ack)(struct irq_data *data);
void (*irq_mask)(struct irq_data *data);
void (*irq_unmask)(struct irq_data *data);
void (*irq_eoi)(struct irq_data *data);
int (*irq_set_affinity)(struct irq_data *data, const struct cpumask
```
*dest, bool force);
```c
int (*irq_retrigger)(struct irq_data *data);
int (*irq_set_type)(struct irq_data *data, unsigned int flow_type);
int (*irq_set_wake)(struct irq_data *data, unsigned int on);
void (*irq_bus_lock)(struct irq_data *data);
void (*irq_bus_sync_unlock)(struct irq_data *data);
int (*irq_get_irqchip_state)(struct irq_data *data, enum irqchip_irq_state which, bool *state);
```
int(*irq_set_irqchip_state)(struct irq_data *data, enum irqchip_irq_state which, bool state);
```c
unsigned long flags;
};
```
The following are the meanings of the elements in the structure:
parent_device: This is a pointer to the parent of this irqchip.
name: This is the name for the /proc/interrupts file.
```c
irq_enable: This hook enables the interrupt, and its default value is chip->unmask, if NULL.
irq_disable: This disables the interrupt.
irq_ack: This is the start of a new interrupt. Some controllers do not need this.
Linux calls this function as soon as an interrupt is raised, long before it is serviced. Some implementations map this function to chip->disable(), so that another interrupt request on the line will not cause another interrupt until after the current interrupt request has been serviced.
irq_mask: This is the hook that masks an interrupt source in the hardware, so that it cannot be raised anymore.
irq_unmask: This hook unmasks an interrupt source.
irq_eoi: eoi stands for end of interrupt. Linux invokes this hook right after IRQ
servicing completes. One uses this function to reconfigure the controller, as necessary, in order to receive another interrupt request on that line. Some implementations map this function to chip->enable(), to reverse operations done in chip->ack().
irq_set_affinity: This sets the CPU affinity, but only on SMP machines. In
```
SMP environments, this function sets the CPU on which the interrupt will be serviced. This function isn't used in single-processor machines.
```c
irq_retrigger: This retriggers the interrupt in the hardware, which resends an
```
IRQ to the CPU.
```c
irq_set_type: This sets the flow type (IRQ_TYPE_LEVEL/ and so on) of an IRQ.
irq_set_wake: This enables/disables the power-management wake-on of an
```
IRQ.
```c
irq_bus_lock: This locks access to slow bus (I2C) chips. Locking a mutex is sufficient.
irq_bus_sync_unlock: This syncs and unlocks slow bus (I2C) chips. Unlock the mutex that was previously locked.
irq_get_irqchip_state and irq_set_irqchip_state: These return or set the internal state of an interrupt, respectively.
```
Each interrupt controller is given a domain, which functions like the address space for a process (see Chapter 11, Kernel Memory Management). The interrupt controller domain is described as an instance of the struct irq_domain structure in the kernel. It manages mappings between hardware IRQs and Linux IRQs (that is, virtual IRQs). It is the hardware interrupt number translation object:
```c
struct irq_domain {
```
const char *name;
const struct irq_domain_ops *ops;
```c
void *host_data;
unsigned int flags;
```
/* Optional data */
```c
struct fwnode_handle *fwnode;
```
[...]
```c
};
```
name is the name of the interrupt domain ops is a pointer to the irq_domain methods.
host_data is a private data pointer, for use by the owner. It is not touched by the irq_domain core code.
flags is the host per the irq_domain flags.
fwnode is optional. It is a pointer to DT nodes associated with the irq_domain.
It's used when decoding DT interrupt specifiers.
An interrupt controller driver creates and registers an irq_domain by calling one of the irq_domain_add_<mapping_method>() functions, where <mapping_method> is the method by which hwirq should be mapped to the Linux IRQ. These are:
1. irq_domain_add_linear(): This uses a fixed-size table, indexed by the hwirq number. When an hwirq is mapped, an irq_desc is allocated for the hwirq, and the IRQ number is stored in the table. This linear mapping is suitable for fixed and small numbers of hwirq (~ < 256). The inconvenience of this mapping is the table size, which can be as large as the largest possible hwirq number. Therefore,
the IRQ number lookup time is fixed, and irq_desc is allocated for in-use IRQs only. The majority of drivers should use the linear map. This function has the following prototype:
```c
struct irq_domain *irq_domain_add_linear(struct device_node
```
*of_node,
```c
unsigned int size,
```
const struct irq_domain_ops *ops,
```c
void *host_data)
```
2. irq_domain_add_tree(): This is where the irq_domain maintains the mapping between Linux IRQs and hwirq numbers, in a radix tree. When an hwirq is mapped, an irq_desc is allocated, and the hwirq is used as the lookup key for the radix tree. The tree map is a good choice if the hwirq number is very large, since it does not need to allocate a table as large as the largest hwirq number. The disadvantage is that hwirq-to-IRQ number lookup is dependent on how many entries are in the table. Very few drivers should need this mapping. It has the following prototype:
```c
struct irq_domain *irq_domain_add_tree(struct device_node *of_node,
```
const struct irq_domain_ops *ops,
```c
void *host_data)
```
3. irq_domain_add_nomap(): You will probably never use this method; however,
its entire description is available in Documentation/IRQ-domain.txt, in the kernel source tree. Its prototype is as follows:
```c
struct irq_domain *irq_domain_add_nomap(struct device_node
```
*of_node,
```c
unsigned int max_irq,
```
const struct irq_domain_ops *ops,
```c
void *host_data)
```
of_node is a pointer to the interrupt controller's DT node. size represents the number of interrupts in the domain. ops represents map/unmap domain callbacks, and host_data is the controller's private data pointer.
Since the IRQ domain was empty at creation time (no mapping), you should use the irq_create_mapping() function to create mapping and assign it to the domain. In the next section, we will decide the right place in the code to create mappings:
```c
unsigned int irq_create_mapping(struct irq_domain *domain,
irq_hw_number_t hwirq)
```
domain: This is the domain to which this hardware interrupt belongs; NULL is the default domain
Hwirq: This is the hardware IRQ number in that domain space
When writing drivers for GPIO controllers that are also interrupt controllers,
```c
irq_create_mapping() is called from within the gpio_chip.to_irq() callback function, as shown here:
return irq_create_mapping(gpiochip->irq_domain, offset);
```
Some people prefer to create the mapping in advance, for each hwirq inside the probe function, as shown here:
```c
for (j = 0; j < gpiochip->chip.ngpio; j++) {
```
irq = irq_create_mapping(
```c
gpiochip ->irq_domain, j);
}
```
hwirq is the GPIO offset from the gpiochip.
If a mapping for the hwirq doesn't already exist, the function will allocate a new Linux irq_desc structure, associate it with the hwirq, and call the irq_domain_ops.map()
callback (by means of the irq_domain_associate() function), so that the driver can perform any required hardware setup:
```c
struct irq_domain_ops {
int (*map)(struct irq_domain *d, unsigned int virq, irq_hw_number_t hw);
void (*unmap)(struct irq_domain *d, unsigned int virq);
int (*xlate)(struct irq_domain *d, struct device_node *node,
```
const u32 *intspec, unsigned int intsize,
```c
unsigned long *out_hwirq, unsigned int *out_type);
};
```
.map(): This creates or updates a mapping between a virtual irq (virq) number and an hwirq number. This is called only once for a given mapping. It generally maps the virq with a given handler, using irq_set_chip_and_handler*, so that calling generic_handle_irq()or handle_nested_irq will trigger the right handler. The magic here is the irq_set_chip_and_handler() function:
```c
void irq_set_chip_and_handler(unsigned int irq,
struct irq_chip *chip, irq_flow_handler_t handle)
```
The following applies to the preceding function:
irq: This is the Linux IRQ, given as a parameter to the map() function.
chip: This is your irq_chip. Some controllers are quite dumb,and need almost nothing in their irq_chip structure. In that case, you should pass dummy_irq_chip, defined in kernel/irq/dummychip.c, which is a kernel irq_chip structure defined for such controllers.
handle: This determines the wrapper function that will call the real handler register, using request_irq(). Its value depends on whether the IRQ is edge or level-triggered. In either case, handle should be set to handle_edge_irq, or handle_level_irq. Both are kernel helper functions that perform a trick before and after calling the real IRQ handler. An example is shown as follows:
```c
static int pcf857x_irq_domain_map(struct irq_domain *domain,
unsigned int irq, irq_hw_number_t hw)
{
struct pcf857x *gpio = domain->host_data;
irq_set_chip_and_handler(irq, &dummy_irq_chip,handle_level_irq);
```
#ifdef CONFIG_ARM
```c
set_irq_flags(irq, IRQF_VALID);
```
#else irq_set_noprobe(irq);
```c
#endif gpio->irq_mapped |= (1 << hw);
return 0;
}
```
xlate: Given a DT node and an interrupt specifier, this hook decodes the hardware IRQ number and Linux IRQ type value. Depending on the
```dts
#interrupt-cells specified in your DT controller node, the kernel provides a generic translation function:
c
irq_domain_xlate_twocell(): This generic translation function is for direct two cell binding. The DT IRQ specifier works with two cell bindings, where the cell values map directly to the hwirq number and Linux IRQ flags.
irq_domain_xlate_onecell(): Generic xlate for direct one cell bindings.
irq_domain_xlate_onetwocell():: Generic xlate for one or two cell bindings.
```
An example of the domain operation is as follows:
```c
static struct irq_domain_ops mcp23016_irq_domain_ops = {
```
.map = mcp23016_irq_domain_map,
.xlate = irq_domain_xlate_twocell,
```c
};
```
When an interrupt is received, the irq_find_mapping() function should be used to find the Linux IRQ number from the hwirq number. Of course, the mapping must exist prior to being returned. A Linux IRQ number is always tied to a struct irq_desc structure,
which is the structure by which Linux describes an IRQ:
```c
struct irq_desc {
struct irq_common_data irq_common_data;
struct irq_data irq_data;
unsigned int __percpu *kstat_irqs;
irq_flow_handler_t handle_irq;
struct irqaction *action;
unsigned int irqs_unhandled;
```
raw_spinlock_t lock;
```c
struct cpumask *percpu_enabled;
```
atomic_t threads_active;
wait_queue_head_t wait_for_threads;
#ifdef CONFIG_PM_SLEEP
```c
unsigned int nr_actions;
unsigned int no_suspend_depth;
unsigned int force_resume_depth;
```
#endif
#ifdef CONFIG_PROC_FS
```c
struct proc_dir_entry *dir;
```
#endif int parent_irq;
```c
struct module *owner;
```
const char *name;
```c
};
```
Some fields that are not described here are internal, and are used by the IRQ core:
```c
irq_common_data is per IRQ and chip data, passed down to chip functions kstat_irqs defines per CPU IRQ statistics since boot handle_irq is a high-level IRQ events handler action represents the list of IRQ actions for this descriptor irqs_unhandled is the stats field for spurious unhandled interrupts lock represents locking for SMP
```
threads_active is the number of IRQ action threads currently running for this descriptor wait_for_threads represents the wait queue, for sync_irq to wait for threaded handlers nr_actions is the number of installed actions on this descriptor no_suspend_depth and force_resume_depth represent the number of irqactions on an IRQ descriptor with IRQF_NO_SUSPEND or
IRQF_FORCE_RESUME flags set dir represents the /proc/irq/ procfs entry name names the flow handler, visible in the /proc/interrupts output
The irq_desc.action field is a list of irqaction structures, each of which records the address of an interrupt handler for the associated interrupt source. Each call to the kernel's request_irq() function (or the threaded version, o) creates an added struct irqaction structure at the end of the list. For example, for a shared interrupt, this field will contain as many IRQ actions as there are handlers registered:
```c
struct irqaction {
irq_handler_t handler;
void *dev_id;
void __percpu *percpu_dev_id;
struct irqaction *next;
irq_handler_t thread_fn;
struct task_struct *thread;
unsigned int irq;
unsigned int flags;
unsigned long thread_flags;
unsigned long thread_mask;
```
const char *name;
```c
struct proc_dir_entry *dir;
};
```
The preceding code is explained as follows:
handler is the non-threaded (hard) interrupt handler function name is the device's name dev_id is a cookie to identify the device percpu_dev_id is a cookie to identify the device next is a pointer to the next IRQ action for shared interrupts irq is the Linux interrupt number flags represent the IRQ's flags (see IRQF_*)
thread_fn is the threaded interrupt handler function for threaded interrupts thread is a pointer to the thread structure in case of threaded interrupts thread_flags represents the flags related to the thread thread_mask is a bitmask for keeping track of thread activity dir points to the /proc/irq/NN/<name>/ entry
The interrupt handlers referenced by the irqaction.handler field are simply functions associated with the handling of interrupts from particular external devices, and they have minimal knowledge (if any) of the means by which those interrupt requests are delivered to the host microprocessor. They are not microprocessor-level interrupt service routines, and therefore, they do not exit through RTE or similar interrupt-related opcodes. This makes interrupt-driven device drivers largely portable across different microprocessor architectures.
The following is the definition of important fields of the struct irq_data structure,
which is per-IRQ chip data passed down to chip functions:
```c
struct irq_data {
```
[...]
```c
unsigned int irq;
unsigned long hwirq;
struct irq_common_data *common;
struct irq_chip *chip;
struct irq_domain *domain;
void *chip_data;
};
irq is the interrupt number (Linux IRQ)
```
hwirq is the hardware interrupt number, local to the irq_data.domain interrupt domain common points to data shared by all irqchips chip represents the low level interrupt controller hardware access domain represents the interrupt translation domain, responsible for mapping between the hwirq number and the Linux IRQ number chip_data is a platform-specific, per-chip private data for the chip methods, to allow shared chip implementations
## Advanced peripheral IRQ management
In Chapter 3, Kernel Facilities and Helper Functions, we introduced peripheral IRQs, using request_irq() and request_threaded_irq(). With request_irq(), one registers a handler (top half) that will be executed in atomic context, from which one can schedule a bottom half using one of a differing mechanism discussed in the same chapter. On the other hand, with request_thread_irq(), one can provide top and bottom halves to the function, so that the former will run as a hardirq handler, which may decide to raise the second and threaded handler that can run in a kernel thread.
The problem with those approaches is that sometimes, the drivers requesting an IRQ do not know about the nature of the interrupt that provides this IRQ line, especially when the interrupt controller is a discrete chip (typically, a GPIO expander connected over SPI or I2C
buses). Next comes request_any_context_irq(), a function with which drivers requesting an IRQ can find out whether the handler will run in a thread context or not, and can call request_threaded_irq() or request_irq(), accordingly. It means that whether the IRQ associated to our device comes from an interrupt controller that may not sleep (a memory mapped one) or from one that can sleep (behind an I2C/SPI bus), there will be no need to change the code. Its prototype is the following:
```c
int request_any_context_irq ( unsigned int irq, irq_handler_t handler,
unsigned long flags, const char * name, void * dev_id);
```
The following lists the meaning of each parameter in the function:
irq represents the interrupt line to allocate.
handler is the function to be called when the IRQ occurs. Depending on the context, this function may run as hardirq, or may be threaded.
flags represents the interrupt type flags. It is the same as those in request_irq().
name will be used for debug purposes, to name the interrupt in
```bash
/proc/interrupts.
```
dev_id is a cookie passed back to the handler function.
The request_any_context_irq() command means that one can either get a hardirq or a threaded one. It works like the usual request_irq(), except that it checks whether the
IRQ level is configured as nested, and calls the right backend. In other words, it selects either a hardIRQ or threaded handling method, depending on the context. This function returns a negative value up on failure. Upon success, it returns either IRQC_IS_HARDIRQ or
IRQC_IS_NESTED. The following is a use case:
```c
static irqreturn_t packt_btn_interrupt(int irq, void *dev_id)
{
struct btn_data *priv = dev_id;
input_report_key(priv->i_dev, BTN_0,
gpiod_get_value(priv->btn_gpiod) & 1);
input_sync(priv->i_dev);
return IRQ_HANDLED;
}
static int btn_probe(struct platform_device *pdev)
{
struct gpio_desc *gpiod;
int ret, irq;
```
[...]
```c
gpiod = gpiod_get(&pdev->dev, "button", GPIOD_IN);
if (IS_ERR(gpiod))
return -ENODEV;
priv->irq = gpiod_to_irq(priv->btn_gpiod);
priv->btn_gpiod = gpiod;
```
[...]
```c
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
return ret;
}
```
The preceding code is an excerpt of the driver sample of an input device driver. Actually, it is the one used in the next chapter. The advantage of using request_any_context_irq()
is that one does not need to care about what can be done in the IRQ handler, as the context in which the handler will run depends on the interrupt controller that provides the IRQ
line. In our example, if the GPIO is below a controller seated on an I2C or SPI bus, the handler will be threaded. Otherwise, the handler will run in hardirq.
## Interrupt request and propagation
Let us consider the following diagram, which represents a chained IRQ flow:
Interrupt requests are always performed on a Linux IRQ (not hwirq). The general function to request an IRQ on Linux is either request_threaded_irq() or request_irq(),
which internally calls the former:
```c
int request_threaded_irq(unsigned int irq, irq_handler_t handler,
irq_handler_t thread_fn, unsigned long irqflags,
```
const char *devname, void *dev_id)
When called, the function extracts the struct irq_desc associated with the IRQ, using the irq_to_desc() macro. It then allocates a new struct irqaction structure and sets it up, filling parameters such as the handler, the flags, and so on:
```c
action->handler = handler;
action->thread_fn = thread_fn;
action->flags = irqflags;
action->name = devname;
action->dev_id = dev_id;
```
That same function inserts/registers the descriptor in the proper IRQ list by invoking the __setup_irq() function (by means of setup_irq()), defined in kernel/irq/manage.c.
Now, when an IRQ is raised, the kernel executes a few assembler codes, in order to save the current state, and jumps to the arch-specific handler, handle_arch_irq, which is set with the handle_irq field of struct machine_desc of our platform, in the setup_arch()
function, in arch/arm/kernel/setup.c:
```c
handle_arch_irq = mdesc->handle_irq
```
For SoCs that use ARM GIC, the handle_irq callback is set with gic_handle_irq, in either drivers/irqchip/irq-gic.c or drivers/irqchip/irq-gic-v3.c:
```c
set_handle_irq(gic_handle_irq);
gic_handle_irq() calls handle_domain_irq(), which executes generic_handle_irq(), in turn calling generic_handle_irq_desc() and ending by calling desc->handle_irq(). Take a look at include/linux/irqdesc.h for the last call and arch/arm/kernel/irq.c for other function calls. handle_irq is the actual call for the flow handler, which we registered as mcp23016_irq_handler.
```
gic_hande_irq() is a GIC interrupt handler. generic_handle_irq() will execute the handler of the SoC's GPIO4 IRQ, which will look for the GPIO pin responsible for the interrupt, will call generic_handle_irq_desc(), and so on. Now that you are familiar with interrupt propagation, let us look at practical example by writing our own interrupt controller.
## Chaining IRQs
This section describes how interrupt handlers of a parent call the child's interrupt handlers,
in turn calling their child's interrupt handlers, and so on. The kernel offers two approaches to how to call interrupt handlers for child devices in the IRQ handler of the parent
(interrupt controller) device. These are the chained and nested methods, described in the following sections.
## Chained interrupts
This approach is used for the SoC's internal GPIO controller, which is memory mapped and whose access does not sleep. Chained means that those interrupts are just chains of function calls (for example, the SoC's GPIO module interrupt handler is being called from the GIC
interrupt handler, just like a function call). generic_handle_irq() is used for interrupts chaining child IRQ handlers, and it is called inside of the parent hwirq handler. Even from within the child interrupt handlers, we are still in atomic context (HW interrupt). One cannot call functions that may sleep.
## Nested interrupts
This method is used by controllers that sit on slow buses, such as I2C (for example, the
GPIO expander), and whose access may sleep (such as I2C functions). Nested refers to those interrupt handlers that do not run in the HW context (they are not really hwirq, and are not in atomic context), but are threaded instead, and can be preempted (or interrupted by another interrupt). handle_nested_irq() is used for creating nested interrupt child
IRQs. Handlers are called inside of the new thread created by the handle_nested_irq()
function; we need them to be run in the process context, so that we can call sleeping bus functions (such as I2C functions that may sleep).
## A case study – the GPIO and IRQ chip
Let us consider the following diagram, which ties interrupt controller device to another one which we will use to describe interrupt multiplexing:
mcp23016 IRQ flow
Suppose that you have configured io_1 and io_2 as interrupts. Even if the interrupt happens on io_1 or io_2, the same interrupt line will be triggered to the interrupt controller. Now, the GPIO driver has to read the interrupt status register of the GPIO to find which interrupt (io_1 or io_2) has really fired. Therefore, in this case, a single interrupt line is a multiplex for 16 GPIO interrupts.
Now, let us mangle the original driver of the mcp23016 written in Chapter 15, GPIO
Controller Drivers, gpio_chip, in order to support the IRQ domain API first, which will let it act as an interrupt controller, as well. The second part will introduce the new and recommended gpiolib irqchip API. This will be used as a step-by-step guide to write the interrupt controller driver, at least for the GPIO controller.
## The legacy GPIO and IRQ chip
1. The first step is to allocate a struct irq_domain to our gpiochip that will store the mapping between hwirq and virq. The linear mapping is suitable for us. We will do that in the probe function. That domain will hold the number of IRQs that our drivers wish to provide. For example, for an I/O expander, the number of IRQs could be the number of GPIOs the expander provides:
```c
my_gpiochip->irq_domain = irq_domain_add_linear(
client->dev.of_node,
my_gpiochip->chip.ngpio, &mcp23016_irq_domain_ops,
```
NULL);
The host_data parameter is NULL. Therefore, you can parse whatever data structure you need. Prior to allocating the domain, our domain ops structure should be defined, as follows:
```c
static struct irq_domain_ops mcp23016_irq_domain_ops = {
```
.map = mcp23016_irq_domain_map,
.xlate = irq_domain_xlate_twocell,
```c
};
```
And, prior to filling our IRQ domain ops structure, we must define the .map()
callback:
```c
static int mcp23016_irq_domain_map(
struct irq_domain *domain,
unsigned int virq, irq_hw_number_t hw)
{
irq_set_chip_and_handler(virq,
```
&dummy_irq_chip, /* Dumb irqchip */
```c
handle_level_irq); /* Level trigerred irq */
return 0;
}
```
Our controller is not smart enough. There is no need to set up an irq_chip. We will use the one provided by the kernel for this kind of chip: dummy_irq_chip.
Some controllers are smart enough and need an irq_chip to be set up. Take a look at drivers/gpio/gpio-mcp23s08.c.
The next ops callback is .xlate. Here, we will again use a helper provided by the kernel. irq_domain_xlate_twocell is a helper that is able to parse an interrupt specifier with two cells. We can add this interrupt-cells = <2>; in our controller DT node.
2. The next step is to fill the domain with IRQ mappings, using the irq_create_mapping() function. In our driver, we will do it in the gpiochip.to_irq callback, so that whenever someone calls gpio{d}_to_irq() on the GPIO, the mapping will be returned (if it exists), or created (if it doesn't):
```c
static int mcp23016_to_irq(struct gpio_chip *chip,
unsigned offset)
{
return irq_create_mapping(chip->irq_domain, offset);
}
```
We could have done that for each GPIO in the probe function, and called irq_find_mapping() in the .to_irq function.
3. Now, still in the probe function, we need to register our controller's IRQ handler,
which is in turn is responsible for calling the right handler that raised the interrupt on its pins:
```c
devm_request_threaded_irq(client->irq, NULL,
```
mcp23016_irq, irqflags,
```c
dev_name(chip->parent), mcp);
```
The mcp23016 function should have been defined prior to registering the IRQ:
```c
static irqreturn_t mcp23016_irq(int irq, void *data)
{
struct mcp23016 *mcp = data;
unsigned int child_irq, i;
```
/* Do some stuff */
[...]
```c
for (i = 0; i < mcp->chip.ngpio; i++) {
if (gpio_value_changed_and_raised_irq(i)) {
```
child_irq =
```c
irq_find_mapping(mcp->chip.irqdomain, i);
handle_nested_irq(child_irq);
}
}
return IRQ_HANDLED;
}
handle_nested_irq(), already described in the preceding section, will create a dedicated thread for each handler registered.
```
## The new gpiolib irqchip API
Almost every GPIO controller driver was using an IRQ domain for the same purpose.
Instead of each of them rolling their own irqdomain handling, and so on, kernel developers decided to move that code to the gpiolib framework, by means of the GPIOLIB_IRQCHIP
Kconfig symbol, in order to harmonize the development and avoid redundant code.
That portion of code helps with handling the management of GPIO irqchips and the associated irq_domain and resource allocation callbacks, as well as their setup, using the reduced set of helper functions. These are gpiochip_irqchip_add() and gpiochip_set_chained_irqchip().
```c
gpiochip_irqchip_add(): This adds an irqchip to a gpiochip. A summary of this function's actions is as follows:
Sets the gpiochip.to_irq field to gpiochip_to_irq, which is an IRQ callback that returns irq_find_mapping(chip->irqdomain, offset);
Allocates an irq_domain to the gpiochip using the irq_domain_add_simple()
```
function, parsing a kernel IRQ core irq_domain_ops called gpiochip_domain_ops and defined in drivers/gpio/gpiolib.c
Creates mapping from 0 to gpiochip.ngpio, using the irq_create_mapping() function
Its prototype is as follows:
```c
int gpiochip_irqchip_add(struct gpio_chip *gpiochip,
struct irq_chip *irqchip,
unsigned int first_irq,
irq_flow_handler_t handler,
unsigned int type)
```
Where gpiochip is our GPIO chip (the one to add the irqchip to), irqchip is the irqchip to add to the gpiochip. first_irq, if not dynamically assigned, is the base (first) IRQ to allocate gpiochip IRQs from. handler is the IRQ handler to use (often a predefined IRQ
core function), and type is the default type for IRQs on this irqchip,
passing IRQ_TYPE_NONE, so that the core avoids setting up any default type in the hardware.
This function will handle two celled simple IRQs (because it sets irq_domain_ops.xlate to irq_domain_xlate_twocell) and assumes that all of the pins on the gpiochip can generate a unique IRQ.
```c
static const struct irq_domain_ops gpiochip_domain_ops = {
```
.map = gpiochip_irq_map,
.unmap = gpiochip_irq_unmap,
/* Virtually all GPIO irqchips are twocelled */
.xlate = irq_domain_xlate_twocell,
```c
};
gpiochip_set_chained_irqchip(): This function sets a chained irqchip to a gpio_chip from a parent IRQ, and passes a pointer to the struct gpio_chip as handler data:
void gpiochip_set_chained_irqchip(struct gpio_chip *gpiochip,
struct irq_chip *irqchip, int parent_irq,
irq_flow_handler_t parent_handler)
```
parent_irq is the IRQ number to which this chip is connected. In the case of our mcp23016, as shown in the figure in the Case study-GPIO and IRQ chip section, it corresponds to the IRQ of the gpio4_29 line. In other words, it is the parent IRQ number for this chained irqchip. parent_handler is the parent interrupt handler for the accumulated IRQ coming out of the gpiochip. If the interrupt is nested rather than cascaded
(chained), pass NULL in this handler argument.
With this new API, the only code to add to our probe function is as follows:
/* Do we have an interrupt line? Enable the irqchip */
```c
if (client->irq) {
dts
status = gpiochip_irqchip_add(&gpio->chip, &dummy_irq_chip,
```
0, handle_level_irq, IRQ_TYPE_NONE);
```c
if (status) {
dev_err(&client->dev, "cannot add irqchip\n");
```
goto fail_irq;
```c
}
dts
status = devm_request_threaded_irq(&client->dev, client->irq,
```
NULL, mcp23016_irq, IRQF_ONESHOT |
IRQF_TRIGGER_FALLING | IRQF_SHARED,
```c
dev_name(&client->dev), gpio);
if (status)
```
goto fail_irq;
```c
gpiochip_set_chained_irqchip(&gpio->chip,
&dummy_irq_chip, client->irq, NULL);
}
```
IRQ core does everything for us. There is no need to define even the gpiochip.to_irq function, since the API already sets it. Our example uses the IRQ core dummy_irq_chip,
but one could have defined its own as well. Since the v4.10 version of the kernel, two other functions have been added; these are gpiochip_irqchip_add_nested() and gpiochip_set_nested_irqchip(). Take a look at Documentation/gpio/driver.txt for more details. A driver that uses this API in the same kernel version is drivers/gpio/gpio-mcp23s08.c.
## The interrupt controller and DT
Now, we will declare our controller in the DT. As you may remember from Chapter 6, The
```dts
Concept of Device Tree, every interrupt controller must have the Boolean property interrupt controller set. The second mandatory Boolean property is gpio-controller, since it is a
GPIO controller, too. We need to define how many cells are needed for an interrupt specifier for our device. Since we have the irq_domain_ops.xlate field set to irq_domain_xlate_twocell, #interrupt-cells should be set to 2:
c
expander: mcp23016@20 {
dts
compatible = "microchip,mcp23016";
reg = <0x20>;
interrupt-controller;
#interrupt-cells = <2>;
gpio-controller;
#gpio-cells = <2>;
interrupt-parent = <&gpio4>;
interrupts = <29 IRQ_TYPE_EDGE_FALLING>;
c
};
dts
The interrupt-parent and interrupts properties describe the interrupt line connection.
```
Finally, let's suppose that we have a driver for mcp23016, and drivers for two other devices, foo_device and bar_device (all running in the CPU, of =course). In the foo_device driver, one wants to request interrupt for events when foo_device changes a level on the io_2 pin of mcp23016. The bar_device driver requires io_8 and io_12,
respectively, for reset and power GPIOs. Let us declare this in the DT:
```c
foo_device: foo_device@1c {
dts
reg = <0x1c>;
interrupt-parent = <&expander>;
interrupts = <2 IRQ_TYPE_EDGE_RISING>;
c
};
bar_device {
```
reset-gpios = <&expander 8 GPIO_ACTIVE_HIGH>;
power-gpios = <&expander 12 GPIO_ACTIVE_HIGH>;
/* Other properties do here */
```c
};
```
## Summary
Now, IRQ multiplexing contains no more secrets for you. In this chapter, we discussed the most important elements of IRQ management in Linux systems, the IRQ domain API. You learned the basics for developing interrupt controller drivers and managing their bindings from within the DT. IRQ propagation was discussed so that you could understand what happens from the request to the handling. This chapter will help you to understand the interrupt-driven part of the next chapter, which deals with input device drivers