```bash
# Chapter 4 - Storming the Common Clock Framework
```
From the beginning, embedded systems have always needed clock signals in order to orchestrate their inner workings, either for synchronization or for power management
(for example, enabling clocks when the device is in active use or adjusting the clock depending on some criteria, such as the system load). Therefore, Linux has always had a clock framework. There has only ever been programming interface declaration support for software management of the system clock tree, and each platform had to implement this API. Different System on Chips (SoCs) had their own implementation. This was okay for a while, but people soon found that their hardware implementations were quite similar. The code also became bushy and redundant, which meant it was necessary to use platform-dependent APIs to get/set the clock.
146 Storming the Common Clock Framework
This was rather an uncomfortable situation. Then, the common clock framework (CCF)
came in, allowing software to manage clocks available on the system in a hardwareindependent manner. The CCF is an interface that allows us to control various clock devices (most of time, these are embedded in SoCs) and offers a uniform API that can be used to control them (enabling/disabling, getting/setting the rate, gating/un-gating, and so on). In this chapter, the concept of a clock does not refer to Real-Time Clocks (RTCs),
nor timekeeping devices, which are other kinds of devices that have their own subsystems in the kernel.
The main idea behind the CCF is to unify and abstract the similar code that's spread in different SoC clock drivers. This standardized approach introduced the concept of a clock provider and a clock consumer in the following manner:
• Providers are Linux kernel drivers that connect with the framework and provide access to hardware, thereby providing (making these available to consumers)
```c
the clock tree (thanks to which one can dump the whole clock tree nowadays)
```
according to the SoC datasheet.
• Consumers are Linux kernel drivers or subsystems that access the framework through a common API.
• That being said, a driver can be both a provider and a consumer (it would then either consume one or more clocks it provides, or one or more clocks provided by others).
In this chapter, we will introduce CCF data structures, and then focus on writing clock provider drivers (regardless of the clock type) before introducing the consumer API. We will do this by covering the following topics:
• CCF data structures and interfaces
• Writing a clock provider device driver
• Clock consumer device drivers and APIs
Technical requirements
The following are the technical requirements for this chapter:
• Advanced computer architecture knowledge and C programming skills
• Linux kernel v4.19.X sources, available at https://git.kernel.org/pub/
scm/linux/kernel/git/stable/linux.git/refs/tags
CCF data structures and interfaces 147
## CCF data structures and interfaces
In the old kernel days, each platform had to implement a basic API defined in the kernel
(to grab/release the clock, set/get the rate, enable/disable the clock, and so on) that could be used by consumer drivers. Since the implementation of these specific APIs was done by each machine's code, this resulted in a similar file in each machine directory, with similar logic to implement the clock provider functions. This had several drawbacks, among which there was a lot of redundant code inside them. Later, the kernel abstracted this common code in the form of a clock provider (drivers/clk/clk.c), which became what we now call the CCF core.
Before playing with the CCF, its support needs to be pulled into the kernel by means of the
CONFIG_COMMON_CLK option. The CCF itself is divided into two halves:
• The Common Clock Framework core: This is the core of the framework and is not supposed to be modified when you add a new driver and provide the common definition of struct clk, which unifies the framework-level code and the traditional platform-dependent implementation that used to be duplicated across a variety of platforms. This half also allows us to wrap the consumer interface (also called the clk implementation) on top of struct clk_ops, which must be provided by each clock provider.
• The hardware-specific half: This targets the clock device that must be written for each new hardware clock. This requires the driver to provide struct clk_ops that corresponds to the callbacks that are used to let us operate on the underlying hardware (these are invoked by the clock's core implementation), as well as the corresponding hardware-specific structures that wrap and abstract the clock hardware.
The two halves are tied together by the struct clk_hw structure. This structure helps us with implementing our own hardware clock type. In this chapter, this is referenced as struct clk_foo. Since struct clk_hw is also pointed to within struct clk, it allows for navigation between the two halves.
148 Storming the Common Clock Framework
Now, we can introduce CCF data structures. The CCF is built on top of common heterogeneous data structures (in include/linux/clk-provider.h) that help keep this framework as generic as possible. These are as follows:
• struct clk_hw: This structure abstracts the hardware clock line and is used in the provider code only. It ties the two halves introduced previously and allows navigation to occur between them. Moreover, this hardware clock's base structure allows platforms to define their own hardware-specific clock structure, along with their own clock operation callbacks, as long as they wrap an instance of the struct clk_hw structure.
• struct clk_ops: This structure represents the hardware-specific callbacks that can operate on a clock line; that is, the hardware. This is why all of the callbacks in this structure accept a pointer to a struct clk_hw as the first parameter, though only a few of these operations are mandatory, depending on the clock type.
• struct clk_init_data: This holds init data that's common to all clocks that are shared between the clock provider and the common clock framework. The clock provider is responsible for preparing this static data for each clock in the system,
and then handing it to the core logic of the clock framework.
• struct clk: This structure is the consumer representation of a clock since each consumer API relies on this structure.
• struct clk_core: This is the CCF representation of a clock.
Important note
Discerning the difference between struct clk_hw and struct clk allows us to move closer to a clear split between the consumer and provider clk
APIs.
Now that we have enumerated the data structures of this framework, we can go through them and learn how they are implemented and what they are used for.
Understanding struct clk_hw and its dependencies struct clk_hw is the base structure for every clock type in the CCF. It can be seen as a handle for traversing from a struct clk to its corresponding hardware-specific structure. The following is the body of struct clk_hw:
```c
struct clk_hw {
struct clk_core *core;
struct clk *clk;
```
CCF data structures and interfaces 149 const struct clk_init_data *init;
```c
};
```
Let's take a look at the fields in the preceding structure:
• core: This structure is internal to the framework core. It also internally points back to this struct clk_hw instance.
• clk: This is a per-user struct clk instance that can operate with the clk API.
It is assigned and maintained by the clock framework and provided to the clock consumer when needed. Whenever the consumer initiates access to the clock device
(that is, clk_core) in the CCF through clk_get, it needs to obtain a handle,
which is clk.
• init: This is a pointer to struct clk_init_data. In the process of initializing the underlying clock provider driver, the clk_register() interface is called to register the clock hardware. Prior to this, you need to set some initial data, and this initial data is abstracted into a struct clk_init_data data structure. During the initialization process, the data from clk_init_data is used to initialize the clk_core data structure that corresponds to clk_hw. When the initialization is completed, clk_init_data has no meaning.
```c
struct clk_init_data is defined as follows:
struct clk_init_data {
const char *name;
const struct clk_ops *ops;
const char * const *parent_names;
```
u8 num_parents;
```c
unsigned long flags;
};
```
It holds initialization data that's common to all clocks and is shared between the clock provider and the common clock framework. Its fields are as follows:
• name, which denotes the name of the clock.
• ops is a set of operation functions related to the clock. This will be described later in the Providing clock ops section. Its callbacks are to be provided by the clock provider driver (in order to allow driving hardware clocks), and will be invoked by drivers through the clk_* consumer API.
• parent_names contains the names of all the parent clocks of the clock. This is an array of strings that holds all possible parents.
150 Storming the Common Clock Framework
• num_parents is the number of parents. It should correspond to the number of entries in the preceding array.
• flags represent the framework-level flags of the clock. We will explain this in detail later in the Providing clock ops section, since these flags actually modify some ops.
Important note struct clk and struct clk_core are private data structures and are defined in drivers/clk/clk.c. The struct clk_core structure abstracts a clock device to the CCF layer in such a way that each actual hardware clock device (struct clk_hw) corresponds to a struct clk_core.
Now that we are done with struct clk_hw, which is the centerpiece of the CCF, we can learn how to register a clock provider with the system.
Registering/unregistering the clock provider
The clock provider is responsible for exposing the clocks it provides in the form of a tree, sorting them out, and initializing the interface through the provider or the clock framework's core during system initialization.
In the early kernel days (before the CCF), clock registration was unified by the clk_
register() interface. Now that we have clk_hw-based (provider) APIs, we can get rid of struct clk-based APIs while registering clocks. Since it's recommended that clock providers use the new struct clk_hw-based API, the appropriate registration interface to consider is devm_clk_hw_register(), which is the managed version of clk_hw_register(). However, for historical reasons, the old clk-based API name is still maintained, and you may find several drivers using it. A resource managed version has even been implemented called devm_clk_register(). We're only discussing this old API is to let you understand the existing code, not to help you implement new drivers:
```c
struct clk *clk_register(struct device *dev, struct clk_hw *hw)
int clk_hw_register(struct device *dev, struct clk_hw *hw)
```
Based on this clk_hw_register() interface, the kernel also provides other more convenient registration interfaces (which will be introduced later), depending on the clock type to be registered. It is responsible for registering the clock to the kernel and returning a struct clk_hw pointer representing the clock.
CCF data structures and interfaces 151
It accepts a pointer to a struct clk_hw (since struct clk_hw is the provider side representation of a clock) and must contain some of the information of the clock to be registered. This will be populated with further data by the kernel. The implementation logic for this is as follows:
```c
• Assigning the struct clk_core space (clk_hw->core):
```
--Initializing the field's name, ops, hw, flags, num_parents, and parents_
names of clk according to the information provided by the struct clk_hw pointer.
--Calling the kernel interface, __clk_core_init(), on it to perform subsequent initialization operations, including building the clock tree hierarchy.
```c
• Assigning the struct clk space (clk_hw->clk) by means of the internal kernel interface, clk_create_clk(), and returning this struct clk variable.
```
• Even though clk_hw_register() wraps clk_register(), you should not use clk_register() directly as it returns struct clk. This may lead to confusion and breaks the strict separation between the provider and consumer interfaces.
The following is the implementation of clk_hw_register in drivers/clk/clk.c:
```c
int clk_hw_register(struct device *dev, struct clk_hw *hw)
{
return PTR_ERR_OR_ZERO(clk_register(dev, hw));
}
```
You should check the return value of clk_hw_register() prior to executing further steps. Since the CCF framework is responsible for establishing the tree structure of the entire abstract clock tree and maintaining its data, it does this by means of two static linked lists that are defined in drivers/clk/clk.c, as follows:
```c
static HLIST_HEAD(clk_root_list);
static HLIST_HEAD(clk_orphan_list);
```
152 Storming the Common Clock Framework
Whenever you call clk_hw_register() (which internally calls __clk_core_
init() in order to initialize the clock) on a clock hw, if there is a valid parent for this clock, it will end up in the children list of the parent. On other hand, if num_parent is 0, it is placed in clk_root_list. Otherwise, it will hang inside clk_orphan_
list, meaning that it has no valid parent. Moreover, every time a new clk is clk_init'd,
CCF will walk through clk_orphan_list (the list of orphan clocks) and re-parent any that are children of the clock currently being initialized. This is how CCF keeps the clock tree consistent with the hardware topology.
On the other hand, struct clk is the consumer-side instance of a clock device.
Basically, all user access to the clock device creates an access handle of the struct clk type. When different users access the same clock device, although the same struct clk_core instance is being used under the hood, the handles they access (struct clk) are different.
Important note
You should keep in mind that clk_hw_register (or its ancestor, clk_
register()) plays with struct clk_core under the hood since this is the CCF representation of a clock.
The CCF manages clk entities by means of a globally linked list declared in drivers/
clk/clkdev.c, along with a mutex to protect its access, as follows:
```c
static LIST_HEAD(clocks);
static DEFINE_MUTEX(clocks_mutex);
```
This comes from the era where the device tree was not heavily used. Back then, the clock consumer obtained clk by name (the name of the clk). This was used to identify clocks.
Knowing that the purpose of clk_register() is just to register to the common clock framework, there was no way for the consumer to know how to locate the clk. So, for the underlying clock provider driver, in addition to calling the clk_register() function to register to the common clock framework, clk_register_clkdev() also had to be called immediately after clk_register() in order to bind the clock with a name
(otherwise, the clock consumer wouldn't know how to locate the clock). Therefore, the kernel used struct clk_lookup, as its name says, to look up the available clock in case a consumer requested a clock (by name, of course).
This mechanism is still valid and supported in the kernel. However, in order to enforce separation between the provider and consumer code using a hw-based API, clk_
register() and clk_register_clkdev() should be replaced with clk_hw_
register() and clk_hw_register_clkdev() in your code, respectively.
CCF data structures and interfaces 153
In other words, let's say you have the following code:
/* Not be used anymore, introduced here for studying purpose */
```c
int clk_register_clkdev(struct clk *clk,
const char *con_id, const char *dev_id)
```
This should be replaced with the following code:
/* recommended interface */
```c
int clk_hw_register_clkdev(struct clk_hw *hw,
const char *con_id,
const char *dev_id)
```
Going back to the struct clk_lookup data structure, let's take a look at its definition:
```c
struct clk_lookup {
struct list_head node;
const char *dev_id;
const char *con_id;
struct clk *clk;
struct clk_hw *clk_hw;
};
```
In the preceding data structure, dev_id and con_id are used to identify/find the appropriate clk. This clk is the corresponding underlying clock. node is the list entry that will hang inside the global clocks list, as shown in the low-level __clkdev_add()
function in the following excerpt:
```c
static void __clkdev_add(struct clk_lookup *cl)
{
mutex_lock(&clocks_mutex);
list_add_tail(&cl->node, &clocks);
mutex_unlock(&clocks_mutex);
}
```
154 Storming the Common Clock Framework
The preceding __clkdev_add() function is indirectly called from within clk_
hw_register_clkdev(), which actually wraps clk_register_clkdev().
Now that we've introduced the device tree, things have changed. Basically, each clock provider became a node in DTS; that is, each clk has a device node in the device tree that corresponds to it. In this case, instead of bundling clk and a name, it is better to bundle clk and your device nodes by means of a new data structure, struct of_clk_
provider. This specific data structure is as follows:
```c
struct of_clk_provider {
struct list_head link;
struct device_node *node;
struct clk *(*get)(struct of_phandle_args *clkspec,
void *data);
struct clk_hw *(*get_hw)(struct of_phandle_args *clkspec,
void *data);
void *data;
};
```
In the preceding structure, the following takes place:
• link hangs in the of_clk_providers global list.
• node represents the DTS node of the clock device.
• get_hw is a callback for the decoding clock. For devices (consumers), it is called through clk_get() to return the clock associated with the node or NULL.
• get is there for the old clk-based APIs for historical and compatibility reasons.
However, nowadays, due to the frequent and common use of the device tree, for the underlying provider driver, the original clk_hw_register() + clk_hw_register_
clkdev() (or its old clk-based implementation, clk_register() + clk_
register_clkdev()) combination becomes a combination of clk_hw_register +
```c
of_clk_add_hw_provider (formerly clk_register + of_clk_add_provider
```
– this can be found in old and non-clk_hw-based drivers). Also, a new globally linked list, of_clk_providers, has been introduced in the CCF to help manage the correspondence between all DTS nodes and clocks, along with a mutex to protect this list:
```c
static LIST_HEAD(of_clk_providers);
static DEFINE_MUTEX(of_clk_mutex);
```
CCF data structures and interfaces 155
Although the clk_hw_register() and clk_hw_register_clkdev() function names are quite similar, the goals of these two functions differ. With the former, the clock provider can register a clock in the common clock framework. On the other hand,
```c
clk_hw_register_clkdev()registers a struct clk_lookup in the common clock framework, as its name suggests. This operation is mainly for finding clk. If you have a device tree-only platform, you no longer need all the calls to clk_hw_register_
```
clkdev() (unless you have a strong reason to), so you should rely on one call to of_
```c
clk_add_provider().
```
Important note
Clock providers are recommended to use the new struct clk_hw-based
API as this allows us to move closer to a clear split of consumer and provider clk APIs.
```c
clk_hw_* interfaces are provider interfaces that should be used in clock provider drivers, while clk_* is for the consumer side. Whenever you encounter a clk_*-based API in provider code, note that this driver should be updated to support the new hw-based interface.
Some drivers still use both functions (clk_hw_register_clkdev()
```
and of_clk_add_hw_provider()) in order to support both clock lookup methods, such as SoC clock drivers, but you should not use both unless you have a reason to do so.
So far, we have spent time discussing clock registration. However, it might be necessary to unregister a clock, either because the underlying clock hardware goes off the system or because things went wrong during hardware initialization. Clock unregistration APIs are fairly straightforward:
```c
void clk_hw_unregister(struct clk_hw *hw)
void clk_unregister(struct clk *clk)
```
The former targets clk_hw-based clocks, while the second targets clk-based ones. When it comes to managed variants, unless the Devres core handles unregistration, you should use the following APIs:
```c
void devm_clk_unregister(struct device *dev, struct clk *clk)
void devm_clk_hw_unregister(struct device *dev, struct clk_hw
```
*hw)
In both case, dev represents the underlying device structure associated with the clock.
156 Storming the Common Clock Framework
With that, we have finished looking at clock registration/unregistration. That being said, one of the main purposes of the driver is to expose device resources to potential consumers, and this applies to clock devices as well. In the next section, we'll learn how to expose clock lines to consumers.
```c
Exposing clocks to others (in detail)
```
Once the clocks have been registered with CCF, the next step consists of registering this clock provider so that other devices can consume its clock lines. In the old kernel days
(when the device tree was not heavily used), you had to expose clocks to the consumer by calling clk_hw_register_clkdev() on each clock line, which resulted in registering a lookup structure for the given clock line. Nowadays, the device tree is used for this purpose by calling the of_clk_add_hw_provider() interface, as well as a certain number of arguments:
```c
int of_clk_add_hw_provider(
struct device_node *np,
struct clk_hw *(*get)(struct of_phandle_args *clkspec,
void *data),
void *data)
```
Let's take a look at the arguments in this function:
• np is the device node pointer associated with the clock provider.
• get is a callback for the decoding clock. We will discuss this callback in detail in the next section.
• data is the context pointer for the given get callback. This is usually a pointer to the clock(s) that need to be associated with the device node. This is useful for decoding.
This function returns 0 on a success path. It does the opposite to of_clk_del_
provider(), which consists of removing the provider from the global list and freeing its space:
```c
void of_clk_del_provider(struct device_node *np)
```
Its resource managed version, devm_of_clk_add_hw_provider(), can also be used to get rid of the deletion function.
CCF data structures and interfaces 157
The clock provider device tree node and its associated mechanisms
For a quite some time now, the device tree is the preferred method to describe (declare)
devices on a system. The common clock framework does not escape this rule. Here, we will try to figure out how clocks are described from within the device tree and related driver code. To achieve this, we'll need to consider the following device tree excerpt:
```c
clocks {
```
/* Provider node */
```c
clk54: clk54 {
dts
#clock-cells = <0>;
compatible = 'fixed-clock';
clock-frequency = <54000000>;
clock-output-names = 'osc';
c
};
};
```
[...]
```c
i2c0: i2c-master@d090000 {
```
[...]
/* Consumer node */
```c
cdce706: clock-synth@69 {
dts
compatible = 'ti,cdce706';
#clock-cells = <1>;
reg = <0x69>;
clocks = <&clk54>;
clock-names = 'clk_in0';
c
};
};
```
Keep in mind that clocks are assigned to consumers through the clocks property, and that a clock provider can be a consumer as well. In the preceding excerpt, clk54 is a fixed clock; we won't go into the details here. cdce706 is a clock provider that also consumes clk54 (given as a phandle in the clocks property).
158 Storming the Common Clock Framework
The most important piece of information that clock provider nodes need to specify is the
#clock- cells property, which determines the length of a clock specifier: when it is
```dts
0, this means that only the phandle property of this provider needs to be given to the consumer. When it is 1 (or greater), this means that the phandle property has multiple outputs and needs to be provided with additional information, such as an ID indicating what output needs to be used. This ID is directly represented by an immediate value. It is better to define the ID of all clocks in the system in a header file. The device tree can include this header file, such as clocks = <&clock CLK_SPI0>, where CLK_SPI0 is a macro defined in a header file.
Now, let's have a look at clock-output-names. This is an optional but recommended property and should be a list of strings that correspond to the names of the output (that is,
```
provided) clock lines.
Take a look at the following provider node excerpt:
```c
osc {
dts
#clock-cells = <1>;
clock-output-names = 'ckout1', 'ckout2';
c
};
dts
The preceding node defines a device that's providing two clock output lines named ckout1 and ckout2, respectively. Consumer nodes should never use these names directly to reference these clock lines. Instead, they should use an appropriate clock specifier (referencing clocks by index in respect to #clock-cells of the provider) that allows them to name their input clock line with respect to the device's needs:
c
device {
dts
clocks = <&osc 0>, <&osc 1>;
clock-names = 'baud', 'register';
c
};
```
This device consumes the two clock lines provided by osc and names its input lines according to its needs. We will discuss consumer nodes at the end of this chapter.
When a clock line is assigned to a consumer device and when this consumer's driver calls clk_get() (or similar interfaces that are used to grab a clock), this interface calls of_clk_get_by_name(), which, in turn, calls __of_clk_get(). The function of interest here is __of_clk_get(). It is defined in drivers/clk/clkdev.c as follows:
```c
static struct clk * of_clk_get(struct device_node *np,
int index,
```
CCF data structures and interfaces 159 const char *dev_id,
```c
const char *con_id)
{
struct of_phandle_args clkspec;
struct clk *clk;
int rc;
```
rc = of_parse_phandle_with_args(np, 'clocks',
```dts
'#clock-cells',
```
index, &clkspec);
```c
if (rc)
return ERR_PTR(rc);
```
clk = of_clk_get_from_provider(&clkspec, dev_id, con_id);
```c
of_node_put(clkspec.np);
return clk;
}
```
Important note
It is totally normal for this function to return a pointer to struct clk instead of a pointer to struct clk_hw as this interface operates from the consumer side.
The magic here comes from of_parse_phandle_with_args(), which parses lists of phandle and its arguments, and then calls __of_clk_get_from_provider(),
which we will describe later.
Understanding the of_parse_phandle_with_args() API
The following is the prototype of of_parse_phandle_with_args:
```c
int of_parse_phandle_with_args(const struct device_node *np,
const char *list_name,
const char *cells_name,
int index,
struct of_phandle_args *out_
```
args)
160 Storming the Common Clock Framework
This function returns 0 on success and fills out_args; it returns an appropriate errno value on error. Let's take a look at its arguments:
• np is a pointer to a device tree node containing a list. In our case, it will be the node corresponding to the consumer.
• list_name is the property name that contains a list. In our case, it is clocks.
• cells_name is the property name that specifies the argument count of phandle.
```dts
In our case, it is #clock-cells. It helps us grab an argument (other cells) after the phandle property in the specifier.
```
• index is the index of the phandle property and is used to parse out the list.
• out_args is an optional and output parameter that's filled on the success path.
This parameter is of the of_phandle_args type and is defined as follows:
```c
#define MAX_PHANDLE_ARGS 16 struct of_phandle_args {
struct device_node *np;
int args_count;
```
uint32_t args[MAX_PHANDLE_ARGS];
```c
};
```
In struct of_phandle_args, the np element is the pointer to the node that corresponds to the phandle property. In the case of the clock specifier, it will be the device tree node of the clock provider. The args_count element corresponds to the number of cells after the phandle in the specifier. It is can be used to walk through args,
which is an array containing the arguments in question.
Let's look at an example of using of_parse_phandle_with_args(), given the following DTS excerpt:
```c
phandle1: node1 {
```
#gpio-cells = <2>;
```c
};
phandle2: node2 {
```
#list-cells = <1>;
```c
};
node3 {
```
list = <&phandle1 1 2 &phandle2 3>;
```c
};
```
/* or */
```c
CCF data structures and interfaces 161 node3 {
```
list = <&phandle1 1 2>, <&phandle2 3>;
```c
}
```
Here, node3 is a consumer. To get a device_node pointer to the node2 node, you can call of_parse_phandle_with_args(node3, 'list', '#list-cells', 1,
&args);. Since &phandle2 is at index 1 (starting from 0) in the list, we specified 1 in the index parameter.
In the same way, to get the associated device_node of the node1 node, you can call of_parse_phandle_with_args(node3, 'list', '#gpio-cells', 0,
```c
&args);. For this second case, if we look at the args output parameter, we will see that args->np corresponds to node3, the value of args->args_count is 2 (as this specifier requires 2 parameters), the value of args->args[0] is 1, and the value of args->args[1] is 2, which would correspond to the 2 argument in the specifier.
```
Important note
For further reading about the device tree API, take a look at of_parse_
phandle_with_fixed_args() and the other interfaces provided by the device tree core code in drivers/of/base.c.
Understanding the __of_clk_get_from_provider() API
The next function call in __of_clk_get() is __of_clk_get_from_provider().
The reason why I'm providing its prototype is that you must not use this in your code.
However, this function simply walks through the clock providers (in the of_clk_
providers list) and when the appropriate provider is found, it calls the underlying callback given as the second parameter to of_clk_add_provider() to decode the underlying clock. Here, the clock specifier returned by of_parse_phandle_with_
args() is given as a parameter. As you may recall when you have to expose a clock provider to other devices, we had to use of_clk_add_hw_provider(). As a second parameter, this interface accepts a callback used by the CCF to decode the underlying clock whenever the consumer calls clk_get(). The structure of this callback is as follows:
```c
struct clk_hw *(*get_hw)(struct of_phandle_args *clkspec, void
```
*data)
162 Storming the Common Clock Framework
This callback should return the underlying clock_hw according to its parameters.
clkspec is the clock specifier returned by of_parse_phandle_with_args(),
while data is the context data given as the third parameter to of_clk_add_hw_
provider(). Remember, data is usually a pointer to the clock(s) to be associated with the node. To see how this callback is internally called, we need to have a look at the definition of the __of_clk_get_from_provider() interface, which is defined as follows:
```c
struct clk * of_clk_get_from_provider(struct of_phandle_args *clkspec,
const char *dev_id,
const char *con_id)
{
struct of_clk_provider *provider;
struct clk *clk = ERR_PTR(-EPROBE_DEFER);
struct clk_hw *hw;
if (!clkspec)
return ERR_PTR(-EINVAL);
```
/* Check if we have such a provider in our array */
```c
mutex_lock(&of_clk_mutex);
list_for_each_entry(provider, &of_clk_providers, link) {
if (provider->node == clkspec->np) {
```
hw = of_clk_get_hw_from_provider (provider, clkspec);
clk = clk_create_clk(hw, dev_id, con_id);
```c
}
if (!IS_ERR(clk)) {
if (! clk_get(clk)) {
clk_free_clk(clk);
```
clk = ERR_PTR(-ENOENT);
```c
}
```
break;
```c
}
}
mutex_unlock(&of_clk_mutex);
```
CCF data structures and interfaces 163 return clk;
```c
}
```
Clock decoding callbacks
If we had to summarize the mechanisms behind getting a clock from the CCF, we would say that, when a consumer calls clk_get(), the CCF internally calls __of_
```c
clk_get(). This is given as the first parameter of the device_node property of this consumer so that the CCF can grab the clock specifier and find the device_node property (by means of of_parse_phandle_with_args()) that corresponds to the provider. It then returns this in the form of of_phandle_args. This of_phandle_
```
args corresponds to the clock specifier and is given as a parameter to __of_clk_
```c
get_from_provider(), which simply compares the device_node property of the provider in of_phandle_args (that is, of_phandle_args->np) to those that exist in of_clk_providers, which is the list of device tree clock providers. Once a match is found, the corresponding of_clk_provider->get() callback of this provider is called and the underlying clock is returned.
```
Important note
If __of_clk_get() fails, this means there was no way to find a valid clock for the given device node. This may also mean that the provider did not register its clocks with the device tree interface. Therefore, when of_clk_get()
fails, the CCF code calls clk_get_sys(), which is a fall back to using a lookup for a clock based on its name that's not on the device tree anymore. This is the real logic behind clk_get().
```c
This of_clk_provider->get() callback often relies on the context data given as a parameter to of_clk_add_provider() so that the underlying clock is returned.
```
Though it is possible to write your own callback (which should respect the prototype that was already introduced in the previous section), the CCF framework provides two generic decoding callbacks that cover the majority of cases. These are of_clk_src_onecell_
get() and of_clk_src_simple_get(), and both have the same prototype:
```c
struct clk_hw *of_clk_hw_simple_get(struct of_phandle_args *clkspec,
void *data);
struct clk_hw *of_clk_hw_onecell_get(struct of_phandle_args *clkspec,
void *data);
```
164 Storming the Common Clock Framework of_clk_hw_simple_get() is used for simple clock providers, where no special context data structure except for the clock itself is needed, such as the clock-gpio driver
(in drivers/clk/clk-gpio.c). This callback simply returns the data given as a context data parameter as-is, meaning that this parameter should be a clock. It is defined in drivers/clk/clk.c as follows:
```c
struct clk_hw *of_clk_hw_simple_get(struct of_phandle_args *clkspec,
void *data)
{
return data;
}
EXPORT_SYMBOL_GPL(of_clk_hw_simple_get);
```
On the other hand, of_clk_hw_onecell_get() is a bit more complex as it requires a special data structure called struct clk_hw_onecell_data. This can be defined as follows:
```c
struct clk_hw_onecell_data {
unsigned int num;
struct clk_hw *hws[];
};
```
In the preceding structure, hws is an array of pointers to struct clk_hw, and num is the number of entries in this array.
Important note
In old clock provider drivers that do not implement clk_hw-based APIs yet, you may see struct clk_onecell_data, of_clk_add_
provider(), of_clk_src_onecell_get(), and of_clk_add_
provider() instead of the data structures and interfaces that have been introduced in this book.
That being said, to keep a hand on the clocks stored in this data structure, it is recommended to wrap them inside your context data structure, as shown in the following example from drivers/clk/sunxi/clk-sun9i-mmc.c:
```c
struct sun9i_mmc_clk_data {
```
spinlock_t lock;
```c
void __iomem *membase;
```
CCF data structures and interfaces 165 struct clk *clk;
```c
struct reset_control *reset;
struct clk_hw_onecell_data clk_hw_data;
struct reset_controller_dev rcdev;
};
```
You should then dynamically allocate space for these clocks according to the number of clocks that should be stored:
```c
int sun9i_a80_mmc_config_clk_probe(struct platform_device *pdev)
{
struct device_node *np = pdev->dev.of_node;
struct sun9i_mmc_clk_data *data;
struct clk_hw_onecell_data *clk_hw_data;
const char *clk_name = np->name;
const char *clk_parent;
struct resource *r;
```
[...]
```c
data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
if (!data)
return -ENOMEM;
clk_hw_data = &data->clk_hw_data;
clk_hw_data->num = count;
```
/* Allocating space for clk_hws, and 'count' is the number
*of entries
*/
```c
clk_hw_data->hws =
devm_kcalloc(&pdev->dev, count, sizeof(struct clk_hw *),
```
GFP_KERNEL);
```c
if (!clk_hw_data->hws)
return -ENOMEM;
```
/* A clock provider may be a consumer from another
* provider as well
*/
```c
data->clk = devm_clk_get(&pdev->dev, NULL);
clk_parent = __clk_get_name(data->clk);
for (i = 0; i < count; i++) {
dts
166 Storming the Common Clock Framework of_property_read_string_index(np, 'clock-output-names',
```
i, &clk_name);
/* storing each clock in its location */
```c
clk_hw_data->hws[i] =
clk_hw_register_gate(&pdev->dev, clk_name,
clk_parent, 0,
data->membase + SUN9I_MMC_WIDTH * i,
SUN9I_MMC_GATE_BIT, 0, &data->lock);
if (IS_ERR(clk_hw_data->hws[i])) {
ret = PTR_ERR(clk_hw_data->hws[i]);
```
goto err_clk_register;
```c
}
}
```
ret =
```c
of_clk_add_hw_provider(np, of_clk_hw_onecell_get,
clk_hw_data);
if (ret)
```
goto err_clk_provider;
[...]
```c
return 0;
}
```
Important note
At the time of writing, the preceding excerpt, which has been taken from the sunxi A80 SoC MMC config clocks/resets driver, still use the clk-based API
(along with the struct clk, clk_register_gate(), and of_
```c
clk_add_src_provider() interfaces) instead of the clk_hw one.
```
Therefore, for learning purposes, I've modified this excerpt so that it uses the recommended clk_hw API.
CCF data structures and interfaces 167
As you can see, the context data that's given during clock registration is clk_hw_data,
which is of the clk_hw_onecell_data type. Moreover, of_clk_hw_onecell_get is given as a clock decoder callback function. This helper simply returns the clock at the index that was given as an argument in the clock specifier (which is of the of_phandle_
args type). Take a look at its definition to get a better understanding:
```c
struct clk_hw * of_clk_hw_onecell_get(struct of_phandle_args *clkspec,
void *data)
{
struct clk_hw_onecell_data *hw_data = data;
unsigned int idx = clkspec->args[0];
if (idx >= hw_data->num) {
pr_err('%s: invalid index %u\n', func , idx);
return ERR_PTR(-EINVAL);
}
return hw_data->hws[idx];
}
EXPORT_SYMBOL_GPL(of_clk_hw_onecell_get);
```
Of course, depending on your needs, feel free to implement your own decoder callback,
similar to the one in the max9485 audio clock generator, whose driver is drivers/
clk/clk-max9485.c in the kernel source's tree.
In this section, we have learned about the device tree aspects of clock providers. We have learned how to expose a device's clock source lines, as well as how to assign those clock lines to consumers. Now, the time has come to introduce the driver side, which also consists of writing code for its clock providers.
168 Storming the Common Clock Framework
## Writing a clock provider driver
While the purpose of a device tree is to describe the hardware at hand (the clock provider,
in this case), it is worth noting that the code used to manage the underlying hardware needs to be written. This section deals with writing code for clock providers so that once their clock lines have been assigned to consumers, they behave the way they were designed to. When writing clock device drivers, it is a good practice to embed the full struct clk_hw (not a pointer) into your private and bigger data structure, since it is given as the first parameter to each callback in clk_ops. This lets you define a custom to_<my-data-structure> helper upon the container_of macro, which gives you back a pointer to your private data structure, as follows:
/* forward reference */
```c
struct max9485_driver_data;
struct max9485_clk_hw {
struct clk_hw hw;
struct clk_init_data init;
```
u8 enable_bit;
```c
struct max9485_driver_data *drvdata;
```
;
```c
struct max9485_driver_data {
struct clk *xclk;
struct i2c_client *client;
```
u8 reg_value;
```c
struct regulator *supply;
struct gpio_desc *reset_gpio;
struct max9485_clk_hw hw[MAX9485_NUM_CLKS];
};
static inline struct max9485_clk_hw *to_max9485_clk(struct clk_hw *hw)
{
return container_of(hw, struct max9485_clk_hw, hw);
}
```
Writing a clock provider driver 169
In the preceding example, max9485_clk_hw abstracts the hw clock (as it contains struct clk_hw). Now, from the driver's point of view, each struct max9485_
```c
clk_hw represents a hw clock, allowing us to define another bigger structure that will be used as the driver data this time: the max9485_driver_data struct. You will notice some cross-referencing in the preceding structures, notably in struct max9485_
clk_hw, which contains a pointer to struct max9485_driver_data, and struct max9485_driver_data, which contains a max9485_clk_hw array. This allows us to grab the driver data from within any clk_ops callback, as follows:
static unsigned long max9485_clkout_recalc_rate(struct clk_hw *hw,
unsigned long parent_rate)
{
struct max9485_clk_hw *max_clk_hw = to_max9485_clk(hw);
struct max9485_driver_data *drvdata = max_clk_hw->drvdata;
```
[...]
```c
return 0;
}
```
Moreover, as shown in the following excerpt, it is a good practice to statically declare the clock lines (abstracted by max9485_clk_hw in this case), as well as the associated ops.
This is because, unlike private data (which may change from one device to another), this information never changes, regardless of the number of clock chips of the same type that are present on the system:
```c
static const struct max9485_clk max9485_clks[MAX9485_NUM_CLKS] = {
[MAX9485_MCLKOUT] = {
```
.name = 'mclkout',
.parent_index = -1,
.enable_bit = MAX9485_MCLK_ENABLE,
```c
.ops = {
```
.prepare = max9485_clk_prepare,
.unprepare = max9485_clk_unprepare,
```c
},
},
[MAX9485_CLKOUT] = {
```
.name = 'clkout',
170 Storming the Common Clock Framework
.parent_index = -1,
```c
.ops = {
```
.set_rate = max9485_clkout_set_rate,
.round_rate = max9485_clkout_round_rate,
.recalc_rate = max9485_clkout_recalc_rate,
```c
},
},
[MAX9485_CLKOUT1] = {
```
.name = 'clkout1',
.parent_index = MAX9485_CLKOUT,
.enable_bit = MAX9485_CLKOUT1_ENABLE,
```c
.ops = {
```
.prepare = max9485_clk_prepare,
.unprepare = max9485_clk_unprepare,
```c
},
},
[MAX9485_CLKOUT2] = {
```
.name = 'clkout2',
.parent_index = MAX9485_CLKOUT,
.enable_bit = MAX9485_CLKOUT2_ENABLE,
```c
.ops = {
```
.prepare = max9485_clk_prepare,
.unprepare = max9485_clk_unprepare,
```c
},
},
};
```
Though ops are embedded in the abstraction data structure, they could have been declared separately, as in the drivers/clk/clk-axm5516.c file in the kernel sources.
On the other hand, it is better to dynamically allocate the driver data structure as it would be easier for it to be private to the driver, thus allowing private data per declared device, as shown in the following excerpt:
```c
static int max9485_i2c_probe(struct i2c_client *client,
const struct i2c_device_id *id)
{
struct max9485_driver_data *drvdata;
Writing a clock provider driver 171 struct device *dev = &client->dev;
const char *xclk_name;
int i, ret;
```
drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
```c
if (!drvdata)
return -ENOMEM;
```
[...]
```c
for (i = 0; i < MAX9485_NUM_CLKS; i++) {
int parent_index = max9485_clks[i].parent_index;
const char *name;
if (of_property_read_string_index
dts
(dev->of_node, 'clock-output-names', i, &name) == 0)
c
{
drvdata->hw[i].init.name = name;
} else {
drvdata->hw[i].init.name = max9485_clks[i].name;
}
drvdata->hw[i].init.ops = &max9485_clks[i].ops;
drvdata->hw[i].init.num_parents = 1;
drvdata->hw[i].init.flags = 0;
if (parent_index > 0) {
drvdata->hw[i].init.parent_names =
&drvdata->hw[parent_index].init.name;
drvdata->hw[i].init.flags |= CLK_SET_RATE_PARENT;
} else {
drvdata->hw[i].init.parent_names = &xclk_name;
}
drvdata->hw[i].enable_bit = max9485_clks[i].enable_bit;
drvdata->hw[i].hw.init = &drvdata->hw[i].init;
drvdata->hw[i].drvdata = drvdata;
ret = devm_clk_hw_register(dev, &drvdata->hw[i].hw);
```
172 Storming the Common Clock Framework if (ret < 0)
```c
return ret;
}
return devm_of_clk_add_hw_provider(dev, max9485_of_clk_get,
```
drvdata);
```c
}
```
In the preceding excerpt, the driver calls clk_hw_register() (this is actually devm_
```c
clk_hw_register(), which is the managed version) in order to register each clock with the CCF. Now that we have looked at the basics of a clock provider driver, we will learn how to allow interactions with the clock line thanks to a set of operations that can be exposed in the driver.
```
Providing clock ops struct clk_hw is the base hardware clock structure on top of which the CCF builds other clock variant structures. As a quick callback, the common clock framework provides the following base clocks:
• fixed-rate: This type of clock can't have its rate changed and is always running.
• gate: This acts as a gate to a clock source as is its parent. Obviously, it can't have its rate changed as it is just a gate.
• mux: This type of clock cannot gate. It has two or more clock inputs: its parents. It allows us to select a parent among those it is connected to. Moreover, it allows us to get the rate from the selected parent.
• fixed-factor: This clock type can't gate/ungate but does divide and multiply the parent rate by its constants.
• divider: This type of clock cannot gate/ungate. However, it divides the parent clock rate by using a divider that can be selected from among the various arrays that are provided at registration.
• composite: This is a combination of three of the base clocks we described earlier:
mux, rate, and gate. It allows us to reuse those base clocks to build a single clock interface.
Writing a clock provider driver 173
You may be wondering how the kernel (that is, the CCF) knows what the type of a given clock is when giving clk_hw as a parameter to the clk_hw_register() function.
```c
Actually, the CCF does not know this, and does not have to know anything. This is the aim of the clk_hw->init.ops field, which is of the struct clk_ops type. According to the callback functions set in this structure, you can guess what type of clock it is facing.
```
The following is a detailed presentation of this set of operation functions for the clock in a struct clk_ops:
```c
struct clk_ops {
int (*prepare)(struct clk_hw *hw);
void (*unprepare)(struct clk_hw *hw);
int (*is_prepared)(struct clk_hw *hw);
void (*unprepare_unused)(struct clk_hw *hw);
int (*enable)(struct clk_hw *hw);
void (*disable)(struct clk_hw *hw);
int (*is_enabled)(struct clk_hw *hw);
void (*disable_unused)(struct clk_hw *hw);
unsigned long (*recalc_rate)(struct clk_hw *hw,
unsigned long parent_rate);
```
long (*round_rate)(struct clk_hw *hw, unsigned long rate,
```c
unsigned long *parent_rate);
int (*determine_rate)(struct clk_hw *hw,
struct clk_rate_request *req);
int (*set_parent)(struct clk_hw *hw, u8 index);
```
u8 (*get_parent)(struct clk_hw *hw);
```c
int (*set_rate)(struct clk_hw *hw, unsigned long rate,
unsigned long parent_rate);
```
[...]
```c
void (*init)(struct clk_hw *hw);
};
```
For clarity, some fields have been removed.
174 Storming the Common Clock Framework
Each prepare*/unprepare*/is_prepared callback is allowed to sleep and therefore must not be called from an atomic context, while each enable*/disable*/is_
enabled callback may not — and must not – sleep. Let's take a look at this code in more detail:
• prepare and unprepare are optional callbacks. What has been done in prepare should be undone in unprepare.
• is_prepared is an optional callback that tells is whether the clock is prepared or not by querying the hardware. If omitted, the clock framework core will do the following:
--Maintain a prepare counter (incremented by one when the clk_prepare()
consumer API is called, and decremented by one when clk_unprepare() is called).
--Based on this counter, it will determine whether the clock is prepared.
• unprepare_unused/disable_unused: These callbacks are optional and used in the clk_disable_unused interface only. This interface is provided by the clock framework core and called (in drivers/clk/clk.c: late_initcall_
sync(clk_disable_unused)) in the system-initiated late call in order to unprepare/ungate/close unused clocks. This interface will call the corresponding
.unprepare_unused and .disable_unused functions of each unused clock on the system.
• enable/disable: Enables/disables the clock atomically. These functions must run atomically and must not sleep. For enable, for example, it should return only when the underlying clock is generating a valid clock signal that can be used by consumer nodes.
• is_enabled has the same logic as is_prepared.
• recalc_rate: This is an optional callback that queries the hardware to recalculate the rate of the underlying clock, given the parent rate as an input parameter. The initial rate is 0 if this op is omitted.
• round_rate: This callback accepts a target rate (in Hz) as input and should return the closest rate actually supported by the underlying clock. The parent rate is an input/output parameter.
• determine_rate: This callback is given a targeted clock rate as a parameter and returns the closest one supported by the underlying hardware.
Writing a clock provider driver 175
• set_parent: This concerns clocks with multiple inputs (multiple possible parents). This callback accepts changing the input source when given the index as a parameter (as a u8) of the parent to be selected. This index should correspond to a parent that's valid in either the clk_init_data.parent_names or clk_
init_data.parents arrays of the clock. This callback should return 0 on a success path or -EERROR otherwise.
• get_parent is a mandatory callback for clocks with multiple (at least two) inputs
(multiple parents). It queries the hardware to determine the parent of the clock.
The return value is a u8 that corresponds to the parent index. This index should be valid in either the clk_init_data.parent_names or clk_init_data.
parents arrays. In other words, this callback translates the parent value that's read from the hardware into an array index.
• set_rate: Changes the rate of the given clock. The requested rate should be the return value of the .round_rate call in order to be valid. This callback should return 0 on a success path or -EERROR otherwise.
• init is a platform-specific clock initialization hook that will be called when the clock is registered to the kernel. For now, no basic clock type implements this callback.
Tip
Since .enable and .disable must not sleep (they are called with spinlocks held), clock providers in discrete chips that are connected to sleepable buses (such as SPI or I2C) cannot be controlled with spinlocks held and should therefore implement their enable/disable logic in the prepare/
unprepare hooks. The general API will directly call the corresponding operation function. This is one of the reasons why, from the consumer side
(the clk-based API), a call to clk_enable must be preceded by a call to clk_prepare() and a call to clock_disable() should be followed by clock_unprepare().
Last but not the least, the following difference should be noticed as well:
Important note
SoC-internal clocks can be seen as fast clocks (controlled via simple MMIO
register writes), and can therefore implement .enable and .disable,
while SPI/I2C-based clocks can be seen as slow clocks and should implement
.prepare and .unprepare.
176 Storming the Common Clock Framework
These functions are not mandatory for all clocks. Depending on the clock type, some may be mandatory, while others may not be. The following array summarizes which clk_ops callbacks are mandatory for which clock type, based on their hardware capabilities:
Figure 4.1 – Mandatory clk_ops callbacks for clock types
Writing a clock provider driver 177
In the preceding array, the ** marker means either round_rate or determine_rate is required.
In the preceding array, y means mandatory, while n means the concerned callback is either invalid or otherwise unnecessary. Empty cells should be considered as either optional or that they must be evaluated on a case-by-case basis.
Clock flags in clk_hw.init.flags
Since we have already introduced the clock ops structure, we will now introduce the different flags (defined in include/linux/clk-provider.h) and see how they affect the behavior of some of the callbacks in this structure:
/*must be gated across rate change*/
```c
#define CLK_SET_RATE_GATE BIT(0)
```
/*must be gated across re-parent*/
```c
#define CLK_SET_PARENT_GATE BIT(1)
```
/*propagate rate change up one level */
```c
#define CLK_SET_RATE_PARENT BIT(2)
```
/* do not gate even if unused */
```c
#define CLK_IGNORE_UNUSED BIT(3)
```
/*Basic clk, can't do a to_clk_foo()*/
```c
#define CLK_IS_BASIC BIT(5)
```
/*do not use the cached clk rate*/
```c
#define CLK_GET_RATE_NOCACHE BIT(6)
```
/* don't re-parent on rate change */
```c
#define CLK_SET_RATE_NO_REPARENT BIT(7)
```
/* do not use the cached clk accuracy */
```c
#define CLK_GET_ACCURACY_NOCACHE BIT(8)
```
/* recalc rates after notifications */
```c
#define CLK_RECALC_NEW_RATES BIT(9)
```
/* clock needs to run to set rate */
```c
#define CLK_SET_RATE_UNGATE BIT(10)
```
/* do not gate, ever */
```c
#define CLK_IS_CRITICAL BIT(11)
```
178 Storming the Common Clock Framework
The preceding code shows the different framework-level flags that can be set in the clk_
```c
hw->init.flags field. You can specify multiple flags by OR'ing them. Let's take a look at them in more detail:
```
• CLK_SET_RATE_GATE: When you change the rate of the clock, it must be gated
(disabled). This flag also ensures there's rate change and rate glitch protection; when a clock has the CLK_SET_RATE_GATE flag set and it has been prepared, the clk_
set_rate() request will fail.
• CLK_SET_PARENT_GATE : When you change the parent of the clock, it must be gated.
• CLK_SET_RATE_PARENT: Once you've changed the rate of the clock, the change must be passed to the upper parent. This flag has two effects:
--When a clock consumer calls clk_round_rate() (which the CCF internally maps to .round_rate) to get an approximate rate, if the clock does not provide the .round_rate callback, the CCF will immediately return the cached rate of the clock if CLK_SET_RATE_PARENT is not set. However, if this flag is set still without
.round_rate provided, then the request is routed to the clock parent. This means the parent is queried and clk_round_rate() is called to get the value that the parent clock can provide that's closest to the targeted rate.
--This flag also modifies the behavior of the clk_set_rate() interface (which the CCF internally maps to .set_rate). If set, any rate change request will be forwarded upstream (passed to the parent clock).
That is to say, if the parent clock can get an approximate rate value, then by changing the parent clock rate, you can get the required rate. This flag is usually set on the clock gate and mux. Use this flag with care.
• CLK_IGNORE_UNUSED: Ignore the disable unused call. This is primarily useful when there's a driver that doesn't claim clocks properly, but the bootloader leaves them on. It is the equivalent of the clk_ignore_unused kernel boot parameters but for a single clock. It's not expected to be used in normal cases, but for bring up and debug, it's very useful to have the option to not gate (not disable) unclaimed clocks that are still on.
• CLK_IS_BASIC: This is no longer used.
• CLK_GET_RATE_NOCACHE: There are chips where the clock rate can be changed by internal hardware without the Linux clock framework being aware of that change at all. This flag makes sure the clk rate from the Linux clock tree always matches the hardware settings. In other words, the get/set rate does not come from the cache and is calculated at the time.
Writing a clock provider driver 179
Important note
While dealing with the gate clock type, note that a gated clock is a disabled clock, while an ungated clock is an enabled clock. See https://elixir.
bootlin.com/linux/v4.19/source/drivers/clk/
clk.c#L931 and https://elixir.bootlin.com/linux/
v4.19/source/drivers/clk/clk.c#L862 for more details.
Now that we are familiar with clock flags, as well as the way those flags may modify the behavior of clock-related callbacks, we can walk through each clock type and learn how to provide their associated ops.
Fixed-rate clock case study and its ops
This is the simplest type of clock. Therefore, we will use this to build some of the strong guidelines we must respect while writing clock drivers. The frequency of this type of clock cannot be adjusted as it is fixed. Moreover, this type of clock cannot be switched, cannot choose its parent, and does not need to provide a clk_ops callback function.
The clock framework uses the struct clk_fixed_rate structure (described as follows) to abstract this type of clock hardware:
```c
Struct clk_fixed_rate {
struct clk_hw hw;
unsigned long fixed_rate;
```
u8 flags; [...]
```c
};
#define to_clk_fixed_rate(_hw) \
container_of(_hw, struct clk_fixed_rate, hw)
```
In the preceding structure, hw is the base structure and ensures there's a link between the common and hardware-specific interfaces. Once given to the to_clk_fixed_rate macro (which is based on container_of), you should get a pointer to clk_fixed_
rate, which wraps this hw. fixed_rate is the constant (fixed) rate of the clock device.
flags represents framework-specific flags.
Let's have a look at the following excerpt, which simply registers two fake fixed-rate clock lines:
```c
#include <linux/clk.h>
#include <linux/clk-provider.h>
```
180 Storming the Common Clock Framework
```c
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
static struct clk_fixed_rate clk_hw_xtal = {
```
.fixed_rate = 24000000,
```c
.hw.init = &(struct clk_init_data){
```
.name = 'xtal',
.num_parents = 0,
.ops = &clk_fixed_rate_ops,
```c
},
};
static struct clk_fixed_rate clk_hw_pll = {
```
.fixed_rate = 45000000,
```c
.hw.init = &(struct clk_init_data){
```
.name = 'fixed_pll',
.num_parents = 0,
.ops = &clk_fixed_rate_ops,
```c
},
};
static struct clk_hw_onecell_data fake_fixed_hw_onecell_data =
{
.hws = {
```
[CLKID_XTAL] = &clk_hw_xtal.hw,
[CLKID_PLL_FIXED] = &clk_hw_pll.hw,
[CLK_NR_CLKS] = NULL,
```c
},
```
.num = CLK_NR_CLKS,
```c
};
```
Writing a clock provider driver 181
With that, we have defined our clocks. The following code shows how to register these clocks on the system:
```c
static int fake_fixed_clkc_probe(struct platform_device *pdev)
{
int ret, i;
struct device *dev = &pdev->dev;
for (i = CLKID_XTAL; i < CLK_NR_CLKS; i++) {
```
ret = devm_clk_hw_register(dev,
fake_fixed_hw_onecell_data.hws[i]);
```c
if (ret)
return ret;
}
return devm_of_clk_add_hw_provider(dev,
of_clk_hw_onecell_get,
```
&fake_fixed_hw_onecell_data);
```c
}
static const struct of_device_id fake_fixed_clkc_match_table[] = {
dts
{ .compatible = 'l.abcsmart,fake-fixed-clkc' },
c
{ }
};
static struct platform_driver meson8b_driver = {
```
.probe = fake_fixed_clkc_probe,
```c
.driver = {
```
.name = 'fake-fixed-clkc',
.of_match_table = fake_fixed_clkc_match_table,
```c
},
};
```
182 Storming the Common Clock Framework
General simplification considerations
In the previous excerpt, we used clk_hw_register() to register the clock. This interface is the base registration interface and can be used to register any type of clock. Its main parameter is a pointer to the struct clk_hw structure that's embedded in the underlying clock-type structure.
Clock initialization and registration through a call to clk_hw_register() requires populating the struct clk_init_data (thus implementing clk_ops) object, which gets bundled with clk_hw. As an alternative, you can use a hardware-specific (that is,
clock-type-dependent) registration function. Here, the kernel is responsible for building the appropriate init data from arguments given to the function according to the clock type, before internally calling clk_hw_register(...). With this alternative, the CCF
will provide appropriate clk_ops according to the clock hardware type.
Generally, the clock provider does not need to use nor allocate the base clock type directly,
which in this case is struct clk_fixed_rate. This is because the kernel clock framework provides dedicated interfaces for this purpose. In a real-life scenario (where there's a fixed clock), this dedicated interface would be clk_hw_register_fixed_
rate():
```c
struct clk_hw *
clk_hw_register_fixed_rate(struct device *dev,
const char *name,
const char *parent_name,
unsigned long flags,
unsigned long fixed_rate)
```
The clk_register_fixed_rate() interface uses the clock's name, parent_
name, and fixed_rate as parameters to create a clock with a fixed frequency. flags represents the framework-specific flags, while dev is the device that is registering the clock. The clk_ops property of the clock is also provided by the clock framework and does not require the provider to care about it. The kernel clock ops data structure for this kind of clock is clk_fixed_rate_ops. It is defined in drivers/clk/clk-fixedrate.c as follows:
```c
static unsigned long clk_fixed_rate_recalc_rate(struct clk_hw *hw,
unsigned long parent_rate)
{
return to_clk_fixed_rate(hw)->fixed_rate;
}
```
Writing a clock provider driver 183 static unsigned long clk_fixed_rate_recalc_accuracy(struct clk_hw *hw,
```c
unsigned long parent_ accuracy)
{
return to_clk_fixed_rate(hw)->fixed_accuracy;
}
const struct clk_ops clk_fixed_rate_ops = {
```
.recalc_rate = clk_fixed_rate_recalc_rate,
.recalc_accuracy = clk_fixed_rate_recalc_accuracy,
```c
};
clk_register_fixed_rate() returns a pointer to the underlying clk_hw structure of the fixed-rate clock. The code can then use the to_clk_fixed_rate macro the grab a pointer to the original clock-type structure.
```
However, you can still use the low-level clk_hw_register() registration interface and reuse some of the CCF provided ops callbacks. The fact that the CCF provides an appropriate ops structure for your clock does not mean you should use it as-is. You may not wish to use the clock-type-dependent registration interface (using clock_hw_
register() instead) and instead use one or more of the individual ops provided by the
CCF. This does not just apply to adjustable clocks, as per the following example, but to all other clock types that we will discuss in this book.
Let's have a look at an example from drivers/clk/clk-stm32f4.c for a clock divider driver:
```c
static unsigned long stm32f4_pll_div_recalc_rate(
struct clk_hw *hw,
unsigned long parent_rate)
{
return clk_divider_ops.recalc_rate(hw, parent_rate);
}
static long stm32f4_pll_div_round_rate(struct clk_hw *hw,
unsigned long rate,
unsigned long *prate)
{
return clk_divider_ops.round_rate(hw, rate, prate);
```
184 Storming the Common Clock Framework
```c
}
static int stm32f4_pll_div_set_rate(struct clk_hw *hw,
unsigned long rate,
unsigned long parent_rate)
{
int pll_state, ret;
struct clk_divider *div = to_clk_divider(hw);
struct stm32f4_pll_div *pll_div = to_pll_div_clk(div);
pll_state = stm32f4_pll_is_enabled(pll_div->hw_pll);
if (pll_state)
stm32f4_pll_disable(pll_div->hw_pll);
```
ret = clk_divider_ops.set_rate(hw, rate, parent_rate);
```c
if (pll_state)
stm32f4_pll_enable(pll_div->hw_pll);
return ret;
}
static const struct clk_ops stm32f4_pll_div_ops = {
```
.recalc_rate = stm32f4_pll_div_recalc_rate,
.round_rate = stm32f4_pll_div_round_rate,
.set_rate = stm32f4_pll_div_set_rate,
```c
};
```
In the preceding excerpt, the driver only implements the .set_rate ops and reuses the
.recalc_rate and .round_rate properties of the CCF-provided clock divider ops known as clk_divider_ops.
Writing a clock provider driver 185
Fixed clock device binding
This type of clock can also be natively and directly supported by DTS configuration without the need to write any code. This device tree-based interface is generally used to provide dummy clocks. There are cases where some devices in the device tree may require clock nodes to describe their own clock inputs. For example, the mcp2515 SPI to CAN
converter needs to be provided with a clock to let it know the frequency of the quartz it is connected to. For such a dummy clock node, the compatible property should be fixedclock. An example of this is as follows:
/* fixed crystal dedicated to mpc251x */
```c
clocks {
```
/* fixed crystal dedicated to mpc251x */
```c
clk8m: clk@1 {
dts
compatible = 'fixed-clock';
```
reg=<0>;
```dts
#clock-cells = <0>;
clock-frequency = <8000000>;
clock-output-names = 'clk8m';
c
};
};
```
/* consumer */
```c
can1: can@1 {
dts
compatible = 'microchip,mcp2515';
reg = <0>;
```
spi-max-frequency = <10000000>;
```dts
clocks = <&clk8m>;
c
};
```
The clock framework's core will directly extract the clock information provided by DTS
and will automatically register it to the kernel without any driver support. #clockcells is 0 here because only one fixed rate line is provided, and in this case, the specifier only needs to be the phandle of the provider.
186 Storming the Common Clock Framework
PWM clock alternative
Because of the lack of output clock sources (clock pads), some board designers (rightly or wrongly) use PWM output pads as the clock source for external components. This kind of clock is only instantiated from the device tree. Moreover, since PWM binding requires specifying the period of the PWM signal, pwm-clock falls into the fixed-rate clock category. An example of such an instantiation can be seen in the following code, which is an excerpt from imx6qdl-sabrelite.dtsi:
```c
mipi_xclk: mipi_xclk {
dts
compatible = 'pwm-clock';
#clock-cells = <0>;
clock-frequency = <22000000>;
clock-output-names = 'mipi_pwm3';
pwms = <&pwm3 0 45>; /* 1 / 45 ns = 22 MHz */
status = 'okay';
c
};
ov5640: camera@40 {
dts
compatible = 'ovti,ov5640';
```
pinctrl-names = 'default';
pinctrl-0 = <&pinctrl_ov5640>;
```dts
reg = <0x40>;
clocks = <&mipi_xclk>;
clock-names = 'xclk';
```
DOVDD-supply = <&reg_1p8v>;
AVDD-supply = <&reg_2p8v>;
DVDD-supply = <&reg_1p5v>;
```dts
reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
powerdown-gpios = <&gpio6 9 GPIO_ACTIVE_HIGH>;
```
[...]
```c
};
```
As you can see, the compatible property should be pwm-clock, while #clockcells should be <0>. This clock-type driver is located at drivers/clk/clk-pwm.c,
and further reading about this can be found at Documentation/devicetree/
bindings/clock/pwm-clock.txt.
Writing a clock provider driver 187
Fixed-factor clock driver and its ops
This type of clock divides and multiplies the parent rate by constants (hence it being a fixed-factor clock driver). This clock cannot gate:
```c
struct clk_fixed_factor {
struct clk_hw hw;
unsigned int mult;
unsigned int div;
};
#define to_clk_fixed_factor(_hw) \
container_of(_hw, struct clk_fixed_factor, hw)
```
The frequency of the clock is determined by the frequency of the parent clock, multiplied by mult, and then divided by div. It is actually a fixed multiplier and divider clock. The only way for a fixed-factor clock to have its rate changed would be to change its parent rate. In this case, you need to set the CLK_SET_RATE_PARENT flag. Since the frequency of the parent clock can be changed, the fixed-factor clock can also have its frequency changed, so callbacks such as .recalc_rate/.set_rate/.round_rate are also provided. That being said, since the set rate request will be propagated upstream if the
CLK_SET_RATE_PARENT flag is set, the .set_rate callback of such a clock needs to return 0 to ensure its call is a valid nop (no-operation):
```c
static int clk_factor_set_rate(struct clk_hw *hw,
unsigned long rate,
unsigned long parent_rate)
{
return 0;
}
```
For such clocks, you're better off using the clock framework provider helper ops known as clk_fixed_factor_ops, which is defined and implemented in drivers/clk/
clk-fixed-factor.c as follows:
```c
const struct clk_ops clk_fixed_factor_ops = {
```
.round_rate = clk_factor_round_rate,
.set_rate = clk_factor_set_rate,
.recalc_rate = clk_factor_recalc_rate,
```c
};
EXPORT_SYMBOL_GPL(clk_fixed_factor_ops);
```
188 Storming the Common Clock Framework
The advantage of using this is that you don't need to care about ops anymore since the kernel has already set everything up for you. Its round_rate and recalc_rate callbacks even take care of the CLK_SET_RATE_PARENT flag, which means we can adhere to our simplification path. Moreover, you're better off using the clock framework helper interface to register such a clock; that is, clk_hw_register_fixed_
factor():
```c
struct clk_hw *
clk_hw_register_fixed_factor(struct device *dev,
const char *name,
const char *parent_name,
unsigned long flags,
unsigned int mult,
unsigned int div)
```
This interface internally sets up a struct clk_fixed_factor that it allocates dynamically, and then returns a pointer to the underlying struct clk_hw. You can use this with the to_clk_fixed_factor macro to grab a pointer to the original fixedfactor clock structure. The ops that's assigned to the clock is clk_fixed_factor_ops,
as discussed previously. In addition, this type of interface is similar to the fixed-rate clock.
You do not need to provide a driver. You only need to configure the device tree.
Device tree binding for fixed-factor clocks
You can find binding documentation for such simple fixed factor rate clocks at
Documentation/devicetree/bindings/clock/fixed-factor-clock.txt,
in the kernel sources. The required properties are as follows:
```dts
• #clock-cells: This will be set to 0 according to the common clock binding.
```
• compatible: This will be 'fixed-factor-clock'.
• clock-div: Fixed divider.
• clock-mult: Fixed multiplier.
• clocks: The phandle of the parent clock.
Writing a clock provider driver 189
Here's an example:
```c
clock {
dts
compatible = 'fixed-factor-clock';
clocks = <&parentclk>;
#clock-cells = <0>;
```
clock-div = <2>;
clock-mult = <1>;
```c
};
```
Now that the fixed-factor clock has been addressed, the next logical step would be to look at the gateable clock, another simple clock type.
Gateable clock and its ops
This type of clock can only be switched, so only providing .enable/.disable callbacks makes sense here:
```c
struct clk_gate {
struct clk_hw hw;
void __iomem *reg;
```
u8 bit_idx;
u8 flags;
spinlock_t *lock;
```c
};
#define to_clk_gate(_hw) container_of(_hw, struct clk_gate, hw)
```
Let's take a look at the preceding structure in more detail:
• reg: This represents the register address (virtual address; that is, MMIO) for controlling the clock switch.
• bit_idx: This is the control bit of the clock switch (this can be 1 or 0 and sets the state of the gate).
190 Storming the Common Clock Framework
• clk_gate_flags: This represents the gate-specific flags of the gate clock. These are as follows:
--CLK_GATE_SET_TO_DISABLE: This is the clock switch's control mode. If set,
writing 1 turns off the clock, and writing 0 turns on the clock.
--CLK_GATE_HIWORD_MASK: Some registers use the concept of readingmodifying-writing to operate at the bit level, while other registers only support hiword mask. Hiword mask is a concept in which (in a 32-bit register)
changing a bit at a given index (between 0 and 15) consists of changing the corresponding bit in the 16 lower bits (0 to 15) and masking the same bit index in the 16 higher bits (16 to 31, hence hiword or High Word) in order to indicate/
validate the change.
For example, if bit b1 needs to be set as a gate, it also needs to indicate the change by setting the hiword mask (b1 << 16). This means that the gate settings are truly in the lower 16 bits of the register, while the mask of gate bits is in the higher 16 bits of this same register. When setting this flag, bit_idx should be no higher than 15.
• lock: This is the spinlock that should be used if the clock switch requires mutual exclusion.
As you have probably guessed, this structure assumes that the clock gate register is mmio.
As for the previous clock type, it is better to use the provided kernel interface to deal with such a clock; that is, clk_hw_register_gate():
```c
struct clk_hw *
clk_hw_register_gate(struct device *dev, const char *name,
const char *parent_name,
unsigned long flags,
void __iomem *reg, u8 bit_idx,
```
u8 clk_gate_flags, spinlock_t *lock);
Some of the parameters of this interface are the same ones we described regarding the clock-type structure. Moreover, the following are extra arguments that need to be described:
• dev is the device that is registering the clock.
• name is the name of the clock.
• parent_name is the name of the parent clock, which should be NULL if it has no parent.
Writing a clock provider driver 191
• flags represents the framework-specific flags for this clock. It is common to set the CLK_SET_RATE_PARENT flag for gate clocks that have a parent so that rate change requests are propagated up one level.
• clk_gate_flags corresponds to the .flags in the clock-type structure.
This interface returns a pointer to the underlying struct clh_hw of the clock gate structure. Here, you can use the to_clk_gate helper macro to grab the original clock gate structure.
While setting up this clock and prior to its registration, the clock framework assigns the clk_gate_ops ops to it. This is actually the default ops for the gate clock. It relies on the fact that the clock is controlled through mmio registers:
```c
const struct clk_ops clk_gate_ops = {
```
.enable = clk_gate_enable,
.disable = clk_gate_disable,
.is_enabled = clk_gate_is_enabled,
```c
};
EXPORT_SYMBOL_GPL(clk_gate_ops);
```
The entire gate clock API is defined in drivers/clk/clk-gate.c. Such a clock driver can be found in drivers/clk/clk-asm9260.c, while its device tree binding can be found in Documentation/devicetree/bindings/clock/alphascale,acc.
txt, in the kernel source tree.
I2C/SPI-based gate clock
Not just mmio peripherals can provide gate clocks. There are also discrete chips behind
I2C/SPI buses that can provide such clocks. Obviously, you cannot rely on the structure
(struct clk_gate) or the interface helper (clk_hw_register_gate()) that we introduced earlier to develop drivers for such chips. The main reasons for this are as follows:
• The aforementioned interface and data structure assume that the clock gate register control is mmio, which is definitely not the case here.
• The standard gate clock ops are .enable and .disable. However, these callbacks don't need to sleep as they are called with spinlocks held, but we all know that I2C/
SPI register accesses may sleep.
192 Storming the Common Clock Framework
Both of these restrictions have workarounds:
• Instead of using the gate-specific clock framework helper, you can want to use the low-level clk_hw_register() interface to control the parameters of the clock,
from its flags to its ops.
• You can implement the .enable/.disable logic in the .prepare/.
unprepare callbacks. Remember, .prepare/.unprepare ops may sleep.
This is guaranteed to work as it is a requirement for the consumer side to call clk_prepare() prior to calling clk_enable(), and then to follow a call to clk_disable() by a call to clk_unprepare(). By doing so, any consumer call to clk_enable() (mapped to the provider's .enable callback) will immediately return. However, since it is always preceded by a consumer call to clk_
prepare() (mapped to the .prepare callback), we can be sure that our clock will be ungated. The same goes for clk_disable (mapped to the .disable callback), which guarantees that our clock will be gated.
This clock driver implementation can be found in drivers/clk/clk-max9485.c,
while its device tree binding can found in Documentation/devicetree/
bindings/clock/maxim,max9485.txt, in the kernel source tree.
GPIO gate clock alternative
```dts
This is a basic clock that can be enabled and disabled through a gpio output. gpiogate-clock instances can only be instantiated from the device tree. For this, the compatible property should be gpio-gate-clock and #clock-cells should be
```
<0> as shown in the following excerpt from imx6qdl-sr-som-ti.dtsi:
```c
clk_ti_wifi: ti-wifi-clock {
dts
compatible = 'gpio-gate-clock';
#clock-cells = <0>;
clock-frequency = <32768>;
```
pinctrl-names = 'default';
pinctrl-0 = <&pinctrl_microsom_ti_clk>;
```dts
enable-gpios = <&gpio5 5 GPIO_ACTIVE_HIGH>;
c
};
pwrseq_ti_wifi: ti-wifi-pwrseq {
dts
compatible = 'mmc-pwrseq-simple';
```
pinctrl-names = 'default';
pinctrl-0 = <&pinctrl_microsom_ti_wifi_en>;
```dts
Writing a clock provider driver 193 reset-gpios = <&gpio5 26 GPIO_ACTIVE_LOW>;
```
post-power-on-delay-ms = <200>;
```dts
clocks = <&clk_ti_wifi>;
clock-names = 'ext_clock';
c
};
```
This clock-type driver is located in drivers/clk/clk-gpio.c, and further reading can be found in Documentation/devicetree/bindings/clock/gpio-gateclock.txt.
Clock multiplexer and its ops
A clock multiplexer has multiple input clock signals or parents, among which only one can be selected as output. Since this type of clock can choose from among multiple parents,
the .get_parent/.set_parent/.recalc_rate callbacks should be implemented.
A mux clock is represented in the CCF by an instance of struct clk_mux, which looks as follows:
```c
struct clk_mux {
struct clk_hw hw;
void __iomem *reg;
```
u32 *table;
u32 mask;
u8 shift;
u8 flags;
spinlock_t *lock;
```c
};
#define to_clk_mux(_hw) container_of(_hw, struct clk_mux, hw)
```
Let's take a look at the elements shown in the preceding structure:
• table is an array of register values corresponding to the parent index.
• mask and shift are used to modify the reg bit field prior to getting the appropriate value.
194 Storming the Common Clock Framework
• reg is the mmio register used for parent selection. By default, when the register's value is 0, it corresponds to the first parent, and so on. If there are exceptions,
various flags can be used, as well as another interface.
• flags represents the unique flags of the mux clock, which are as follows:
--CLK_MUX_INDEX_BIT: The register value is a power of 2. We will look at how this works shortly.
--CLK_MUX_HIWORD_MASK: This uses the concept of the hiword mask, which we explained earlier.
--CLK_MUX_INDEX_ONE: The register value does not start from 0, instead starting at 1. This means that the final value should be incremented by one.
--CLK_MUX_READ_ONLY: Some platforms have read-only clock muxes that are preconfigured at reset and cannot be changed at runtime.
--CLK_MUX_ROUND_CLOSEST : This flag uses the parent rate that is closest to the desired frequency.
• lock, if provided, is used to protect access to the register.
The CCF helper that's used to register such a clock is clk_hw_register_mux(). This looks as follows:
```c
struct clk_hw *
clk_hw_register_mux(struct device *dev, const char *name,
const char * const *parent_names,
```
u8 num_parents, unsigned long flags,
```c
void __iomem *reg, u8 shift, u8 width,
```
u8 clk_mux_flags, spinlock_t *lock)
Some of the parameters in the preceding registration interface were introduced when we described the mux clock structure. The remaining parameters are as follows:
• parent_names: This is an array of strings that describes all possible parent clocks.
• num_parents: This specifies the number of parent clocks.
Writing a clock provider driver 195
While registering such a clock, depending on the CLK_MUX_READ_ONLY flag being set or not, the CCF assigns different clock ops. If set, clk_mux_ro_ops is used. This clock ops only implements the .get_parent ops as there would be no way to change the parent.
If this is not set, clk_mux_ops is used. This ops implements .get_parent, .set_
parent, and .determine_rate, as follows:
```c
if (clk_mux_flags & CLK_MUX_READ_ONLY)
```
init.ops = &clk_mux_ro_ops;
else init.ops = &clk_mux_ops;
These clock ops are defined as follows:
```c
const struct clk_ops clk_mux_ops = {
```
.get_parent = clk_mux_get_parent,
.set_parent = clk_mux_set_parent,
.determine_rate = clk_mux_determine_rate,
```c
};
EXPORT_SYMBOL_GPL(clk_mux_ops);
const struct clk_ops clk_mux_ro_ops = {
```
.get_parent = clk_mux_get_parent,
```c
};
EXPORT_SYMBOL_GPL(clk_mux_ro_ops);
```
In the preceding code, there is a .table field. This is used to provide a set of values according to the parent index. However, the preceding registration interface, clk_hw_
register_mux(), does not provide us with any way to feed this table.
Due to this, there is another variant available in the CCF that allows us to pass the table:
```c
struct clk *
clk_register_mux_table(struct device *dev,
const char *name,
const char **parent_names,
```
u8 num_parents,
```c
unsigned long flags,
void __iomem *reg, u8 shift, u32 mask,
```
u8 clk_mux_flags, u32 *table, spinlock_t *lock);
196 Storming the Common Clock Framework
The interface registers a mux to control an irregular clock through a table. Whatever the registration interface is, the same internal ops are used. Now, let's pay special attention to the most important ones; that is, .set_parent and .get_parent:
• clk_mux_set_parent: When this is called, if table is not NULL, it gets a register value from the index in table. If table is NULL and the CLK_MUX_
INDEX_BIT flag is set, this means the register value is a power of 2 according to index. This value is then obtained with val = 1 << index; if CLK_MUX_
INDEX_ONE is set, this value is incremented by one. If table is NULL and CLK_
MUX_INDEX_BIT is not set, index is used as the default value. In either case, the final value is left-shifted at shift time and OR'ed with a mask prior to us obtaining the real value. This should be written into reg for parent selection:
```c
unsigned int clk_mux_index_to_val(u32 *table, unsigned int flags,
```
u8 index)
```c
{
unsigned int val = index;
if (table) {
```
val = table[index];
```c
} else {
if (flags & CLK_MUX_INDEX_BIT)
```
val = 1 << index;
```c
if (flags & CLK_MUX_INDEX_ONE) val++;
}
return val;
}
static int clk_mux_set_parent(struct clk_hw *hw,
```
u8 index)
```c
{
struct clk_mux *mux = to_clk_mux(hw);
```
u32 val =
```c
clk_mux_index_to_val(mux->table, mux->flags,
```
index);
```c
unsigned long flags = 0; u32 reg;
if (mux->lock)
spin_lock_irqsave(mux->lock, flags);
```
Writing a clock provider driver 197 else
```c
__acquire(mux->lock);
if (mux->flags & CLK_MUX_HIWORD_MASK) {
dts
reg = mux->mask << (mux->shift + 16);
c
} else {
dts
reg = clk_readl(mux->reg);
c
reg &= ~(mux->mask << mux->shift);
}
val = val << mux->shift; reg |= val;
clk_writel(reg, mux->reg);
if (mux->lock)
spin_unlock_irqrestore(mux->lock, flags);
```
else
```c
__release(mux->lock);
return 0;
}
```
• clk_mux_get_parent: This reads the value in reg, shifts it shift time to the right and applies (the AND operation) mask to it prior to getting the real value.
This value is then given to the clk_mux_val_to_index() helper, which will return the right index according to the reg value. clk_mux_val_to_index()
first gets the number of parents for the given clock. If table is not NULL, this number is used as the upper limit in a loop to walk through table. Each iteration will check whether the table value at the current position matches val. If it does,
the current position in the iteration is returned. If no match is found, an error is returned. ffs() returns the position of the first (least significant) bit set in the word:
```c
int clk_mux_val_to_index(struct clk_hw *hw, u32 *table,
unsigned int flags,
unsigned int val)
{
int num_parents = clk_hw_get_num_parents(hw);
if (table) {
int i;
```
198 Storming the Common Clock Framework for (i = 0; i < num_parents; i++)
```c
if (table[i] == val)
return i;
return -EINVAL;
}
if (val && (flags & CLK_MUX_INDEX_BIT))
```
val = ffs(val) - 1;
```c
if (val && (flags & CLK_MUX_INDEX_ONE))
```
val--;
```c
if (val >= num_parents)
return -EINVAL;
return val;
}
EXPORT_SYMBOL_GPL(clk_mux_val_to_index);
static u8 clk_mux_get_parent(struct clk_hw *hw)
{
struct clk_mux *mux = to_clk_mux(hw);
```
u32 val;
```c
val = clk_readl(mux->reg) >> mux->shift;
val &= mux->mask;
return clk_mux_val_to_index(hw, mux->table,
mux->flags, val);
}
```
An example of such a driver can be found in drivers/clk/microchip/
clk-pic32mzda.c.
I2C/SPI-based clock mux
The aforementioned CCF interfaces that are used to handle clock muxes assume that control is provided via mmio registers. However, there are some I2C/SPI-based clock mux chips where you have to rely on the low-level clk_hw (using a clk_hw_register()
registration-based interface) interface and register each clock according to its properties before providing the appropriate ops.
Writing a clock provider driver 199
Each mux input clock should be a parent of the mux output, which must have at least
.set_parent and .get_parent ops. Other ops are also allowed but not mandatory.
A concrete example is the Linux driver for the Si5351a/b/c programmable I2C
clock generator from Silicon Labs, available in drivers/clk/clk-si5351.c in the kernel sources. Its device tree binding is available in Documentation/devicetree/
bindings/clock/silabs,si5351.txt.
Important note
To write such clock drivers, you must learn how clk_hw_register_
mux is implemented and base your registration function on it, without the mmio/spinlock part, and then provide your own ops according to the clock's properties.
GPIO mux clock alternative
The GPIO mux clock can be represented as follows:
Figure 4.2 – GPIO mux clock
This is a limited alternative to clock multiplexing that only accepts two parents, as stated in the following excerpt from its drivers, which are available in drivers/clk/
clk-gpio.c. In this case, the parent selection depends on the value of the gpio being used:
```c
struct clk_hw *clk_hw_register_gpio_mux(struct device *dev,
const char *name,
const char *
const *parent_names,
```
u8 num_parents,
```c
struct gpio_desc *gpiod,
unsigned long flags)
{
200 Storming the Common Clock Framework if (num_parents != 2) {
pr_err('mux-clock %s must have 2 parents\n', name);
return ERR_PTR(-EINVAL);
}
return clk_register_gpio(dev, name, parent_names,
```
num_parents,
gpiod, flags, &clk_gpio_mux_ops);
```c
}
EXPORT_SYMBOL_GPL(clk_hw_register_gpio_mux);
```
According to its binding, it is only instantiable in the device tree. This binding can be found in Documentation/devicetree/bindings/clock/gpio-mux-clock.
txt, in the kernel sources. The following example show how to use it:
```c
clocks {
```
/* fixed clock oscillators */
```c
parent1: oscillator22 {
dts
compatible = 'fixed-clock';
#clock-cells = <0>;
clock-frequency = <22579200>;
c
};
parent2: oscillator24 {
dts
compatible = 'fixed-clock';
#clock-cells = <0>;
clock-frequency = <24576000>;
c
};
```
/* gpio-controlled clock multiplexer */
```c
mux: multiplexer {
dts
compatible = 'gpio-mux-clock';
clocks = <&parent1>, <&parent2>;
```
/* parent clocks */
```dts
#clock-cells = <0>;
select-gpios = <&gpio 42 GPIO_ACTIVE_HIGH>;
c
};
};
```
202 Storming the Common Clock Framework
• flags: This is the divider-clock-specific flag of the clock. Various flags can be used here, some of which are as follows:
--CLK_DIVIDER_ONE_BASED: When set, this means that the divider is the raw value that's read from the register since the default divisor is the value that's read from the register plus one. This also implies 0 is invalid, unless the CLK_DIVIDER_
ALLOW_ZERO flag is set.
--CLK_DIVIDER_ROUND_CLOSEST: This should be used when we want to be able to round the divider to the closest and best calculated one instead of just rounding up, which is the default behavior.
--CLK_DIVIDER_POWER_OF_TWO: The actual divider value is the register value raised to a power of 2.
--CLK_DIVIDER_ALLOW_ZERO: The divider value can be 0 (no change, depending on hardware support).
--CLK_DIVIDER_HIWORD_MASK: See the Gateable clock and its ops section for more details on this flag.
--CLK_DIVIDER_READ_ONLY: This flag shows that the clock has preconfigured settings and instructs the framework not to change anything. This flag also affects the ops that have been assigned to the clock.
CLK_DIVIDER_MAX_AT_ZERO: This allows a clock divider to have a max divisor when it's set to zero. So, if the field value is zero, the divisor value should be 2 bits in width. For example, let's consider a divisor clock with a 2-bit field:
Value divisor
0 4
1 1
2 2
3 3
• table: This is an array of value/divider pairs whose last entry should have div =
0. This will be described shortly.
• lock: Like in other clock data structures, if provided, it is used to protect access to the register.
Writing a clock provider driver 203
• clk_hw_register_divider(): This is the most commonly used registration interface for such clocks. It is defined as follows:
```c
struct clk_hw *
clk_hw_register_divider(struct device *dev,
const char *name,
const char *parent_name,
unsigned long flags,
void __iomem *reg,
```
u8 shift, u8 width,
u8 clk_divider_flags,
spinlock_t *lock)
This function registers a divider clock with the system and returns a pointer to the underlying clk_hw field. Here, you can can use the to_clk_divider macro to grab a pointer to the wrapper's clk_divider structure. Except for name and parent_name,
which represent the name of the clock and the name of its parent, respectively, the other arguments in this function match the fields described in the struct clk_divider structure.
You may have noticed that the .table field is not being used here. This field is kind of special as it is used for clock dividers whose division ratios are uncommon. Actually, there are clock dividers where each individual clock line has a number of division ratios that are not related to each other's clock lines. Sometimes, there is not even any linearity between each ratio and the register value. For such cases, the best solution is to feed each clock line a table, where each ratio corresponds to its register value. This requires us to introduce a new registration interface that accepts such a table; that is, clk_hw_register_
divider_table. This can be defined as follows:
```c
struct clk_hw *
clk_hw_register_divider_table(
struct device *dev,
const char *name,
const char *parent_name,
unsigned long flags,
void __iomem *reg,
```
u8 shift, u8 width,
u8 clk_divider_flags,
```c
const struct clk_div_table *table,
```
spinlock_t *lock)
204 Storming the Common Clock Framework
This interface is used to register the clock with an irregular frequency division ratio,
compared to the preceding interface. The difference is that the relationship between the value of the divider and the value of the register is determined by a table of the struct clk_div_table type. This table structure can be defined as follows:
```c
struct clk_div_table {
unsigned int val;
unsigned int div;
};
```
In the preceding code, val represents the register value, while div represents the division ratio. Their relationship can also be changed byusing clk_divider_flags. Regardless of what registration interface is used, the CLK_DIVIDER_READ_ONLY flag determines the ops to be assigned to the clock, as follows:
```c
if (clk_divider_flags & CLK_DIVIDER_READ_ONLY)
```
init.ops = &clk_divider_ro_ops;
else init.ops = &clk_divider_ops;
Both these clock ops are defined in drivers/clk/clk-divider.c, as follows:
```c
const struct clk_ops clk_divider_ops = {
```
.recalc_rate = clk_divider_recalc_rate,
.round_rate = clk_divider_round_rate,
.set_rate = clk_divider_set_rate,
```c
};
EXPORT_SYMBOL_GPL(clk_divider_ops);
const struct clk_ops clk_divider_ro_ops = {
```
.recalc_rate = clk_divider_recalc_rate,
.round_rate = clk_divider_round_rate,
```c
};
EXPORT_SYMBOL_GPL(clk_divider_ro_ops);
```
While the former can set the clock rate, the last one cannot.
Writing a clock provider driver 205
Important note
Once again, so far, using the clock-type-dependent registration interface provided by the kernel requires your clock to be mmio. Implementing such a clock driver for a non-mmio-based (SPI or I2C-based) clock would require using the low-level hw_clk registration interface and implementing the appropriate ops. An example of such a driver for an I2C-based clock, along with the appropriate ops implemented, can be found in drivers/clk/
clk-max9485.c. Its binding can be found in Documentation/
devicetree/bindings/clock/maxim,max9485.txt. This is a much more adjustable clock driver than the divider one.
The adjustable clock has no secrets for us anymore. Its APIs and ops have been described,
as well as how it deals with irregular ratios. Next, we'll look at our final clock type, which is a mix of all the clock types we have seen so far: the composite clock.
Composite clock and its ops
This clock is used for clock branches that use a combination of mux, divider, and gate components. This is the case on most Rockchip SoCs. The clock framework abstracts such clocks by means of struct clk_composite, which looks as follows:
```c
struct clk_composite {
struct clk_hw hw;
struct clk_ops ops;
struct clk_hw *mux_hw;
struct clk_hw *rate_hw;
struct clk_hw *gate_hw;
const struct clk_ops *mux_ops;
const struct clk_ops *rate_ops;
const struct clk_ops *gate_ops;
};
#define to_clk_composite(_hw) container_of(_hw,
struct clk_composite,
```
hw)
206 Storming the Common Clock Framework
The fields in this data structure are quite self-explanatory, as follows:
• hw, as in other clock structures, is the handle between common and hardwarespecific interfaces.
• mux_hw represents the mux clock.
• rate_hw represents the divider clock.
• gate_hw represents the gate clock.
• mux_ops, rate_ops, and gate_ops are the clock ops for mux, rate, and gate,
respectively.
Such a clock can be registered through the following interface:
```c
struct clk_hw *clk_hw_register_composite(
struct device *dev, const char *name,
const char * const *parent_names, int num_parents,
struct clk_hw *mux_hw,
const struct clk_ops *mux_ops,
struct clk_hw *rate_hw,
const struct clk_ops *rate_ops,
struct clk_hw *gate_hw,
const struct clk_ops *gate_ops,
unsigned long flags)
```
This may look a bit complicated, but if you went through the previous clock, this one will be more or less obvious to you. Take a look at drivers/clk/sunxi/clk-a10-
hosc.c in the kernel source for an example of a composite clock driver.
Putting it all together – global overview
If you are still confused, then take a look at the following diagram:
Writing a clock provider driver 207
Figure 4.3 – Clock tree example
The preceding clock tree shows an oscillator clock feeding three PLLs – that is, pll1,
pll2, and pll3 – as well as a multiplexer. According to the multiplexer (mux), hw3_clk can be derived from either the pll2, pll3, or osc clock.
The following device tree excerpt can be used to model the preceding clock tree:
```c
osc: oscillator {
dts
#clock-cells = <0>;
compatible = 'fixed-clock';
clock-frequency = <20000000>;
clock-output-names = 'osc20M';
c
};
pll2: pll2 {
dts
#clock-cells = <0>;
compatible = 'abc123,pll2-clock';
clock-frequency = <23000000>; clocks = <&osc>;
```
[...]
```c
};
pll3: pll3 {
dts
#clock-cells = <0>;
compatible = 'abc123,pll3-clock';
208 Storming the Common Clock Framework clock-frequency = <23000000>; clocks = <&osc>;
```
[...]
```c
};
hw3_clk: hw3_clk {
dts
#clock-cells = <0>;
compatible = 'abc123,hw3-clk';
clocks = <&pll2>, <&pll3>, <&osc>;
clock-output-names = 'hw3_clk';
c
};
```
When it comes to the source code, the following excerpt shows how to register hw_clk3 as a mux (a clock multiplexer) and points out the parent relationship of pll2, pll3, and osc:
```dts
of_property_read_string(node, 'clock-output-names', &clk_name);
```
parent_names[0] = of_clk_get_parent_name(node, 0);
parent_names[1] = of_clk_get_parent_name(node, 1);
parent_names[2] = of_clk_get_parent_name(node, 2); /* osc */
clk = clk_register_mux(NULL, clk_name, parent_names,
```c
ARRAY_SIZE(parent_names), 0, regs_base,
```
offset_bit, one_bit, 0, NULL);
A downstream clock provider should use of_clk_get_parent_name() to obtain its parent clock name. For a block with multiple outputs, of_clk_get_parent_name()
```dts
can return a valid clock name, but only when the clock-output-names property is present.
```
Now, we can look at the clock tree summary via the CCF sysfs interface, /sys/kernel/
debug/clk/clk_summary. This can be seen in the following excerpt:
```bash
$ mount -t debugfs none /sys/kernel/debug
# cat /sys/kernel/debug/clk/clk_summary
```
[...]
Writing a clock provider driver 209
With that, we are done with the clock producer side. We have learned about its APIs and discussed its declaration in the device tree. Moreover, we have learned how to dump their topology from sysfs. Now, let's look at clock consumer APIs.
## Introducing clock consumer APIs
Clock producer device drivers are useless without consumers at the other end to leverage the clock lines that have been exposed. The main purpose of such drivers is to assign their clock source lines to consumers. These clock lines are then used for several purposes,
and the Linux kernel provides consequent APIs and helpers to achieve the required goal.
Consumer drivers need to include <linux/clk.h> in their code for its APIs to be used. Moreover, nowadays, the clock consumer interface entirely relies on the device tree,
```dts
meaning that consumers should be assigned clocks they need from the device tree. The consumer binding should follow the provider's since the consumer specifier is determined by the provider's #clock-cells property. Take a look at the following UART node description, which requires two clock lines:
c
uart1: serial@02020000 {
dts
compatible = 'fsl,imx6sx-uart', 'fsl,imx21-uart';
reg = <0x02020000 0x4000>;
interrupts = <GIC_SPI 26 IRQ_TYPE_LEVEL_HIGH>;
clocks = <&clks IMX6SX_CLK_UART_IPG>,
```
<&clks IMX6SX_CLK_UART_SERIAL>;
```dts
clock-names = 'ipg', 'per';
```
dmas = <&sdma 25 4 0>, <&sdma 26 4 0>;
dma-names = 'rx', 'tx';
```dts
status = 'disabled';
c
};
```
210 Storming the Common Clock Framework
This represents a device with two clock inputs. The preceding node excerpt allows us to introduce the device tree binding for the clock consumer, which should have, at the very least, the following properties:
```dts
• The clocks property is where you should specify the source clock lines for a device with respect to the #clock-cells property of the provider.
• clock-names is the property used to name clocks in the same way they are listed in clocks. In other words, this property should be used to list the input name(s)
```
for the clock(s) with respect to the consuming node. This name(s) should reflect the consumer input signal name(s) and can/must be used in the code (see [devm_]
```c
clk_get()) so that it matches the corresponding clock.
```
Important note
Clock consumer nodes must never directly reference the provider's clockoutput-names property.
The consumer has a reduced and portable API based on whatever the underlying hardware clock is. Next, we'll take a look at the common operations that are performed by consumer drivers, along with their associated APIs.
Grabbing and releasing clocks
The following functions allow us to grab and release a clock, given its id:
```c
struct clk *clk_get(struct device *dev, const char *id);
void clk_put(struct clk *clk);
struct clk *C(struct device *dev, const char *id)
```
dev is the device using this clock, while id is the name given to the clock in the device tree. On success, clk_get returns a pointer to a struct clk. This can be given to any other clk-consumer API. clk_put actually releases the clock line. The first two APIs in the preceding code are defined in drivers/clk/clkdev.c. However, other clock consumer APIs are defined in drivers/clk/clk.c. devm_clk_get is simply the managed version of clk_get.
Preparing/unpreparing clocks
To prepare a clock for use, you can use clk_prepare(), as follows:
```c
void clk_prepare(struct clk *clk);
void clk_unprepare(struct clk *clk);
```
Writing a clock provider driver 211
These functions may sleep, which means they cannot be called from within an atomic context. It is worth always calling clk_prepare() before clock_enable(). This may be useful if the underlying clock is behind a slow bus (SPI/I2C) since such clock drivers must implement their enable/disable (which must not sleep) code from within the prepare/unprepare ops (which are allowed to sleep).
Enabling/disabling
When it comes to gating/ungating the clock, you can use the following API:
```c
int clk_enable(struct clk *clk);
void clk_disable(struct clk *clk);
clk_enable must not sleep and actually ungates the clock. It returns 0 on success or an error otherwise. clk_disable does the reverse. To enforce the fact of calling prepare prior to calling enable, the clock framework provide the clk_prepare_enable API,
```
which internally calls both. The opposite can be done with clk_disable_unprepare:
```c
int clk_prepare_enable(struct clk *clk)
void clk_disable_unprepare(struct clk *clk)
```
Rate functions
For clocks whose rates can be changed, we can use the following function to get/set the rate of the clock:
```c
unsigned long clk_get_rate(struct clk *clk);
int clk_set_rate(struct clk *clk, unsigned long rate);
long clk_round_rate(struct clk *clk, unsigned long rate);
clk_get_rate()returns 0 if clk is NULL; otherwise, it will return the rate of the clock;
```
that is, the cached rate. However, if the CLK_GET_RATE_NOCACHE flag is set, a new calculation will be done (by means of recalc_rate()) to return the real clock rate.
On the other hand, clk_set_rate() will set the rate of the clock. However, its rate parameter can't take any value. To see if the rate you are targeting is supported or allowed by the clock, you should use clk_round_rate(), along with the clock pointer and the target rate in Hz, as follows.
rounded_rate = clk_round_rate(clkp, target_rate);
212 Storming the Common Clock Framework
This is the return value of clk_round_rate() that must be given to clk_set_
rate(), as follows:
ret = clk_set_rate(clkp, rounded_rate);
Changing the clock rate may fail in the following cases:
• The clock is drawing its source from a fixed-rate clock source (for example, OSC0,
OSC1, XREF, and so on).
• The clock is in use by multiple modules/children, which would mean that usecount is greater than 1.
• The clock source is in use by more than one child.
Note that parent rates are returned if .round_rate() is not implemented.
Parent functions
There are clocks that are the children of other clocks, thus creating a parent/child relationship. To either get/set the parent of a given clock, you can use the following functions:
```c
int clk_set_parent(struct clk *clk, struct clk *parent);
struct clk *clk_get_parent(struct clk *clk);
clk_set_parent() actually sets the parent of the given clock, while clk_get_
```
parent() returns the current parent.
Putting it all together
To summarize this, take a look at the following excerpt of the i.MX serial driver
(drivers/tty/serial/imx.c), which deals with the preceding device node:
```c
sport->clk_per = devm_clk_get(&pdev->dev, 'per');
if (IS_ERR(sport->clk_per)) {
ret = PTR_ERR(sport->clk_per);
dev_err(&pdev->dev, 'failed to get per clk: %d\n', ret);
return ret;
}
sport->port.uartclk = clk_get_rate(sport->clk_per);
```
/*
* For register access, we only need to enable the ipg clock.
Writing a clock provider driver 213
*/
```c
ret = clk_prepare_enable(sport->clk_ipg);
if (ret)
return ret;
```
In the preceding code excerpt, we see can how the driver grabs the clock and its current rate, and then enables it.
## Summary
In this chapter, we walked through the Linux Common Clock Framework. We introduced both the provider and consumer sides, as well as the user space interface. We then discussed the different clock types and learned how to write the appropriate Linux drivers for each.
The next chapter deals with ALSA SoC, the Linux kernel framework for audio. This framework heavily relies on the clock framework to, for example, sample audio