```bash
# Chapter 13 - The Linux Device Model
```
Until version 2.5, the kernel had no way to describe and manage objects, and code reusability was not as enhanced as it is now. In other words, there was no device topology nor organization. There was no information on subsystem relationships, nor on how the system is put together. Then came the Linux Device Model (LDM), which introduced the following:
The concept of class, to group devices of the same type or devices that expose the same functionalities (for example, mice and keyboards are both input devices).
Communication with the user space through a virtual filesystem called sysfs, in order to let the user space manage and enumerate devices and the properties they expose.
Management of the object life cycle, using reference counting (heavily used in managed resources).
Power management in order to handle the order in which devices should shut down.
Code reusability. Classes and frameworks expose interfaces, behaving like a contract that any driver that registers with them must respect.
LDM brought an Object Oriented (OO)-like programming style to the kernel.
In this chapter, we will take advantage of LDM and export some properties to the user space through the sysfs filesystem.
In this chapter, we will cover the following topics:
```c
Introducing LDM data structures (driver, device, bus)
```
Gathering kernel objects by type
Dealing with the kernel sysfs interface
## LDM data structures
The goal is to build a complete DT that will map each physical device present on the system, and introduce their hierarchy. One common and generic structure has been created to represent any object that could be a part of the device model. The upper level of LDM
relies on the bus represented in the kernel as an instance of struct bus_type; the device driver, represented by a struct device_driver structure, and the device, which is the last element represented as an instance of the struct device structure. In this section, we will design a bus driver packt bus, in order to explore into LDM data structures and mechanisms in detail.
## The bus
A bus is a channel link between devices and processors. The hardware entity that manages the bus and exports its protocol to devices is called the bus controller. For example, the USB
controller provides USB support. The I2C controller provides I2C bus support. Therefore,
the bus controller, being a device on its own, must be registered like any device. It will be the parent of devices that need to sit on the bus. In other words, every device sitting on the bus must have its parent field pointing to the bus device. A bus is represented in the kernel by the struct bus_type structure:
```c
struct bus_type {
```
const char *name;
const char *dev_name;
```c
struct device *dev_root;
struct device_attribute *dev_attrs; /* use dev_groups instead */
```
const struct attribute_group **bus_groups;
const struct attribute_group **dev_groups;
const struct attribute_group **drv_groups;
```c
int (*match)(struct device *dev, struct device_driver *drv);
int (*probe)(struct device *dev);
int (*remove)(struct device *dev);
void (*shutdown)(struct device *dev);
int (*suspend)(struct device *dev, pm_message_t state);
int (*resume)(struct device *dev);
```
const struct dev_pm_ops *pm;
```c
struct subsys_private *p;
struct lock_class_key lock_key;
};
```
The following are the meanings of elements in the structure:
match: This is a callback, called whenever a new device or driver is added to the bus. The callback must be smart enough and should return a nonzero value when there is a match between a device and a driver, both given as parameters. The main purpose of a match callback is to allow a bus to determine if a particular device can be handled by a given driver or the other logic, if the given driver supports a given device. Most of the time, the verification is done by a simple string comparison (device and driver name, of table- and DT-compatible property). For enumerated devices (PCI, USB), the verification is done by comparing the device IDs supported by the driver with the device ID of the given device, without sacrificing bus-specific functionality.
probe: This is a callback when a new device or driver is added to the bus, after the match has occurred. This function is responsible for allocating the specific bus device structure, and calls the given driver's probe function, which is supposed to manage the device (allocated earlier).
remove: This is called when a device is removed from the bus.
suspend: This is a method called when a device on the bus needs to be put into sleep mode.
resume: This is called when a device on the bus has to be brought out of sleep mode.
pm: This is a set of power management operations of the bus, which will call the specific device driver's pm-ops.
drv_groups: This is a pointer to a list (array) of struct attribute_group elements, each of which has a pointer to a list (array) of struct attribute elements. It represents the default attributes of the device drivers on the bus.
Attributes passed to this field will be given to every driver registered with the bus. Those attributes can be found in the driver's directory in /sys/bus/<busname>/drivers/<driver-name>.
dev_groups: This represents the default attributes of the devices on the bus.
Attributes passed (through the list/array of the struct attribute_group elements) to this field will be given to every device registered with the bus. Those attributes can be found in the device directory in /sys/bus/<busname>/devices/<device-name>.
```c
bus_group: This holds the set (group) of default attributes added automatically when the bus is registered with the core.
```
Apart from defining a bus_type, the bus controller driver must define a bus-specific driver structure that extends the generic struct device_driver, and a bus-specific device structure that extends the generic struct device structure, both part of the device model core. The bus drivers must also allocate a bus-specific device structure for each physical device discovered when probing, and is responsible for initializing the bus and parent fields of the device and registering the device with the LDM core. Those fields must point to the bus device and the bus_type structures defined in the bus driver. The LDM core uses that to build the device hierarchy and initialize the other fields.
In our example, the following are two helper macros to get the packt device and the packt driver, given a generic struct device and struct driver:
```c
#define to_packt_driver(d) container_of(d, struct packt_driver, driver)
#define to_packt_device(d) container_of(d, struct packt_device, dev)
```
And, then comes the structure used to identify a packt device:
```c
struct packt_device_id {
```
char name[PACKT_NAME_SIZE];
kernel_ulong_t driver_data; /* Data private to the driver */
```c
};
```
The following are packt-specific devices and driver structures:
/*
* Bus specific device structure
* This is what a packt device structure looks like
*/
```c
struct packt_device {
struct module *owner;
unsigned char name[30];
unsigned long price;
struct device dev;
};
```
/*
* Bus specific driver structure
* This is what a packt driver structure looks like
* You should provide your device's probe and remove function.
* may be release too
*/
```c
struct packt_driver {
int (*probe)(struct packt_device *packt);
int (*remove)(struct packt_device *packt);
void (*shutdown)(struct packt_device *packt);
};
```
Each bus internally manages two important lists; a list of devices added and sitting on it,
and the list of drivers registered with it. Whenever you add/register or remove/unregister a device/driver to/from the bus, the corresponding list is updated with the new entry. The bus driver must provide helper functions to register/unregister device drivers that can handle devices on that bus, as well as helper functions to register/unregister devices sitting on the bus. These helper functions always wrap the generic functions provided by the LDM
core, which are driver_register(), device_register(), driver_unregister, and device_unregister().
/*
* Now let us write and export symbols that people writing
* drivers for packt devices must use.
*/
```c
int packt_register_driver(struct packt_driver *driver)
{
driver->driver.bus = &packt_bus_type;
return driver_register(&driver->driver);
}
EXPORT_SYMBOL(packt_register_driver);
void packt_unregister_driver(struct packt_driver *driver)
{
driver_unregister(&driver->driver);
}
EXPORT_SYMBOL(packt_unregister_driver);
int packt_device_register(struct packt_device *packt)
{
return device_register(&packt->dev);
}
EXPORT_SYMBOL(packt_device_register);
void packt_unregister_device(struct packt_device *packt)
{
device_unregister(&packt->dev);
}
EXPORT_SYMBOL(packt_device_unregister);
```
The function used to allocate packt devices is as follows. One must use this to create an instance of any physical device sitting on the bus:
/*
* This function allocate a bus specific device structure
* One must call packt_device_register to register
* the device with the bus
*/
```c
struct packt_device * packt_device_alloc(const char *name, int id)
{
struct packt_device *packt_dev;
int status;
```
packt_dev = kzalloc(sizeof *packt_dev, GFP_KERNEL);
```c
if (!packt_dev)
return NULL;
```
/* new devices on the bus are son of the bus device */
```c
strcpy(packt_dev->name, name);
packt_dev->dev.id = id;
dev_dbg(&packt_dev->dev,
"device [%s] registered with packt bus\n", packt_dev->name);
return packt_dev;
```
out_err:
```c
dev_err(&adap->dev, "Failed to register packt client %s\n",
packt_dev->name);
kfree(packt_dev);
return NULL;
}
EXPORT_SYMBOL_GPL(packt_device_alloc);
int packt_device_register(struct packt_device *packt)
{
packt->dev.parent = &packt_bus;
packt->dev.bus = &packt_bus_type;
return device_register(&packt->dev);
}
EXPORT_SYMBOL(packt_device_register);
```
## Bus registration
The bus controller is a device itself, and in 99% of cases buses are platform devices (even buses that offer enumeration). For example, the PCI controller is a platform device, and so is its respective driver. One must use the bus_register(struct *bus_type) function in order to register a bus with the kernel. The packt bus structure looks like this:
/*
* This is our bus structure
*/
```c
struct bus_type packt_bus_type = {
```
.name = "packt",
.match = packt_device_match,
.probe = packt_device_probe,
.remove = packt_device_remove,
.shutdown = packt_device_shutdown,
```c
};
```
The bus controller is a device itself; it has to be registered with the kernel, and will be used as a parent of the device siting on the bus. This is done in the bus controller's probe or init function. In the case of the packt bus, the code would be as follows:
/*
* Bus device, the master.
*
*/
```c
struct device packt_bus = {
```
.release = packt_bus_release,
.parent = NULL, /* Root device, no parent needed */
```c
};
static int __init packt_init(void)
{
int status;
```
status = bus_register(&packt_bus_type);
```c
if (status < 0)
```
goto err0;
status = class_register(&packt_master_class);
```c
if (status < 0)
```
goto err1;
/*
* After this call, the new bus device will appear
* under /sys/devices in sysfs. Any devices added to this
* bus will shows up under /sys/devices/packt-0/.
*/
```c
device_register(&packt_bus);
return 0;
```
err1:
```c
bus_unregister(&packt_bus_type);
```
err0:
```c
return status;
}
```
When a device is registered by the bus controller driver, the parent member of the device must point to the bus controller device, and its bus property must point to the bus type to build the physical DT. To register a packt device, one must call packt_device_register,
given as an argument allocated with packt_device_alloc:
```c
int packt_device_register(struct packt_device *packt)
{
packt->dev.parent = &packt_bus;
packt->dev.bus = &packt_bus_type;
return device_register(&packt->dev);
}
EXPORT_SYMBOL(packt_device_register);
```
## The device driver
A global device hierarchy allows each device in the system to be represented in a common way. This allows the core to easily walk the DT to create such things as properly ordered power management transitions:
```c
struct device_driver {
```
const char *name;
```c
struct bus_type *bus;
struct module *owner;
```
const struct of_device_id *of_match_table;
const struct acpi_device_id *acpi_match_table;
```c
int (*probe) (struct device *dev);
int (*remove) (struct device *dev);
void (*shutdown) (struct device *dev);
int (*suspend) (struct device *dev, pm_message_t state);
int (*resume) (struct device *dev);
```
const struct attribute_group **groups;
const struct dev_pm_ops *pm;
```c
};
struct device_driver defines a simple set of operations for the core to perform these actions on each device:
```
*name represents the driver's name. It can be used for matching, by comparing with the device name.
*bus represents the bus the driver sits on. The bus driver must fill in this field.
module represents the module owning the driver. In 99% of cases, one should set this field to THIS_MODULE.
of_match_table is a pointer to the array of struct of_device_id. The struct of_device_id structure is used to perform OF matching through a special file called DT, passed to the kernel during the boot process:
```c
struct of_device_id {
```
char compatible[128];
const void *data;
```c
};
```
suspend and resume callbacks provide power management functionality. The remove callback is called when the device is physically removed from the system, or when its reference count reaches 0. The remove callback is also called during system reboot.
probe is the probe callback that runs when attempting to bind a driver to a device. The bus driver is in charge of calling the device driver's probe function.
group is a pointer to a list (array) of struct attribute_group, used as a default attribute for the driver. Use this method instead of creating an attribute separately.
## Device driver registration driver_register() is the low-level function used to register a device driver with the bus.
It adds the driver to the bus's list of drivers. When a device driver is registered with the bus, the core walks through the bus's list of devices and calls the bus's match callback for each device that does not have a driver associated with it in order to find out if there are any devices that the driver can handle.
When a match occurs, the device and the device driver are bound together. The process of associating a device with a device driver is called binding.
Back to the registration of drivers with our packt bus; one has to use packt_register_driver(struct packt_driver *driver), which is a wrapper around driver_register(). The *driver parameter must have been filled prior to registering the packt driver. The LDM core provides helper functions for iterating over the list of drivers registered with the bus:
```c
int bus_for_each_drv(struct bus_type * bus,
struct device_driver * start,
void * data, int (*fn)(struct device_driver *,
void *));
```
This helper iterates over the bus's list of drivers, and calls the fn callback for each driver in the list.
## The device
The struct device is the generic data structure used to describe and characterize each device on the system, whether it is physical or not. It contains details about the physical attributes of the device, and provides proper linkage information to build suitable device trees and reference counting:
```c
struct device {
struct device *parent;
struct kobject kobj;
```
const struct device_type *type;
```c
struct bus_type *bus;
struct device_driver *driver;
void *platform_data;
void *driver_data;
struct device_node *of_node;
struct class *class;
```
const struct attribute_group **groups;
```c
void (*release)(struct device *dev);
};
```
* parent represents the device's parent, used to build a device tree hierarchy.
When registered with a bus, the bus driver is responsible for setting this field with the bus device.
*bus represents the bus the device sits on. The bus driver must fill this field.
*type identifies the device's type.
kobj is the kobject in handle reference counting and device model support.
* of_node is a pointer to the OF (DT) node associated with the device. It is up to the bus driver to set this field.
platform_data is a pointer to the platform data specific to the device. Usually declared in a board-specific file during device provisioning.
```c
driver_data is a pointer to private data for the driver.
```
class is a pointer to the class that the device belongs to.
* group is a pointer to a list (array) of struct attribute_group, used as the default attribute for the device. Use this method instead of creating the attribute separately.
release is a callback called when the device reference count reaches zero. The bus is responsible for setting this field. The packt bus driver shows you how to do this.
## Device registration device_register is the function provided by the LDM core to register a device with the bus. After this call, the bus list of drivers is iterated over to find the driver that supports this device, and then this device is added to the bus's list of devices. device_register()
internally calls device_add():
```c
int device_add(struct device *dev)
{
```
[...]
```c
bus_probe_device(dev);
if (parent)
klist_add_tail(&dev->p->knode_parent,
&parent->p->klist_children);
```
[...]
```c
}
```
The helper function provided by the kernel to iterate over the bus's list of devices is bus_for_each_dev:
```c
int bus_for_each_dev(struct bus_type * bus,
struct device * start, void * data,
int (*fn)(struct device *, void *));
```
Whenever a device is added, the core invokes the match method of the bus driver
```c
(bus_type->match). If the match function says there is a driver for this device, the core will invoke the probe function of the bus driver (bus_type->probe), given both device and driver as parameters. It is then up to the bus driver to invoke the probe method of the device's driver (driver->probe). For our packt bus driver, the function used to register a device is packt_device_register(struct packt_device *packt), which internally calls device_register, and where the parameter is a packt device allocated with packt_device_alloc.
```
## Deep inside LDM
Under the hood, the LDM relies on three important structures, which are kobject,
kobj_type, and kset. Let us see how each of these structures is involved in the device model.
## kobject structure kobject is the core of the device model, running behind the scenes. It brings an OO-like programming style to the kernel, and is mainly used for reference counting and to expose devices hierarchies and relationships between them. kobjects introduce the concept of encapsulation of common object properties, such as usage reference counts:
```c
struct kobject {
```
const char *name;
```c
struct list_head entry;
struct kobject *parent;
struct kset *kset;
struct kobj_type *ktype;
struct sysfs_dirent *sd;
struct kref kref;
```
/* Fields out of our interest have been removed */
```c
};
```
name points to the name of this kobject. One can change this using the kobject_set_name(struct kobject *kobj, const char *name)
function.
parent is a pointer to this kobject's parent. It is used to build a hierarchy to describe the relationship between objects.
sd points to a struct sysfs_dirent structure that represents this kobject in sysfs inode inside this structure for sysfs.
kref provides reference counting on the kobject.
ktype describes the object, and kset tells us which set (group) of objects this object belongs to.
Each structure that embeds a kobject is embedded and receives the standardized functions that kobjects provide. The embedded kobject will enable the structure to become a part of an object hierarchy.
The container_of macro is used to get a pointer on the object to which the kobject belongs. Every kernel device directly or indirectly embeds a kobject property. Prior to being added to the system, the kobject must be allocated using the kobject_create() function,
which will return an empty kobject that one must initialize with kobj_init(), given as a parameter the allocated and non-initialized kobject pointer, along with its kobj_type pointer:
```c
struct kobject *kobject_create(void)
void kobject_init(struct kobject *kobj, struct kobj_type *ktype)
```
The kobject_add() function is used to add and link a kobject to the system, at the same time creating its directory according to its hierarchy, along with its default attributes. The reverse function is kobject_del():
```c
int kobject_add(struct kobject *kobj, struct kobject *parent,
```
const char *fmt, ...);
The reverse function of both kobject_create and kobject_add is kobject_put. In the source provided with the book, the following excerpt ties a kobject to the system:
/* Somewhere */
```c
static struct kobject *mykobj;
```
mykobj = kobject_create();
```c
if (mykobj) {
kobject_init(mykobj, &mytype);
if (kobject_add(mykobj, NULL, "%s", "hello")) {
```
err = -1;
```c
printk("ldm: kobject_add() failed\n");
kobject_put(mykobj);
```
mykobj = NULL;
```c
}
```
err = 0;
```c
}
```
One could have used kobject_create_and_add, which internally calls kobject_create and kobject_add. The following excerpt from drivers/base/core.c shows how to use it:
```c
static struct kobject * class_kobj = NULL;
static struct kobject * devices_kobj = NULL;
```
/* Create /sys/class */
```c
class_kobj = kobject_create_and_add("class", NULL);
if (!class_kobj) {
return -ENOMEM;
}
```
/* Create /sys/devices */
devices_kobj = kobject_create_and_add("devices", NULL);
```c
if (!devices_kobj) {
return -ENOMEM;
}
```
If a kobject has a NULL parent, then kobject_add sets the parent to kset.
If both are NULL, the object becomes a child-member of the top-level sys directory.
## kobj_type
A struct kobj_type structure describes the behavior of kobjects. A kobj_type structure describes the type of object that embeds a kobject by means of the ktype field. Every structure that embeds a kobject needs a corresponding kobj_type, which will control what happens when the kobject is created and destroyed, and when attributes are read or written to. Every kobject has a field of the struct kobj_type type, which stands for kernel object type:
```c
struct kobj_type {
void (*release)(struct kobject *);
```
const struct sysfs_ops sysfs_ops;
```c
struct attribute **default_attrs;
};
```
A struct kobj_type structure allows kernel objects to share common operations
(sysfs_ops), whether those objects are functionally related or not. Fields of that structure are meaningful enough. release is a callback called by the kobject_put() function whenever your object needs to be freed. You must free memory held by your object here.
One can use the container_of macro to get a pointer to the object. The sysfs_ops field points to sysfs operations, whereas default_attrs defines the default attributes associated with this kobject. sysfs_ops is a set of callbacks (sysfs operation) called when a sysfs attribute is accessed. default_attrs is a pointer to a list of struct attribute elements that will be used as default attributes for each object of this type:
```c
struct sysfs_ops {
```
ssize_t (*show)(struct kobject *kobj,
```c
struct attribute *attr, char *buf);
```
ssize_t (*store)(struct kobject *kobj,
```c
struct attribute *attr,const char *buf,
```
size_t size);
```c
};
```
show is the callback called when one reads an attribute of any kobject that has this kobj_type. The buffer size is always PAGE_SIZE in length, even if the value to show is a simple char. One should set the value of buf (using scnprintf ), and return the size (in bytes) of data actually written into the buffer on success, or negative error on failure. store is called for write purposes. Its buf parameter is, at most, PAGE_SIZE, but can be smaller. It returns the size (in bytes) of data actually read from buffer on success, or negative error on failure (or if it receives an unwanted value). One can use get_ktype to get the kobj_type of a given kobject:
```c
struct kobj_type *get_ktype(struct kobject *kobj);
```
In the example in this book, our k_type variable represents our kobject's type:
```c
static struct sysfs_ops s_ops = {
```
.show = show,
.store = store,
```c
};
static struct kobj_type k_type = {
```
.sysfs_ops = &s_ops,
.default_attrs = d_attrs,
```c
};
```
Here, the show and store callbacks are defined as follows:
```c
static ssize_t show(struct kobject *kobj, struct attribute *attr, char
```
*buf)
```c
{
struct d_attr *da = container_of(attr, struct d_attr, attr);
printk( "LDM show: called for (%s) attr\n", da->attr.name );
return scnprintf(buf, PAGE_SIZE,
"%s: %d\n", da->attr.name, da->value);
}
static ssize_t store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t len)
{
struct d_attr *da = container_of(attr, struct d_attr, attr);
sscanf(buf, "%d", &da->value);
printk("LDM store: %s = %d\n", da->attr.name, da->value);
return sizeof(int);
}
```
## ksets
Kernel object sets (ksets) mainly group related kernel objects together. ksets are a collection of kobjects. In other words, a kset gathers related kobjects into a single place, for example, all block devices:
```c
struct kset {
struct list_head list;
```
spinlock_t list_lock;
```c
struct kobject kobj;
};
```
The preceding structure can be explained as follows:
list is a linked list of all kobjects in the kset list_lock is a spinlock protecting linked list access kobj represents the base class for the set
Each registered (added-to-the-system) kset corresponds to a sysfs directory. A kset can be created and added using the kset_create_and_add() function, and removed with the kset_unregister() function:
```c
struct kset * kset_create_and_add(const char *name,
```
const struct kset_uevent_ops *u,
```c
struct kobject *parent_kobj);
void kset_unregister (struct kset * k);
```
Adding a kobject to the set is as simple as specifying its kset field to the right kset:
```c
static struct kobject foo_kobj, bar_kobj;
```
example_kset = kset_create_and_add("kset_example", NULL, kernel_kobj);
/*
* since we have a kset for this kobject,
* we need to set it before calling the kobject core.
*/
foo_kobj.kset = example_kset;
bar_kobj.kset = example_kset;
retval = kobject_init_and_add(&foo_kobj, &foo_ktype,
NULL, "foo_name");
retval = kobject_init_and_add(&bar_kobj, &bar_ktype,
NULL, "bar_name");
Now, note the following in the module exit function, after the kobject and its attributes have been removed:
```c
kset_unregister(example_kset);
```
## Attributes
Attributes are sysfs files exported to the user space by kobjects. An attribute represents an object property that can be readable, writable, or both, from the user space. That said, every data structure that embeds a struct kobject can expose either default attributes provided by the kobject itself (if any), or custom ones. In other words, attributes map kernel data to files in sysfs.
An attribute definition looks like this:
```c
struct attribute {
```
char * name;
```c
struct module *owner;
```
umode_t mode;
```c
};
```
The kernel functions used to add/remove attributes from the filesystem are as follows:
```c
int sysfs_create_file(struct kobject * kobj,
```
const struct attribute * attr);
```c
void sysfs_remove_file(struct kobject * kobj,
```
const struct attribute * attr);
Let us try to define two properties that we will export, each represented by an attribute:
```c
struct d_attr {
struct attribute attr;
int value;
};
static struct d_attr foo = {
```
.attr.name="foo",
.attr.mode = 0644,
.value = 0,
```c
};
static struct d_attr bar = {
```
.attr.name="bar",
.attr.mode = 0644,
.value = 0,
```c
};
```
To create each enumerated attribute separately, we have to call the following:
```c
sysfs_create_file(mykobj, &foo.attr);
sysfs_create_file(mykobj, &bar.attr);
```
A good place to start with attributes is samples/kobject/kobject-example.c, in the kernel source.
## The attributes group
So far, we have seen how to individually add attributes and call (directly or indirectly through a wrapper function such as device_create_file(), class_create_file(),
and so on) sysfs_create_file() on each of them. Why bother with multiple calls if we can do it once? Here is where the attribute group comes in. It relies on the struct attribute_group structure:
```c
struct attribute_group {
struct attribute **attrs;
};
```
Of course, we have removed fields that are not of interest. The attrs field is a pointer to a
NULL-terminated list of attributes. Each attribute group must be given a pointer to a list/array of struct attribute elements. The group is just a helper wrapper that makes it easier to manage multiple attributes.
The kernel functions used to add/remove group attributes to the filesystem are:
```c
int sysfs_create_group(struct kobject *kobj,
```
const struct attribute_group *grp)
```c
void sysfs_remove_group(struct kobject * kobj,
```
const struct attribute_group * grp)
The two previously defined properties can be embedded in a struct attribute_group,
to make only one call to add both of them to the system:
```c
static struct d_attr foo = {
```
.attr.name="foo",
.attr.mode = 0644,
.value = 0,
```c
};
static struct d_attr bar = {
```
.attr.name="bar",
.attr.mode = 0644,
.value = 0,
```c
};
```
/* attrs is a pointer to a list (array) of attributes */
```c
static struct attribute * attrs [] =
{
```
&foo.attr,
&bar.attr,
NULL,
```c
};
static struct attribute_group my_attr_group = {
```
.attrs = attrs,
```c
};
```
The one and only function to call here is this:
```c
sysfs_create_group(mykobj, &my_attr_group);
```
This is much better than making a call for each attribute.
## The device model and sysfs
Sysfs is a non-persistent virtual filesystem that provides a global view of the system and exposes the kernel object's hierarchy (topology) by means of their kobjects. Each kobject shows up as a directory, and files in a directory representing kernel variables, exported by the related kobject. These files are called attributes, and can be read or written.
Any registered kobject creates a directory in sysfs; where the directory is created depends on the kobject's parent (which is a kobject too). It is natural that directories are created as subdirectories of the kobject's parent. This highlights internal object hierarchies to the user space. Top-level directories in sysfs represent common ancestors of object hierarchies, that is, the subsystems the objects belong to.
Top-level sysfs directories can be found under the /sys/ directory:
```bash
/sys$ tree -L 1
```
├── block
├── bus
├── class
├── dev
├── devices
├── firmware
├── fs
├── hypervisor
├── kernel
├── module
└── power block contains a directory per-block device on the system, each of which contains subdirectories for partitions on the device. bus contains the registered bus on the system.
dev contains the registered device nodes in a raw way (no hierarchy), each being a symlink to the real device in the /sys/devices directory. devices gives a view of the topology of devices in the system. firmware shows a system-specific tree of low-level subsystems, such as: ACPI, EFI, OF (DT). fs lists filesystems actually used on the system. kernel holds kernel configuration options and status info. Modules is a list of loaded modules.
Each of these directories corresponds to a kobject, some of which are exported as kernel symbols. These are:
kernel_kobj, which corresponds to /sys/kernel power_kobj for /sys/power firmware_kobj, which is for /sys/firmware, exported in the drivers/base/firmware.c source file hypervisor_kobj for /sys/hypervisor, exported in the drivers/base/hypervisor.c fs_kobj, which corresponds to /sys/fs, exported in the fs/namespace.c file
However, class/, dev/, and devices/ are created during the boot up by the devices_init function in drivers/base/core.c in the kernel source, block/ is created in block/genhd.c, and bus/ is created as a kset in drivers/base/bus.c.
```c
When a kobject directory is added to sysfs (using kobject_add), where it is added depends on the kobject's parent location. If its parent pointer is set, it is added as a subdirectory inside the parent's directory. If the parent pointer is NULL, it is added as a subdirectory inside kset->kobj. If neither parent nor kset fields are set, it maps to the root level directory in sysfs (/sys).
```
One can create/remove symbolic links on existing objects (directories), using the sysfs_{create|remove}_link functions:
```c
int sysfs_create_link(struct kobject * kobj,
struct kobject * target, char * name);
void sysfs_remove_link(struct kobject * kobj, char * name);
```
This will allow an object to exist in more than one place. The create function will create a symlink named name pointing to the target kobject sysfs entry. A well-known example is devices appearing in both /sys/bus and /sys/devices. Symbolic links created will be persistent even after target removal. You have to know when the target is removed, and then remove the corresponding symlink.
## Sysfs files and attributes
Now we know that the default set of files is provided through the ktype field in kobjects and ksets, through the default_attrs field of kobj_type. Default attributes will be sufficient in most cases. But, sometimes, an instance of a ktype may need its own attributes to provide data or functionality not shared by a more general ktype.
Just to recap; the low-level functions used to add/remove new attributes (or a group of attributes) on top of the default set are:
```c
int sysfs_create_file(struct kobject *kobj,
```
const struct attribute *attr);
```c
void sysfs_remove_file(struct kobject *kobj,
```
const struct attribute *attr);
```c
int sysfs_create_group(struct kobject *kobj,
```
const struct attribute_group *grp);
```c
void sysfs_remove_group(struct kobject * kobj,
```
const struct attribute_group * grp);
## Current interfaces
There are interface layers that currently exist in sysfs. Apart from creating your own ktype or kobject to add your attributes, you can use those that currently exist: device, driver, bus,
and class attributes. Their descriptions are as follows:
## Device attributes
Apart from default attributes provided by the kobject embedded in your device structure,
you can create custom ones. The structure used for this purpose is struct device_attribute, which is nothing but a wrapping around the standard struct attribute, and a set of callbacks to show/store the value of the attribute:
```c
struct device_attribute {
struct attribute attr;
```
ssize_t (*show)(struct device *dev,
```c
struct device_attribute *attr,
```
char *buf);
ssize_t (*store)(struct device *dev,
```c
struct device_attribute *attr,
```
const char *buf, size_t count);
```c
};
```
Their declaration is done through the DEVICE_ATTR macro:
```c
DEVICE_ATTR(_name, _mode, _show, _store);
```
Whenever you declare a device attribute using DEVICE_ATTR, the prefix dev_attr_ is added to the attribute name. For example, if you declare an attribute with the _name parameter set to foo, the attribute will be accessible through the dev_attr_foo variable name.
To understand why, let us see how the DEVICE_ATTR macro is defined in include/linux/device.h:
```c
#define DEVICE_ATTR(_name, _mode, _show, _store) \
struct device_attribute dev_attr_##_name = __ATTR(_name, _mode, _show,
```
_store)
Finally, you can add/remove those using the device_create_file and device_remove_file functions:
```c
int device_create_file(struct device *dev,
```
const struct device_attribute * attr);
```c
void device_remove_file(struct device *dev,
```
const struct device_attribute * attr);
The following sample is a demonstration of how to put it all together:
```c
static ssize_t foo_show(struct device *child,
struct device_attribute *attr, char *buf)
{
return sprintf(buf, "%d\n", foo_value);
}
static ssize_t bar_show(struct device *child,
struct device_attribute *attr, char *buf)
{
return sprintf(buf, "%d\n", bar_value);
}
```
Here are the static declarations of the attribute:
```c
static DEVICE_ATTR(foo, 0644, foo_show, NULL);
static DEVICE_ATTR(bar, 0644, bar_show, NULL);
```
The following code shows how to actually create files on the system:
```c
if ( device_create_file(dev, &dev_attr_foo) != 0 )
```
/* handle error */
```c
if ( device_create_file(dev, &dev_attr_bar) != 0 )
```
/* handle error*/
For cleanup, attribute removal is done in the remove function, as follows:
```c
device_remove_file(wm->dev, &dev_attr_foo);
device_remove_file(wm->dev, &dev_attr_bar);
```
You may wonder how and why we used to define the same set of store/show callbacks for all attributes of the same kobject/ktype, and why we now use a custom one for each attribute. The first reason is because the device subsystem defines its own attribute structure, which wraps the standard one; secondly, instead of showing/storing the value of the attribute, it uses the container_of macro to extract the struct device_attribute giving a generic struct attribute, and then executes the show/store callback depending on the user action. The following excerpt from drivers/base/core.c shows sysfs_ops for the device kobject:
```c
static ssize_t dev_attr_show(struct kobject *kobj,
struct attribute *attr,
```
char *buf)
```c
{
struct device_attribute *dev_attr = to_dev_attr(attr);
struct device *dev = kobj_to_dev(kobj);
```
ssize_t ret = -EIO;
```c
if (dev_attr->show)
ret = dev_attr->show(dev, dev_attr, buf);
if (ret >= (ssize_t)PAGE_SIZE) {
```
print_symbol("dev_attr_show: %s returned bad count\n",
```c
(unsigned long)dev_attr->show);
}
return ret;
}
static ssize_t dev_attr_store(struct kobject *kobj, struct attribute *attr,
```
const char *buf, size_t count)
```c
{
struct device_attribute *dev_attr = to_dev_attr(attr);
struct device *dev = kobj_to_dev(kobj);
```
ssize_t ret = -EIO;
```c
if (dev_attr->store)
ret = dev_attr->store(dev, dev_attr, buf, count);
return ret;
}
static const struct sysfs_ops dev_sysfs_ops = {
```
.show = dev_attr_show,
.store = dev_attr_store,
```c
};
```
The principle is the same for bus (in drivers/base/bus.c), driver (in drivers/base/bus.c), and class (in drivers/base/class.c) attributes. They use the container_of macro to extract their specific attribute structure, and then call the show/store callback embedded in it.
## Bus attributes
These rely on the struct bus_attribute structure:
```c
struct bus_attribute {
struct attribute attr;
```
ssize_t (*show)(struct bus_type *, char * buf);
ssize_t (*store)(struct bus_type *, const char * buf, size_t count);
```c
};
```
Bus attributes are declared using the BUS_ATTR macro:
```c
BUS_ATTR(_name, _mode, _show, _store)
```
Any bus attribute declared using BUS_ATTR will have the prefix bus_attr_ added to the attribute variable name:
```c
#define BUS_ATTR(_name, _mode, _show, _store) \
struct bus_attribute bus_attr_##_name = __ATTR(_name, _mode, _show, _store)
```
They are created/removed using the bus_{create|remove}_file functions:
```c
int bus_create_file(struct bus_type *, struct bus_attribute *);
void bus_remove_file(struct bus_type *, struct bus_attribute *);
```
## Device driver attributes
The structure used here is struct driver_attribute:
```c
struct driver_attribute {
struct attribute attr;
```
ssize_t (*show)(struct device_driver *, char * buf);
ssize_t (*store)(struct device_driver *, const char * buf,
size_t count);
```c
};
```
The declaration relies on the DRIVER_ATTR macro, which will prefix the attribute variable name with DRIVER_ATTR:
```c
DRIVER_ATTR(_name, _mode, _show, _store)
```
The macro definition is as follows:
```c
#define DRIVER_ATTR(_name, _mode, _show, _store) \
struct driver_attribute driver_attr_##_name = __ATTR(_name, _mode, _show,
```
_store)
Creation/removal relies on the driver_{create|remove}_file functions:
```c
int driver_create_file(struct device_driver *,
```
const struct driver_attribute *);
```c
void driver_remove_file(struct device_driver *,
```
const struct driver_attribute *);
## Class attributes
The struct class_attribute is the base structure here:
```c
struct class_attribute {
struct attribute attr;
```
ssize_t (*show)(struct device_driver *, char * buf);
ssize_t (*store)(struct device_driver *, const char * buf,
size_t count);
```c
};
```
The declaration of a class attribute relies on CLASS_ATTR:
```c
CLASS_ATTR(_name, _mode, _show, _store)
```
As the macro's definition shows, any class attribute declared with CLASS_ATTR will have the prefix class_attr_ added to the attribute variable name:
```c
#define CLASS_ATTR(_name, _mode, _show, _store) \
struct class_attribute class_attr_##_name = __ATTR(_name, _mode, _show,
```
_store)
Finally, file creation and removal are done with the class_{create|remove}_file functions:
```c
int class_create_file(struct class *class,
```
const struct class_attribute *attr);
```c
void class_remove_file(struct class *class,
```
const struct class_attribute *attr);
Notice that device_create_file(), bus_create_file(),
```c
driver_create_file(), and class_create_file() all make an internal call to sysfs_create_file(). As they all are kernel objects,
```
they have a kobject embedded into their structure. That kobject is then passed as a parameter to sysfs_create_file, as you can see in the following code.
```c
int device_create_file(struct device *dev,
```
const struct device_attribute *attr)
```c
{
```
[...]
```c
error = sysfs_create_file(&dev->kobj, &attr->attr);
```
[...]
```c
}
int class_create_file(struct class *cls,
```
const struct class_attribute *attr)
```c
{
```
[...]
error =
```c
sysfs_create_file(&cls->p->class_subsys.kobj,
&attr->attr);
return error;
}
int bus_create_file(struct bus_type *bus,
struct bus_attribute *attr)
{
```
[...]
error =
```c
sysfs_create_file(&bus->p->subsys.kobj,
&attr->attr);
```
[...]
```c
}
```
## Allowing sysfs attribute files to be pollable
Here we will see how not to make CPU wasting polling to sense sysfs attributes data availability. The idea is to use poll or select system calls to wait for the attribute's content to change. The patch to make sysfs attributes pollable was created by Neil Brown and Greg Kroah-Hartman. The kobject manager (the driver which has access to the kobject)
must support notification to allow poll or select to return (be released) when the content changes. The magic function that does the trick comes from the kernel side, and is sysfs_notify():
```c
void sysfs_notify(struct kobject *kobj, const char *dir,
```
const char *attr)
If the dir parameter is non-null, it is used to find a subdirectory, which contains the attribute (presumably created by sysfs_create_group). This has a cost of one int per attribute, one wait_queuehead per kobject, and one int per open file.
poll will return POLLERR|POLLPRI, and select will return the fd whether it is waiting for reads, writes, or exceptions. The blocking poll is from the user's side. sysfs_notify()
should be called only after you have adjusted your kernel attribute value.
Think of the poll() (or select()) code as a subscriber to notice a change in an attribute of interest, and sysfs_notify() as a publisher,
notifying subscribers of any changes.
The following excerpted code provided with the book shows the store function of an attribute:
```c
static ssize_t store(struct kobject *kobj, struct attribute *attr,
```
const char *buf, size_t len)
```c
{
struct d_attr *da = container_of(attr, struct d_attr, attr);
sscanf(buf, "%d", &da->value);
printk("sysfs_foo store %s = %d\n", a->attr.name, a->value);
if (strcmp(a->attr.name, "foo") == 0){
foo.value = a->value;
sysfs_notify(mykobj, NULL, "foo");
}
else if(strcmp(a->attr.name, "bar") == 0){
bar.value = a->value;
sysfs_notify(mykobj, NULL, "bar");
}
return sizeof(int);
}
```
The code from the user space must behave like this in order to sense the data change:
1. Open the file attributes.
2. Make a dummy read of all the contents.
3. Call poll requesting POLLERR|POLLPRI (select/exceptfds works too).
4. When poll (or select) returns (which indicates that a value has changed), read the content of files whose data changed.
5. Close the files and go to the top of the loop.
When in doubt about whether a sysfs attribute is pollable, set a suitable timeout value. The user space example is provided with the book sample.
## Summary
Now you are familiar with the concept of LDM and with its data structures (bus, class,
device drivers, and devices), including low-level data structures, which are kobject, kset,
and kobj_types, and attributes (or a group of those). How objects are represented within the kernel (hence sysfs and device topology) is not a secret anymore. You will be able to create an attribute (or group), exposing your device or driver feature through sysfs. If this topic seems clear to you, we will move on to the next, in chapter 14, Pin Control and GPIO
Subsystem, which heavily utilizes the power of sysfs