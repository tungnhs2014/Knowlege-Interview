# Chapter 8 - Integrating with

## V4L2 Async and

## Media Controller

## Frameworks
Over time, media support has become a must and a saling argument for System on Chips
(SoCs), which keep becoming more and more complex. The complexity of those media
IP cores is such that grabbing sensor data requires a whole pipeline (made of several
sub-devices) to be set up by the software. The asynchronous nature of a device tree-based
system means the setup and probing of those sub-devices are not straightforward. Thus
entered the async framework, which addresses the unordered probing of sub-devices in
order for the media device to be popped on time, when all of the media sub-devices are
ready. Last but not least, because of the complexity of the media pipe, it became necessary
to find a way to ease the configuration of the sub-devices it is made of. Thus came the
media controller framework, which wraps the whole media pipe in a single element, the
media device. It comes with some abstractions, one of which is that each sub-device is
considered as an entity, with either a sink pad, a source pad, or both.
350 Integrating with V4L2 Async and Media Controller Frameworks
This chapter will focus on how both the async and media controller frameworks work
and how they are designed, and we will go through their APIs to learn how to leverage
them in Video4Linux2 (V4L2) device driver development.

## In other words, in this chapter, we will cover the following topics:
- The V4L2 async interface and the concept of graph binding
- The V4L2 async and graph-oriented API
- The V4L2 async framework and APIs
- The Linux media controller framework
## Technical requirements

## In this chapter, you'll need the following elements:
- Advanced computer architecture knowledge and C programming skills
- Linux kernel v4.19.X sources, available at https://git.kernel.org/pub/
scm/linux/kernel/git/stable/linux.git/refs/tags

## The V4L2 async interface and the concept of
graph binding
So far, with V4L2 driver development, we have not actually dealt with the probing
order. That being said, we considered the synchronous approach, where bridge device
drivers register devices for all sub-devices synchronously during their probing. However,
this approach cannot be used with intrinsically asynchronous and unordered device
registration systems, such as the flattened device tree. To address this, what we currently
call the async interface has been introduced.
With this new approach, bridge drivers register lists of sub-device descriptors and notifier
callbacks, and sub-device drivers register sub-devices that they are about to probe or
have successfully probed. The async core will take care of matching sub-devices against
hardware descriptors and calling bridge driver callbacks when matches are found.
Another callback is called when the sub-device is unregistered. The async subsystem relies
on device declaration in a special way, called graph binding, which we will deal with in
the next section.

## The V4L2 async interface and the concept of graph binding 351

## Graph binding
Embedded systems have a reduced set of devices, some of which are not discoverable.
The device tree, however, came into the picture to allow describing the actual system
(from a hardware point of view) to the kernel. Sometimes (if not always), these devices
are somehow interconnected.
While the phandle properties pointing to other nodes could be used in the device tree
to describe simple and direct connections, such as parent/child relationships, there was no
way to model compound devices made of several interconnections. There were situations
where the relationship modeling resulted in a quite complete graph – for example, the
i.MX6 Image Processing Unit (IPU), which is a logical device on its own, but made up of
several physical IP blocks whose interconnections may result in a quite complex pipe.
This is where the so-called Open Firmware (OF) graph intervenes, along with its API and
some new concepts, the concepts of port and endpoint:
- A port can be seen as an interface in a device (as in an IP block).
- An endpoint can be seen as a pad, as it describes one end of a connection to a
remote port.
However, phandle properties are still used to refer to other nodes in the tree. More
documentation on this can be found in Documentation/devicetree/bindings/
graph.txt.

## Port and endpoint representations
A port is an interface to a device. A device can have one or several ports. Ports are
represented by port nodes contained in the node of the device they belong to. Each port
node contains an endpoint subnode for each remote device port to which this port is
connected. This means a single port can be connected to more than one port on the
remote device(s) and that each link must be represented by an endpoint child node. Now,
if a device node contains more than one port, if there is more than one endpoint at a port,
or a port node needs to be connected to a selected hardware interface, a popular scheme
using the #address-cells, #size-cells, and reg properties is used to number
the nodes.

## The following excerpt shows how to use the #address-cells, #size-cells, and
reg properties to handle those cases:
device {
...
#address-cells = <1>;
352 Integrating with V4L2 Async and Media Controller Frameworks
#size-cells = <0>;
port@0 {
#address-cells = <1>;
#size-cells = <0>;
reg = <0>;
endpoint@0 {
reg = <0>;
...
```c
};
```
endpoint@1 {
reg = <1>;
...
```c
};
};
```
port@1 {
reg = <1>;
endpoint { ... };
```c
};
};

```
## Complete documentation of this can be found in Documentation/devicetree/
bindings/graph.txt. Now that we are done with port and endpoint representation,
we need to learn how to link each with the other, as explained in the next section.

## Endpoint linking
For two endpoints to be linked together, each of them should contain a remoteendpoint phandle property that points to the corresponding endpoint in the port of
the remote device. In turn, the remote endpoint should contain a remote-endpoint
property. Two endpoints with their remote-endpoint phandles pointing at each other
form a link between the containing ports, as in the following example:
device-1 {
port {
device_1_output: endpoint {

## The V4L2 async interface and the concept of graph binding 353
remote-endpoint = <&device_2_input>;
```c
};
};
};
```
device-2 {
port {
device_2_input: endpoint {
remote-endpoint = <&device_1_output>;
```c
};
};
}
```
Introducing the graph binding concept without talking at all about its API would be
a waste of time. Let's jump to the API that comes along with this new binding method.

## The V4L2 async and graph-oriented API
This section heading must not mislead you since graph binding is not just intended for the
V4L2 subsystem. The Linux DRM subsystem also takes advantage of it. That being said, the
async framework heavily relies on the device tree to describe either media devices along
with their endpoints and connections, or links between those endpoints along with their
bus configuration properties.
From the DT (of_graph_*) API to the generic fwnode graph API
(fwnode_graph_*)
The fwnode graph API is a successful attempt at changing the device tree-only-based
OF graph API to a generic API, merging both ACPI and device tree OF APIs together in
order to have unified and generic APIs. This extends the concept of the graph with ACPI
by using the same APIs. By having a look at the struct device_node and struct
acpi_device structures, you can see the members they have in common: struct
fwnode_handle fwnode:
```c
struct device_node {
```
[...]
```c
struct fwnode_handle fwnode;
```
[...]
```c
};
```
354 Integrating with V4L2 Async and Media Controller Frameworks
The preceding excerpt represents a device node from a device tree point of view, while the
following is related to ACPI:
```c
struct acpi_device {
```
[...]
```c
struct fwnode_handle fwnode;
```
[...]
```c
};
```
The fwnode member, which is of the struct fwnode_handle type, is a lower level
and generic data structure abstracting either device_node or acpi_device as they
both inherit from this data structure. This makes struct fwnode_handle a good
client for graph API homogenization so that an endpoint (by means of its field of the
fwnode_handle type) can refer to either an ACPI device or an OF-based device. This
abstraction model is now used in graph APIs, allowing us to abstract an endpoint by
a generic data structure (struct fwnode_endpoint, described as follows) embedding
a pointer to struct fwnode_handle, which may refer to either an ACPI or OF node.
In addition to the genericity, this allows the underlying sub-device to this endpoint to be
either ACPI- or OF-based:
```c
struct fwnode_endpoint {
```
unsigned int port;
unsigned int id;
const struct fwnode_handle *local_fwnode;
```c
};
```
This structure deprecates the old struct of_endpoint structure and the member of
type device_node* leaves room for a member of the fwnode_handle* type. In the
preceding structure, local_fwnode points to the related firmware node, port is the
port number (that is, it corresponds to 0 in port@0 or 1 in port@1), and id is the index
of this endpoint from within the port (that is, it corresponds to the 0 in endpoint@0
and to the 1 in endpoint@1).
The V4L2 framework uses this model for abstracting V4L2-related endpoints by means
of struct v4l2_fwnode_endpoint, which is built on top of fwnode_endpoint,
as follows:
```c
struct v4l2_fwnode_endpoint {
struct fwnode_endpoint base;
/*
```
* Fields below this line will be zeroed by

## The V4L2 async interface and the concept of graph binding 355
* v4l2_fwnode_endpoint_parse()
```c
*/
enum v4l2_mbus_type bus_type;
```
union {
```c
struct v4l2_fwnode_bus_parallel parallel;
struct v4l2_fwnode_bus_mipi_csi1 mipi_csi1;
struct v4l2_fwnode_bus_mipi_csi2 mipi_csi2;
} bus;
```
u64 *link_frequencies;
unsigned int nr_of_link_frequencies;
```c
};

```
## This structure deprecates and replaces struct v4l2_of_endpoint since kernel
v4.13, formerly used by V4L2 to represent endpoint nodes in the era of the V4L2 OF

## API. In the preceding data structure definition, base represents the struct
fwnode_endpoint structure of the underlying ACPI or device node. Other fields
are V4L2-related, as follows:
- bus_type is the type of media bus through which this sub-device streams data.

## The value of this member determines which underlying bus structure should be
filled with the parsed bus properties from the fwnode endpoint (either device tree
or ACPI). Possible values are listed in enum v4l2_mbus_type, as follows:
```c
enum v4l2_mbus_type {

```
## V4L2_MBUS_PARALLEL,

## V4L2_MBUS_BT656,

## V4L2_MBUS_CSI1,

## V4L2_MBUS_CCP2,

## V4L2_MBUS_CSI2,
```c
};
```
- bus is the structure representing the media bus itself. Possible values are already
present in the union, and bus_type determines which one to consider. These bus
structures are all defined in include/media/v4l2-fwnode.h.
- link_frequencies is the list of frequencies supported by this link.
- nr_of_link_frequencies is the number of elements in link_
frequencies.
356 Integrating with V4L2 Async and Media Controller Frameworks

## Important note

## In kernel v4.19, the bus_type member is exclusively set according to the
bus-type property in fwnode. The driver can check the read value and
adapt its behavior. This means the V4L2 fwnode API will always base its
parsing strategy on this fwnode property. However, as of kernel v5.0, drivers
have to set this member to an expected bus type (prior to calling the parsing
function), which will be compared to the value of the bus-type property
read in fwnode and will raise an error if they don't match. If the bus type is
not known or if the driver can deal with several bus types, the V4L2_MBUS_
UNKNOWN value has to be used. This value is also part of enum v4l2_
mbus_type, as of kernel v5.0.

## In the kernel code, you may find the enum v4l2_fwnode_bus_type
```c
enum type. This is a V4L2 fwnode local enum type that is the counterpart
```
of the global enum v4l2_mbus_type enum type and whose values map
each other. Their respective values are kept in sync as the code evolves.
The V4L2-related binding then requires additional properties. Part of these properties
is used to build v4l2_fwnode_endpoint, while the other part is used to build the
underlying bus (the media bus, actually) structure. All are described in a dedicated and
video-related binding documentation, Documentation/devicetree/bindings/
media/video- interfaces.txt, which I strongly recommend checking out.
The following is a typical binding between a bridge (isc) and a sensor sub-device
(mt9v032):
&i2c1 {
#address-cells = <1>;
#size-cells = <0>;
mt9v032@5c {
compatible = "aptina,mt9v032";
reg = <0x5c>;
port {
mt9v032_out: endpoint {
remote-endpoint = <&isc_0>;
link-frequencies =
/bits/ 64 <13000000 26600000 27000000>;
hsync-active = <1>;
vsync-active = <0>;
pclk-sample = <1>;

## The V4L2 async interface and the concept of graph binding 357
```c
};
};
};
};
```
&isc {
port {
isc_0: endpoint@0 {
remote-endpoint = <&mt9v032_out>;
hsync-active = <1>;
vsync-active = <0>;
pclk-sample = <1>;
```c
};
};
};

```
## In the preceding binding, hsync-active, vsync-active, link-frequencies,
and pclk- sample are all V4L2-specific properties and describe the media bus. Their
values are not coherent here and do not really make sense but fit well for our learning
purpose. This excerpt shows well the concepts of endpoint and remote endpoint; the use
of struct v4l2_fwnode_endpoint is discussed in detail in the The Linux media
controller framework section.

## Important note
The part of V4L2 dealing with the fwnode API is called the V4L2 fwnode
API. It is a replacement of the device tree-only API, the V4L2 OF API. The
former has a set of APIs prefixed with v4l2_fwnode_, while the second's
API set is prefixed with v4l2_of_. Do note that in OF-only-based APIs, an
endpoint is represented by struct of_endpoint, and a V4L2-related
endpoint is represented by struct v4l2_of_endpoint. There are APIs
that allow switching from OF- to fwnode-based models and vice versa.
V4L2 fwnode and V4L2 OF are fully interoperable. For example, a subdevice driver using V4L2 fwnode will work with a media device driver using
V4L2 OF without any effort, and vice versa! However, new drivers must use
the fwnode API, including #include <media/v4l2- fwnode.
h>, which should replace #include <media/v4l2-of.h> in the old
driver when switching to the fwnode API.
358 Integrating with V4L2 Async and Media Controller Frameworks
That being said, struct fwnode_endpoint, which was discussed earlier, is just for
showing the underlying mechanisms. We could have completely skipped it since only
the core deals with this data structure. For a more generic approach, instead of using
```c
struct device_node to refer to the device's firmware node, you're better off using the
```
new struct fwnode_handle. This definitely makes sure that DT and ACPI bindings
are compatible/interoperable using the same code in the driver. The following is a short
excerpt of how changes should look in new drivers:
- struct device_node *of_node;
+ struct fwnode_handle *fwnode;
- of_node = ddev->of_node;
+ fwnode = dev_fwnode(dev);

## Some of the common fwnode node-related APIs are as follows:
[...]
```c
struct fwnode_handle *fwnode_get_parent(
```
const struct fwnode_handle *fwnode);
```c
struct fwnode_handle *fwnode_get_next_child_node(
```
const struct fwnode_handle *fwnode,
```c
struct fwnode_handle *child);
struct fwnode_handle *fwnode_get_next_available_child_node(
```
const struct fwnode_handle *fwnode,
```c
struct fwnode_handle *child);
#define fwnode_for_each_child_node(fwnode, child) \
for (child = fwnode_get_next_child_node(fwnode, NULL);
```
child; \
child = fwnode_get_next_child_node(fwnode, child))
```c
#define fwnode_for_each_available_child_node(fwnode, child) \
for (child = fwnode_get_next_available_child_node(fwnode,
```
NULL);
child; \
child = fwnode_get_next_available_child_node(fwnode, child))

## The V4L2 async interface and the concept of graph binding 359
```c
struct fwnode_handle *fwnode_get_named_child_node(
```
const struct fwnode_handle *fwnode,
const char *childname);
```c
struct fwnode_handle *fwnode_handle_get(struct
```
fwnode_handle *fwnode);
```c
void fwnode_handle_put(struct fwnode_handle *fwnode);

```
## The aforementioned APIs have the following description:
- fwnode_get_parent() returns the parent handle of the node whose fwnode
value is given in an argument, or NULL otherwise.
- fwnode_get_next_child_node() takes a parent node as its first argument
and returns the next child (or NULL otherwise) after a given child (given as the
second argument) in this parent. If child (the second argument) is NULL, then the
first child of this parent will be returned.
- fwnode_get_next_available_child_node() is the same as fwnode_
get_next_child_node() but makes sure that the device actually exists (has
been probed successfully) prior to returning the fwnode handle.
- fwnode_for_each_child_node() iterates over the child in a given node (the
first argument) and the second argument is used as an iterator.
- fwnode_for_each_available_child_node is the same as fwnode_for_
each_child_node() but iterates only over nodes whose device is actually
present on the system.
- fwnode_get_named_child_node() gets a child in a given node by its name.
- fwnode_handle_get() obtains a reference to a device node and fwnode_
handle_put() drops this reference.

## Some of the fwnode-related properties are as follows:
[...]
bool fwnode_device_is_available(const
```c
struct fwnode_handle *fwnode);
```
bool fwnode_property_present(const
```c
struct fwnode_handle *fwnode,
```
const char *propname);
```c
int fwnode_property_read_string(const
```
360 Integrating with V4L2 Async and Media Controller Frameworks
```c
struct fwnode_handle *fwnode,
```
const char *propname,
const char **val);
```c
int fwnode_property_match_string(const
struct fwnode_handle *fwnode,
```
const char *propname,
const char *string);

## Both property- and node-related fwnode APIs are available in include/linux/
property.h. However, there are helpers that allow switching back and forth between

## OF, ACPI, and fwnode. The following is a short example:
```c
/* to switch from fwnode to of */
struct device_node *of_node = to_of_node(fwnode);
/* to switch from of to fw */
struct fwnode_handle *fwnode = of_fwnode_handle(node)
/* to switch from fwnode to acpi handle, the below macro has
```
* been introduced
*
* #define ACPI_HANDLE_FWNODE(fwnode) \
* acpi_device_handle(to_acpi_device_node(fwnode))
*
* and to switch from acpi device to fwnode:
*
* struct fwnode_handle *
* acpi_fwnode_handle(struct acpi_device *adev)
*
```c
*/

```
## The V4L2 async interface and the concept of graph binding 361
Finally, and most important for us, is the fwnode graph API. In the following code
snippet, we enumerate the most important function of this API:
```c
struct fwnode_handle
```
*fwnode_graph_get_next_endpoint(const
```c
struct fwnode_handle *fwnode,
struct fwnode_handle *prev);
struct fwnode_handle
```
*fwnode_graph_get_port_parent(const
```c
struct fwnode_handle *fwnode);
struct fwnode_handle
```
*fwnode_graph_get_remote_port_parent(
const struct fwnode_handle *fwnode);
```c
struct fwnode_handle
```
*fwnode_graph_get_remote_port(const
```c
struct fwnode_handle *fwnode);
struct fwnode_handle
```
*fwnode_graph_get_remote_endpoint(
const struct fwnode_handle *fwnode);
```c
#define fwnode_graph_for_each_endpoint(fwnode, child) \
for (child = NULL; \
```
(child = fwnode_graph_get_next_endpoint(fwnode, child)); )
```c
int fwnode_graph_parse_endpoint(const
struct fwnode_handle *fwnode,
struct fwnode_endpoint *endpoint);
```
[...]
362 Integrating with V4L2 Async and Media Controller Frameworks
Though the preceding function names talk about themselves, the following are better
descriptions of what they do:
- fwnode_graph_get_next_endpoint() returns the next endpoint (or NULL
otherwise) in a given node (the first argument) after a previous endpoint (prev,
the second argument). If prev is NULL, then the first endpoint is returned. This
function obtains a reference to the returned endpoint that must be dropped after
use. See fwnode_handle_put().
- fwnode_graph_get_port_parent() returns the parent of the port node
given in the argument.
- fwnode_graph_get_remote_port_parent() returns the firmware node
of the remote device containing the endpoint whose firmware node is given through
the fwnode argument.
- fwnode_graph_get_remote_endpoint() returns the firmware node of the
remote endpoint corresponding to a local endpoint whose firmware node is given
through the fwnode argument.
- fwnode_graph_parse_endpoint() parses common endpoint node properties
in fwnode (the first argument) representing a graph endpoint node and stores the
information in endpoint (the second and output argument). The V4L2 firmware
node API heavily uses this.
The V4L2 firmware node (V4L2 fwnode) API

## The main data structure in the V4L2 fwnode API is struct v4l2_fwnode_
endpoint. This structure is nothing but struct fwnode_handle augmented
with some V4L2-related properties. However, there is a V4L2-related fwnode graph
function that it is worth talking about here: v4l2_fwnode_endpoint_parse(). This
function's prototype is declared include/media/v4l2-fwnode.h, as follows:
```c
int v4l2_fwnode_endpoint_parse(struct fwnode_handle *fwnode,
struct v4l2_fwnode_endpoint *vep);

```
## The V4L2 async interface and the concept of graph binding 363
Given fwnode_handle (the first argument in the preceding function) of an endpoint,
you can use v4l2_fwnode_endpoint_parse() to parse all the fwnode node
properties. This function also recognizes and takes care of the V4L2-specific properties,
which are, if you remember, those documented in Documentation/devicetree/
bindings/media/video-interfaces.txt. v4l2_fwnode_endpoint_
parse() uses fwnode_graph_parse_endpoint() to parse common fwnode
properties and uses V4L2-specific parser helpers to parse V4L2-related properties. It
returns 0 on success or a negative error code on failure.
If we consider the mt9v032 CMOS image sensor node in dts, we can have the following
code in the probe method:
```c
int err;
struct fwnode_handle *ep;
struct v4l2_fwnode_endpoint bus_cfg;
/* We grab the fwnode corresponding to the device */
struct fwnode_handle *fwnode = dev_fwnode(dev);
/* We grab its endpoint(s) node */
```
ep = fwnode_graph_get_next_endpoint(fwnode, NULL);
```c
/* We parse the endpoint common properties as well as
```
* v4l2 related properties
```c
*/
```
err = v4l2_fwnode_endpoint_parse(ep, &bus_cfg);
```c
if (err) { /* handle error */ }
/* At this point we can access parameters such as bus_type,
```
* bus.flags
* (in case of mipi csi2 or parallel buses), V4L2_MBUS_*
* which are the
* media bus flags
```c
*/
/* we drop the reference on the enpoint */
```
fwnode_handle_put(ep);
364 Integrating with V4L2 Async and Media Controller Frameworks
The preceding code shows how you use the fwnode API, as well as its V4L2 version, for
accessing node and endpoint properties. There are, however, V4L2-specific properties
being parsed upon the v4l2_fwnode_endpoint_parse() call. These properties
describe the so-called media bus through which data is carried from one interface to
another. We will discuss this in the next section.

## V4L2 fwnode or media bus types
Most media devices support a particular media bus type. While endpoints are linked
together, they are actually connected through buses, whose properties need to be
described to the V4L2 framework. For V4L2 to be able to find this information, it
is provided as properties in the device's fwnode (DT or ACPI). As these are specific
properties, the V4L2 fwnode API is able to recognize and parse them. Each bus has its
specificities and properties.
First of all, let's have a look at the currently supported buses, along with their data
structures:
- MIPI CSI-1: This is MIPI Alliance's Camera Serial Interface (CSI) version 1. This
bus is represented as instances of struct v4l2_fwnode_bus_mipi_csi1.
- CCP2: This stands for Compact Camera Port 2, made by the Standard Mobile
Imaging Architecture (SMIA), which is an open standard for companies dealing
with camera modules for use in mobile applications (such as SMIA CCP2). This bus
is represented in this framework with instances of struct v4l2_fwnode_bus_
mipi_csi1 too.
- Parallel bus: This is the classic parallel interface, with HSYNC and VSYNC signals.

## The structure used to represent this bus is struct v4l2_fwnode_bus_
parallel.
- BT656: This is for BT.1120 or any parallel bus that transmits the conventional video
timing and synchronization signal (HSYNC, VSYNC, and BLANK) in the data. These
buses have a reduced number of pins compared to the standard parallel bus. This
framework uses struct v4l2_fwnode_bus_parallel to represent this bus.
- MIPI CSI-2: This is version 2 of MIPI Alliance's CSI interface. This bus is abstracted
by the struct v4l2_fwnode_bus_mipi_csi2 structure. However, this
data structure does not differentiate between D-PHY and C-PHY. This lack of
differentiation is addressed as of kernel v5.0.

## The V4L2 async interface and the concept of graph binding 365
As we will see later in the chapter, in the The concept of a media bus section, this concept
of a bus can be used to detect compatibility between a local endpoint and its remote
counterpart so that two sub-devices can't be linked together if they don't have the same
bus properties, which makes complete sense.

## Earlier, in the The V4L2 fwnode API section, we saw that v4l2_fwnode_endpoint_
parse() is responsible for parsing the endpoint's fwnode and filling the appropriate
bus structure. This function first calls fwnode_graph_parse_endpoint() in order
to parse the common fwnode graph-related properties, and then checks the value of the
bus-type property, as follows, in order to determine the appropriate v4l2_fwnode_
endpoint.bus data type:
u32 bus_type = 0;
fwnode_property_read_u32(fwnode, "bus-type", &bus_type);
Depending on this value, a bus data structure will be chosen. The following are expected
possible values from the fwnode device:
- 0: This means auto-detect. The core will try to guess the bus type according to the
properties present in the fwnode (MIPI CSI-2 D-PHY, parallel, or BT656).
- 1: This means MIPI CSI-2 C-PHY.
- 2: This means MIPI CSI-1.
- 3: This means CCP2.
For the CPP2 bus, for example, the device's fwnode would contain the following line:
bus-type = <3>;

## Important note
As of kernel v5.0, drivers can specify the expected bus type in the bus_type
member of v4l2_fwnode_endpoint prior to giving it as a second
argument to v4l2_fwnode_endpoint_parse(). This way, parsing
will fail if the value returned by the preceding fwnode_property_read_
u32 does not match the expected one, except if the expected bus type was set
to V4L2_MBUS_UNKNOWN.
366 Integrating with V4L2 Async and Media Controller Frameworks

## BT656 and parallel buses

## Those bus types are all represented by struct v4l2_fwnode_bus_parallel, as
follows:
```c
struct v4l2_fwnode_bus_parallel {
```
unsigned int flags;
unsigned char bus_width;
unsigned char data_shift;
```c
};
```
In the preceding data structure, flags represents the flags of the bus. Those flags will
be set according to the properties present in the device's firmware node. bus_width
represents the number of data lines actively used, not necessarily the total number of lines
of the bus. data_shift is used to specify which data lines are really used by specifying
the number of lines to skip prior to reaching the first active data line. The following
are the binding properties of these media buses, which are used to set up struct
```c
v4l2_fwnode_bus_parallel:
```
- hsync-active: Active state of the HSYNC signal; 0/1 for LOW/HIGH,
respectively. If this property's value is 0, then the V4L2_MBUS_HSYNC_ACTIVE_
LOW flag is set in the flags member. Any other value will set the V4L2_MBUS_

## HSYNC_ACTIVE_HIGH flag instead.
- vsync-active: Active state of the VSYNC signal; 0/1 for LOW/HIGH, respectively.

## If this property's value is 0, then the V4L2_MBUS_VSYNC_ACTIVE_LOW flag
is set in the flags member. Any other value will set the V4L2_MBUS_VSYNC_

## ACTIVE_HIGH flag instead.
- field-even-active: The field signal level during the even field data
transmission. This is the same as the preceding, but the concerned flags are

## V4L2_MBUS_FIELD_EVEN_HIGH and V4L2_MBUS_FIELD_EVEN_LOW.
- pclk-sample: Sample data on the rising (1) or falling (0) edge of the pixel clock
signal, V4L2_MBUS_PCLK_SAMPLE_RISING and V4L2_MBUS_PCLK_SAMPLE_

## FALLING.
- data-active: Similar to HSYNC and VSYNC, specifies data line polarity,

## V4L2_MBUS_DATA_ACTIVE_HIGH and V4L2_MBUS_DATA_ACTIVE_LOW.
- slave-mode: This is a Boolean property whose presence indicates that the link
is run in slave mode, and the V4L2_MBUS_SLAVE flag is set. Otherwise, the

## V4L2_MBUS_MASTER flag will be set.

## The V4L2 async interface and the concept of graph binding 367
- data-enable-active: Similar to HSYNC and VSYNC, specifies the data-enable
signal polarity.
- bus-width: This property concerns parallel buses only and represents the
number of data lines actively used. The V4L2_MBUS_DATA_ENABLE_HIGH or

## V4L2_MBUS_DATA_ENABLE_LOW flags are set accordingly.
- data-shift: On parallel data buses where bus-width is used to specify the
number of data lines, this property can be used to specify which data lines are really
used; for example, bus-width=<8>; data-shift=<2>; means that lines 9:2
are used.
- sync-on-green-active: The active state of the Sync-on-Green (SoG) signal;
0/1 for LOW/HIGH, respectively. The V4L2_MBUS_VIDEO_SOG_ACTIVE_HIGH or

## V4L2_MBUS_VIDEO_SOG_ACTIVE_LOW flags are set accordingly.

## The type of these buses is either V4L2_MBUS_PARALLEL or V4L2_MBUS_BT656. The
underlying function responsible for parsing these buses is v4l2_fwnode_endpoint_
parse_parallel_bus().

## MIPI CSI-2 bus
This is version 2 of MIPI Alliance's CSI bus. This bus involves two PHYs: either D- PHY
or C-PHY. D-PHY has been around for a while and targets cameras, displays, and
lower-speed applications. C-PHY is a newer and more complex PHY, where a clock is
embedded into the data, rendering a separate clock lane unnecessary. It has fewer wires,
a smaller number of lanes, and lower power consumption, and can achieve a higher
data rate compared to D-PHY. C-PHY provides high throughput performance over
bandwidth-limited channels.

## Both C-PHY- and D-PHY-enabled buses are represented using one data structure,
```c
struct v4l2_fwnode_bus_mipi_csi2, as follows:
struct v4l2_fwnode_bus_mipi_csi2 {
```
unsigned int flags;
unsigned char data_lanes[V4L2_FWNODE_CSI2_MAX_DATA_LANES];
unsigned char clock_lane;
unsigned short num_data_lanes;
bool lane_polarities[1 + V4L2_FWNODE_CSI2_MAX_DATA_LANES];
```c
};
```
368 Integrating with V4L2 Async and Media Controller Frameworks
In the preceding block, flags represents the flags of the bus and will be set according to
the properties present in the firmware node:
- data-lanes is an array of physical data lane indexes.
- lane-polarities: This property is valid for serial busses only. It is an array of
polarities of the lanes, starting from the clock lane and followed by the data lanes,
in the same order as in the data-lanes property. Valid values are 0 (normal)
and 1 (inverted). The length of this array should be the combined length of the
data-lanes and clock-lanes properties. Valid values are 0 (normal) and
1 (inverted). If the lane-polarities property is omitted, the value must be
interpreted as 0 (normal).
- clock-lanes is the physical lane index of the clock lane. This is the clock lane
position.
- clock-noncontinuous: If present, the V4L2_MBUS_CSI2_

## NONCONTINUOUS_CLOCK flag is set. Otherwise, V4L2_MBUS_CSI2_

## CONTINUOUS_CLOCK is set.

## These buses have the V4L2_MBUS_CSI2 type. Until Linux kernel v4.20, there were
no differences between C-PHY- and D-PHY-enabled CSI buses. However as of Linux
kernel v5.0, this difference has been introduced and V4L2_MBUS_CSI2 has been
replaced with either V4L2_MBUS_CSI2_DPHY or V4L2_MBUS_CSI2_CPHY,
respectively, for D-PHY- or C-PHY-enabled buses.

## The underlying function responsible for parsing these buses is v4l2_fwnode_
endpoint_parse_csi2_bus(). An example is as follows:
[...]
port {
tc358743_out: endpoint {
remote-endpoint = <&mipi_csi2_in>;
clock-lanes = <0>;
data-lanes = <1 2 3 4>;
lane-polarities = <1 1 1 1 1>;
clock-noncontinuous;
```c
};
};
```
370 Integrating with V4L2 Async and Media Controller Frameworks

## V4L2 async
Because of the complexity of video-based hardware that sometimes integrates non-V4L2
devices (sub-devices, actually) sitting on different buses, the need has come for
sub-devices to defer initialization until the bridge driver has been loaded, and on the
other hand, the bridge driver needs to postpone initializing sub-devices until all required
sub-devices have been loaded; that is, V4L2 async.
In asynchronous mode, sub-device probing can be invoked independently of bridge driver
availability. The sub-device driver then has to verify whether all the requirements for
a successful probing are satisfied. This can include a check for master clock availability,
a GPIO, or anything else. If any of the conditions aren't satisfied, the sub-device driver
might decide to return -EPROBE_DEFER to request further re-probing attempts. Once
all the conditions are met, the sub-device will be registered with the V4L2 async core
using the v4l2_async_register_subdev() function. The unregistration is
performed using the v4l2_async_unregister_subdev() call.
We saw earlier where synchronous registration applies. It is a mode where the bridge
driver is aware of the context of all the sub-devices it is responsible for. It has the
responsibility of registering all the sub-devices using v4l2_device_register_
subdev() on each of them during its probing, as is the case with the drivers/media/
platform/exynos4-is/media-dev.c driver.
In the V4L2 async framework, the concept of a sub-device is abstracted. A sub-device
is known in the async framework as an instance of a struct v4l2_async_subdev
structure. Along with this structure, there is another struct v4l2_async_notifier
structure. Both are defined in include/media/v4l2-async.h and somehow form
the center part of the V4L2 async core. Prior to going further, we have to introduce the
center part of the V4L2 async framework, struct v4l2_async_notifier,
as follows:
```c
struct v4l2_async_notifier {
```
const struct v4l2_async_notifier_operations *ops;
unsigned int num_subdevs;
unsigned int max_subdevs;
```c
struct v4l2_async_subdev **subdevs;
struct v4l2_device *v4l2_dev;
struct v4l2_subdev *sd;
struct v4l2_async_notifier *parent;
struct list_head waiting;
struct list_head done;

```
## The V4L2 async interface and the concept of graph binding 371
```c
struct list_head list;
};
```
The preceding structure is mostly used by the bridge drivers and the async core. In some
cases, however, sub-device drivers may need to be notified by some other sub-devices.
In either case, the uses and meanings of the members are the same:
- ops is a set of callbacks to be provided by the owner of this notifier that are invoked
by the async core as and when sub-devices waiting in this notifier are probed.
- v4l2_dev is the V4L2 parent of the bridge driver that registered this notifier.
- sd, if this notifier has been registered by a sub-device, will point to this sub-device.

## We do not address this case here.
- subdevs is an array of sub-devices for which the registrar of this notifier
(either the bridge driver or another sub-device driver) should be notified.
- waiting is a list of the sub-devices in this notifier waiting to be probed.
- done is a list of the sub-devices actually bound to this notifier.
- num_subdevs is the number of sub-devices in **subdevs.
- list is used by the async core during the registration of this notifier in order to
link this notifier to the global list of notifiers, notifier_list.

## Back to our struct v4l2_async_subdev structure, which is defined as follows:
```c
struct v4l2_async_subdev {
enum v4l2_async_match_type match_type;
```
union {
```c
struct fwnode_handle *fwnode;
```
const char *device_name;
```c
struct {
int adapter_id;
```
unsigned short address;
```c
} i2c;
struct {
```
bool (*match)(struct device *,
```c
struct v4l2_async_subdev *);
void *priv;
} custom;
} match;
```
372 Integrating with V4L2 Async and Media Controller Frameworks
```c
/* v4l2-async core private: not to be used by drivers */
struct list_head list;
};
```
The preceding data structure is a sub-device in the eyes of the V4L2 async framework.
Only the bridge driver (which allocates the async sub-device) and the async core can play
with this structure. The sub-device driver is not aware of this at all. The meanings of its
members are as follows:
- match_type is of the enum v4l2_async_match_type type. A match is
a comparison of some criteria (occurring strictly between a sub-device of the
```c
struct v4l2_subdev type and an async sub-device of the struct v4l2_
```
async_subdev type). Since each struct v4l2_async_subdev structure
must be associated with its struct v4l2_subdev structure, this field specifies
the algorithm used by the async core to match both. This field is set by the driver
(which is also responsible for allocating asynchronous sub-devices). Possible values
are as follows:
--V4L2_ASYNC_MATCH_DEVNAME, which instructs the async core to use
the device name for the matching. In this case, the bridge driver must set the
```c
v4l2_async_subdev.match.device_name field so that it can match the
```
sub-device device name (that is, dev_name(v4l2_subdev->dev)) when that
sub-device will be probed.
--V4L2_ASYNC_MATCH_FWNODE, which means the async core should use the
firmware node for the match. In this case, the bridge driver must set v4l2_
async_subdev.match.fwnode with the firmware node handle corresponding
to the sub-device's device node so that they can match.
--V4L2_ASYNC_MATCH_I2C is to be used to perform the match by checking
for the I2C adapter ID and address. Using this, the bridge driver must set both
```c
v4l2_async_subdev.match.i2c.adapter_id and v4l2_async_
```
subdev.match.i2c.address. These values will be compared with the address
and the adapter number of the i2c_client object associated with v4l2_
subdev.dev.
--V4L2_ASYNC_MATCH_CUSTOM is the last possibility and means the async
core should use the matching callback set by the bridge driver in v4l2_async_
subdev.match.custom.match. If this flag is set and there is no custom
matching callback provided, any matching attempt will immediately return true.
- list is used to add this async sub-device waiting to be probed in the waiting list
of a notifier.

## The V4L2 async interface and the concept of graph binding 373
Sub-device registration does not depend on the bridge availability anymore and only
consists of calling the v4l2_async_unregister_subdev() method. However, prior
to registering itself, the bridge driver will have to do the following:
1. Allocate a notifier for later use. It is better to embed this notifier in a larger
device state data structure. This notifier object is of the struct v4l2_async_
notifier type.
2. Parse its port node(s) and create an async sub-device (struct v4l2_async_
subdev) for each sensor (or IP block) specified there and that it needs for its
operations:
a) This parsing is done using the fwnode graph API (old drivers still use the of_
graph API), such as the following:
--fwnode_graph_get_next_endpoint() (or of_graph_get_next_
endpoint() in old drivers) to grab the fw_handle (or the of_node in old
drivers) of an endpoint from within the bridge's port subnode.
--fwnode_graph_get_remote_port_parent() (or of_graph_get_
remote_port_parent() in old drivers) to grab the fw_handle (or the device's
of_node in old drivers) corresponding to the parent of the remote port of the
current endpoint.
Optionally (in old drivers using the OF API), of_fwnode_handle() is used in
order to convert the of_node grabbed in the previous state into an fw_handle.
b) Set up the current async sub-device according to the matching logic that should
be used. It should set the v4l2_async_subdev.match_type and v4l2_
async_subdev.match members.
c) Add this async sub-device to the list of async sub-devices of the notifier. As
of version 4.20 of the kernel, there is a helper, v4l2_async_notifier_add_
subdev(), allowing you to do this.
3. Register the notifier object (this notifier will be stored in the global notifier_
list list defined in drivers/media/v4l2-core/v4l2-async.c) using
the v4l2_async_notifier_register(&big_struct->v4l2_dev,
&big_struct->notifier) call. To unregister the notifier, the driver has to call
```c
v4l2_async_notifier_unregister(&big_struct->notifier).
```
374 Integrating with V4L2 Async and Media Controller Frameworks
When the bridge driver invokes v4l2_async_notifier_register(), the async
core iterates over async sub-devices in the notifier->subdevs array. For each
async sub-device inside, the core checks whether this asd->match_type value is

## V4L2_ASYNC_MATCH_FWNODE. If applicable, the async core makes sure asd is not
present in the notifier->waiting list or in the notifier->done list by comparing
fwnodes. This provides assurance that asd was not already set up for fwnode and it
does not already exist in the given notifier. If asd is not already known, it is added to
notifier->waiting. After this, the async core will test all async sub-devices in the
notifier->waiting list for a match with all sub-devices present in subdev_list,
which is the list of "kind-of " orphan sub-devices, those that were registered prior to their
bridge driver (thus prior to their notifier). The async core uses the asd->match value
of each current asd for this. If a match occurs (the asd->match callback returns true),
the current async sub-device (from notifier->waiting) and the current sub-device
(from subdev_list) will be bound, the async sub-device will be removed from the
notifier->waiting list, the sub-device will be registered with the V4L2 core using
```c
v4l2_device_register_subdev(), and the sub-device will be moved from the
```
global subdev_list list to the notifier->done list.
Finally, the actual notifier being registered will be added to the global list of notifiers,
notifier_list, so that it can be used later for matching attempts whenever a new
sub-device is registered with the async core.

## Important note

## What the async core does when the sub-device driver invokes v4l2_async_
register_subdev() can be guessed from the preceding matching and
bounding logic descriptions. Effectively, upon this call, the async core will
attempt to match the current sub-device with all the async sub-devices waiting
in each notifier present in the notifier_list global list. If no match
occurs, it means this sub-device's bridge has not been probed yet, and the
sub-device is added to the global list of sub-devices, subdev_list.
If a match occurs, the sub-device will not be added to this list at all.
Do also keep in mind that a match test is a comparison of some criteria,
occurring strictly between a sub-device of the struct v4l2_subdev
type and an async sub-device of the struct v4l2_async_subdev type.

## The V4L2 async interface and the concept of graph binding 375
In the preceding paaragraphs, we said the async sub-device and the sub-device are bound.
But what does this mean? Here is where the notifier->ops member comes into
the picture. It is of the struct v4l2_async_notifier_operations type and is
defined as follows:
```c
struct v4l2_async_notifier_operations {
int (*bound)(struct v4l2_async_notifier *notifier,
struct v4l2_subdev *subdev,
struct v4l2_async_subdev *asd);
int (*complete)(struct v4l2_async_notifier *notifier);
void (*unbind)(struct v4l2_async_notifier *notifier,
struct v4l2_subdev *subdev,
struct v4l2_async_subdev *asd);
};
```
The following are the meanings of each callback in this structure despite the fact that all
three callbacks are optional:
- bound: If set, this callback will be invoked by the async core in response to a
successful sub-device probing by its (sub-device) driver. This also implies that an
async sub-device has successfully matched this sub-device. This callback takes as an
argument the notifier that originated the match, as well as the sub-device (subdev)
and the async sub-device (asd) that matched. Most drivers simply print debug
messages here. However, you can perform additional setup on the sub-device
here – that is, v4l2_subdev_call(). If everything seems OK, it should return
a positive value; otherwise, the sub-device is unregistered.
- unbind is invoked when a sub-device is removed from the system. In addition to
printing debug messages here, the bridge driver must unregister the video device
if the unbound sub-device was a requirement for it to work normally – that is,
video_unregister_device().
- complete is invoked when there are no more async sub-devices waiting in the
notifier. The async core can detect when the notifier->waiting list is empty
(which would mean sub-devices have been probed successfully and are all moved
into the notifier->done list). The complete callback is only executed for the
root notifier. Sub-devices that registered notifiers will not have their .complete
callback invoked. The root notifier is usually the one registered by the bridge device.
376 Integrating with V4L2 Async and Media Controller Frameworks
There is no doubt, then, that, prior to registering the notifier object, the bridge driver must
set the notifier's ops member. The most important callback for us is .complete.
While you can call v4l2_device_register() from within the bridge driver's probe
function, it is a common practice to register the actual video device from within the
notifier.complete callback, as all sub-devices would be registered, and the presence
of /dev/videoX would mean it is really usable. The .complete callback is also
suitable for both registering the actual video device's subnode and registering the media
device by means of v4l2_device_register_subdev_nodes() and
```c
media_device_register().
```
Note that v4l2_device_register_subdev_nodes() will create a device
node (/dev/v4l2-subdevX, actually) for every subdev object marked with the

## V4L2_SUBDEV_FL_HAS_DEVNODE flag.

## Async bridge and sub-device probing example
We will go through this section with a simple use case. Consider the following config:
- One bridge device (our CSI controller) – let's say the omap ISP, with foo as
its name.
- One off-chip sub-device, the camera sensor, with bar as its name.

## Both are connected this way: CSI <-- Camera Sensor.

## In the bar driver, we could register an async sub-device as follows:
```c
static int bar_probe(struct device *dev)
{
int ret;
```
ret = v4l2_async_register_subdev(subdev);
```c
if (ret) {
```
dev_err(dev, "ouch\n");
```c
return -ENODEV;
}
return 0;
}

```
## The V4L2 async interface and the concept of graph binding 377

## The probe function of the foo driver could be as follows:
```c
/* struct foo_device */
struct foo_device {
struct media_device mdev;
struct v4l2_device v4l2_dev;
struct video_device *vdev;
struct v4l2_async_notifier notifier;
struct *subdevs[FOO_MAX_SUBDEVS];
};
/* foo_probe() */
static int foo_probe(struct device *dev)
{
struct foo_device *foo = kmalloc(sizeof(*foo));
media_device_init(&bar->mdev);
```
foo->dev = dev;
foo->notifier.subdevs = kcalloc(FOO_MAX_SUBDEVS,
sizeof(struct v4l2_async_subdev));
foo_parse_nodes(foo);
foo->notifier.bound = foo_bound;
foo->notifier.complete = foo_complete;
return
```c
v4l2_async_notifier_register(&foo->v4l2_dev,
```
&foo->notifier);
```c
}
```
In the following code, we implement the foo fwnode (or of_node) parser helper,
foo_parse_nodes():
```c
struct foo_async {
struct v4l2_async_subdev asd;
struct v4l2_subdev *sd;
};
/* Either */
static void foo_parse_nodes(struct device *dev,
struct v4l2_async_notifier *n)
```
378 Integrating with V4L2 Async and Media Controller Frameworks
```c
{
struct device_node *node = NULL;
while ((node = of_graph_get_next_endpoint(dev->of_node,
```
node))) {
```c
struct foo_async *fa = kmalloc(sizeof(*fa));
```
n->subdevs[n->num_subdevs++] = &fa->asd;
fa->asd.match.of.node =
of_graph_get_remote_port_parent(node);
fa->asd.match_type = V4L2_ASYNC_MATCH_OF;
```c
}
}
/* Or */
static void foo_parse_nodes(struct device *dev,
struct v4l2_async_notifier *n)
{
struct fwnode_handle *fwnode = dev_fwnode(dev);
struct fwnode_handle *ep = NULL;
while ((ep = fwnode_graph_get_next_endpoint(ep, fwnode))) {
struct foo_async *fa = kmalloc(sizeof(*fa));
```
n->subdevs[n->num_subdevs++] = &fa->asd;
fa->asd.match.fwnode =
fwnode_graph_get_remote_port_parent(ep);
fa->asd.match_type = V4L2_ASYNC_MATCH_FWNODE;
```c
}
}
```
In the preceding code, both of_graph_get_next_endpoint() and
fwnode_graph_get_next_endpoint() have been used in order to show
how to play with the two. That being said, you're better off using the fwnode version,
as it is much more generic.
In the meantime, we need to write foo's notifier operations, which could look as follows:
```c
/* foo_bound() and foo_complete() */
static int foo_bound(struct v4l2_async_notifier *n,
struct v4l2_subdev *sd,
struct v4l2_async_subdev *asd)
{
struct foo_async *fa = container_of(asd, struct bar_async,

```
## The V4L2 async interface and the concept of graph binding 379
asd);
```c
/* One can use subdev_call here */
```
[...]
fa->sd = sd;
```c
}
static int foo_complete(struct v4l2_async_notifier *n)
{
struct foo_device *foo =
```
container_of(n, struct foo_async, notifier);
```c
struct v4l2_device *v4l2_dev = &isp->v4l2_dev;
/* Create /dev/sub-devX if applicable */
v4l2_device_register_subdev_nodes(&foo->v4l2_dev);
/* setup the video device: fops, queue, ioctls ... */
```
[...]
```c
/* Register the video device */
```
ret = video_register_device(foo->vdev,
VFL_TYPE_GRABBER, -1);
```c
/* Register with the media controller framework */
return media_device_register(&bar->mdev);
}
```
In the device tree, the V4L2 bridge device can be declared as follows:
csi1: csi@1cb4000 {
compatible = "allwinner,sun8i-v3s-csi";
reg = <0x01cb4000 0x1000>;
interrupts = <GIC_SPI 84 IRQ_TYPE_LEVEL_HIGH>;
```c
/* we omit clock and others */
```
[...]
port {
csi1_ep: endpoint {
remote-endpoint = <&ov7740_ep>;
380 Integrating with V4L2 Async and Media Controller Frameworks
```c
/* We omit v4l2 related properties */
```
[...]
```c
};
};
};
```
The camera node from within the I2C controller node can be declared as follows:
&i2c1 {
#address-cells = <1>;
#size-cells = <0>;
ov7740: camera@21 {
compatible = "ovti,ov7740";
reg = <0x21>;
```c
/* We omit clock or pincontrol or everything else */
```
[...]
port {
ov7740_ep: endpoint {
remote-endpoint = <&csi1_ep>;
```c
/* We omit v4l2 related properties */
```
[...]
```c
};
};
};
};
```
Now we are familiar with the V4L2 async framework and we have seen how the
asynchronous sub-device registration eases both the probe and the code. We ended
with a concrete example that highlights each aspect we have discussed. Now we can
move forward and integrate with the media controller framework, which is the last
improvement we can add to our V4L2 drivers.

## The Linux media controller framework
Media devices turn out to be very complex, involving several IP blocks of the SoC and
thus requiring video stream (re)routing.

## The Linux media controller framework 381
Now, let's consider a case where we have a much more sophisticated SoC made of two
more on-chip sub-devices – let's say a resizer and an image converter, called baz and biz.
In the previous example in the V4L2 async section, the setup was made up of one bridge
device and one sub-device (the fact that it is off-chip does not matter), the camera sensor.
This was quite straightforward. Luckily, things worked. But what if now we have to route
the stream through the image converter or the image resizer, or even through both IPs?
Or, say we have to switch from one to the other (dynamically)?
We could achieve this either via sysfs or ioctls, but this would have the following
problems:
- It would be too ugly (no doubt) and probably buggy.
- It would be too hard (a lot of work).
- It would be deeply SoC vendor-dependent, with possibly a lot of code duplication,
no unified user space API and ABI, and no consistency between drivers.
- It would be not a very credible solution.
Many SoCs can reroute internal video streams – for example, capturing them from
a sensor and doing memory-to-memory resizing, or sending the sensor output directly
to the resizer. Since the V4L2 API did not support these advanced devices, SoC
manufacturers made their own custom drivers. However, V4L2 is undisputably the Linux
API for capturing images and is sometimes used for specific display devices (these are
mem2mem devices).
It is becoming clear that we need another subsystem and framework that covers the limits
of V4L2. This is how the Linux media controller framework was born.

## The media controller abstraction model
Discovering a device's internal topology and configuring it at runtime is one of the goals
of the media framework. To achieve this, it comes with a layer of abstraction. With the
media controller framework, hardware devices are represented through an oriented graph
made of entities whose pads are connected via links. This set of elements put together
forms the so-called media device. A source pad can only generate data.
382 Integrating with V4L2 Async and Media Controller Frameworks
The preceding short description deserves some attention. There are three highlighted
words that are of high interest: entity, pad, and link:
- Entities are represented by a struct media_entity instance, defined in
include/media/media-entity.h. The structure is usually embedded into
a higher-level structure, such as a v4l2_subdev or video_device instance,
although drivers can allocate entities directly.
- Pads are the entity's interface to the outside world. These are input- and
output-connectable points of a media entity. However, a pad can be either an input
(sink pad) or an output (source pad), not both. Data streams from one entity's
source pad to another entity's sink pad. Typically, a device such as a sensor or
a video decoder would have only an output pad since it only feeds video into the
system, and a /dev/videoX pad would be modeled as an input pad since it is the
end of the stream.
- Links: These links can be set, fetched, and enumerated through the media device.
The application, for a driver to properly work, is responsible for setting up the links
properly so that the driver understands the source and destination of the video data.
All the entities on the system, along with their pads and the connection links between
them, give the media device shown in the following diagram:

## Figure 8.1 – Media controller abstraction model
In the preceding diagram, Stream would be the equivalent of a /dev/videoX char
device as it is the end of the stream.

## The Linux media controller framework 383

## V4L2 device abstraction

## At a higher level, the media controller uses struct media_device to abstract
```c
struct v4l2_device in the V4L2 framework. That being said, struct media_
```
device is to the media controller what struct v4l2_device is to V4L2, englobing
other lower-level structures. Back to struct v4l2_device, the mdev member is used
by the media controller framework to abstract this structure. The following is an excerpt:
```c
struct v4l2_device {
```
[...]
```c
struct media_device *mdev;
```
[...]
```c
};
```
However, from a media controller point of view, V4L2 video devices and sub-devices
are all seen as media entities, represented in this framework as instances of struct
```c
media_entity. It is then obvious for the video device and sub-device data structures
```
to embed a member of this type, as shown in the following excerpt:
```c
struct video_device
{
```
#if defined(CONFIG_MEDIA_CONTROLLER)
```c
struct media_entity entity;
struct media_intf_devnode *intf_devnode;
struct media_pipeline pipe;
```
#endif
[...]
```c
};
struct v4l2_subdev {
```
#if defined(CONFIG_MEDIA_CONTROLLER)
```c
struct media_entity entity;
```
#endif
[...]
```c
};
```
384 Integrating with V4L2 Async and Media Controller Frameworks
The video device has additional members, intf_devnode and pipe. The former, of the
```c
struct media_intf_devnode type, represents the media controller interface to the
```
video device node. This structure gives the media controller access to information of the
underlying video device node, such as its major and minor numbers. The other additional
member, pipe, which is of the struct media_pipeline type, stores information
related to the streaming pipeline of this video device.

## Media controller data structures
The media controller framework is based on a few data structures, among which is the
```c
struct media_device structure, which is on top of the hierarchy and defined as
```
follows:
```c
struct media_device {
/* dev->driver_data points to this struct. */
struct device *dev;
struct media_devnode *devnode;
```
char model[32];
char driver_name[32];
[...]
char serial[40];
u32 hw_revision;
u64 topology_version;
```c
struct list_head entities;
struct list_head pads;
struct list_head links;
struct list_head entity_notify;
struct mutex graph_mutex;
```
[...]
const struct media_device_ops *ops;
```c
};

```
## The Linux media controller framework 385
This structure represents a high-level media device. It allows easy access to entities and
provides basic media device-level support:
- dev is the parent device for this media device (usually a &pci_dev, &usb_
interface, or &platform_device instance).
- devnode is the media device node, abstracting the underlying /dev/mediaX.
- driver_name is an optional but recommended field, representing the media
device driver name. If not set, it defaults to dev->driver->name.
- model is the model name of this media device. It doesn't have to be unique.
- serial is an optional member that should be set with the device serial number.
hw_revision is the hardware device revision for this media device.
- topology_version: Monotonic counter for storing the version of the graph
topology. Should be incremented each time the topology changes.
- entities is the list of registered entities.
- pads is the list of pads registered with this media device.
- links is the list of links registered with this media device.
- entity_notify is the notify callback list invoked when a new entity is
registered with this media device. Drivers may register this callback to take action
via media_device_unregister_entity_notify() and unregister it
using media_device_register_entity_notify(). All the registered
```c
media_entity_notify callbacks are invoked when a new entity is registered.
```
- graph_mutex: Protects access to struct media_device data. It should, for
example, be held when using media_graph_* family functions.
- ops is of the struct media_device_ops type and represents the operation
handler callbacks for this media device.
386 Integrating with V4L2 Async and Media Controller Frameworks

## In addition to being manipulated by the media controller framework, struct
```c
media_device is essentially used in the bridge driver, where it is initialized and
```
registered. That being said, the media device on its own is made up of several entities. This
concept of entities allows the media controller to be the central authority when it comes to
modern and complex V4L2 drivers that may also support framebuffers, ALSA, I2C, LIRC,
and/or DVB devices at the same time and is used to inform user space of what is what.
A media entity is represented as an instance of struct media_entity, defined in
include/media/media-entity.h as follows:
```c
struct media_entity {
struct media_gobj graph_obj;
```
const char *name;
```c
enum media_entity_type obj_type;
```
u32 function;
unsigned long flags;
u16 num_pads;
u16 num_links;
u16 num_backlinks;
```c
int internal_idx;
struct media_pad *pads;
struct list_head links;
```
const struct media_entity_operations *ops;
```c
int stream_count;
int use_count;
struct media_pipeline *pipe;
```
[...]
```c
};

```
## The Linux media controller framework 387
This is the second data structure in the media framework in terms of hierarchy. The
preceding definition has been shrunk to the minimum that we are interested in. The
following are the meanings of the members in this structure:
- name is the name of this entity. It should be meaningful enough as it is used as it is
in user space with the media-ctl tool.
- type is most of the time set by the core depending on the type of V4L2 video data
structure this struct is embedded in. It is the type of the object that implements
```c
media_entity – for example, set with MEDIA_ENTITY_TYPE_V4L2_SUBDEV
```
at the sub-device initialization by the core. This allows runtime type identification of
media entities and safe casting to the correct object type using the container_of
macro, for instance. Possible values are as follows:
--MEDIA_ENTITY_TYPE_BASE: This means the entity is not embedded in
another.
--MEDIA_ENTITY_TYPE_VIDEO_DEVICE: This indicates the entity is embedded
in a struct video_device instance.
--MEDIA_ENTITY_TYPE_V4L2_SUBDEV: This means the entity is embedded in
a struct v4l2_subdev instance.
- function represents the entity's main function. This must be set by the driver
according to the value defined in include/uapi/linux/media.h. The
following are commonly used values while dealing with video devices:
--MEDIA_ENT_F_IO_V4L: This flag means the entity is a data streaming input
and/or output entity.
--MEDIA_ENT_F_CAM_SENSOR: This flag means this entity is a camera video
sensor entity.
--MEDIA_ENT_F_PROC_VIDEO_SCALER: Means this entity can perform video
scaling. These entities have at least one sink pad, from which they receive frame(s)
(on the active one) and one source pad where they output the scaled frame(s).
388 Integrating with V4L2 Async and Media Controller Frameworks
--MEDIA_ENT_F_PROC_VIDEO_ENCODER: Means this entity is capable of
compressing video. These entities must have one sink pad and at least one
source pad.
--MEDIA_ENT_F_VID_MUX: This is to be used for a video multiplexer. This entity
```c
has at least two sink pads and one source pad and must pass the video frame(s)
```
received from the active sink pad to the source pad.
--MEDIA_ENT_F_VID_IF_BRIDGE: Video interface bridge. A video interface
bridge entity should have at least one sink pad and one source pad. It receives video
frames on its sink pad from an input video bus of one type (HDMI, eDP, MIPI
CSI-2, and so on) and outputs them on its source pad to an output video bus of
another type (eDP, MIPI CSI-2, parallel, and so on).
- flags is set by the driver. It represents the flags for this entity. Possible values are
the MEDIA_ENT_FL_* flag family defined in include/uapi/linux/media.h.
The following link may be of help to you to understand the possible values:
https://linuxtv.org/downloads/v4l-dvb-apis/userspace-api/
mediactl/media-types.html.
- function represents this entity's function and by default is MEDIA_ENT_F_

## V4L2_SUBDEV_UNKNOWN. Possible values are the MEDIA_ENT_F_* function
family defined in include/uapi/linux/media.h. For example, a camera
sensor sub-device driver must contain sd->entity.function = MEDIA_

## ENT_F_CAM_SENSOR;. You can follow this link to find detailed information
on what may be suitable for your media entity: https://linuxtv.org/
downloads/v4l-dvb-apis/uapi/mediactl/media-types.html.
- num_pads is the total number of pads of this entity (sink and source).
- num_links is the total number of links of this entity (forward, back, enabled,
and disabled)
- num_backlinks is the numbers of backlinks of this entity. Backlinks are used to
help graph traversal and are not reported to user space.
- internal_idx: A unique entity number assigned by the media controller core
when the entity is registered.
- pads is the array of pads of this entity. Its size is defined by num_pads.
- links is the list of data links of this entity. See media_add_link().
- ops is of the media_entity_operations type and represents operations for
this entity. This structure will be discussed later.
- stream_count: Stream count for the entity.

## The Linux media controller framework 389
- use_count: The use count for the entity. Used for power management purposes.
- pipe is the media pipeline that this entity belongs to.
Naturally, the next data structure that seems obvious for us to introduce is the struct
```c
media_pad structure, which represents a pad in this framework. A pad is a connection
```
endpoint through which an entity can interact with other entities. Data (not restricted to
video) produced by an entity flows from the entity's output to one or more entity inputs.
Pads should not be confused with the physical pins at chip boundaries. struct
```c
media_pad is defined as follows:
struct media_pad {
```
[...]
```c
struct media_entity *entity;
```
u16 index;
unsigned long flags;
```c
};
```
Pads are identified by their entity and their 0-based index in the entity's pads array. In
the flags field, either MEDIA_PAD_FL_SINK (which indicates that the pad supports
sinking data) or MEDIA_PAD_FL_SOURCE (which indicates that the pad supports
sourcing data) can be set, but not both at the same time, since a pad can't both sink
and source.
Pads are meant to be bound together to allow data flow paths. Two pads, either from the
same entity or from different entities, are bound together by means of point-to-pointoriented connections called links. Links are represented in the media framework as
instances of struct media_link, defined as follows:
```c
struct media_link {
struct media_gobj graph_obj;
struct list_head list;
```
[...]
```c
struct media_pad *source;
struct media_pad *sink;
```
[...]
```c
struct media_link *reverse;
```
unsigned long flags;
bool is_backlink;
```c
};
```
390 Integrating with V4L2 Async and Media Controller Frameworks
In the preceding code block, only a few fields have been listed for the sake of readability.

## The following are the meanings of those fields:
- list: Used to associate this link with the entity or interface owning the link.
- source: Where this link originates from.
- sink: The link target.
- flags: Represents the link flags, as defined in uapi/media.h (with the
MEDIA_LNK_FL_* pattern). The following are the possible values:
--MEDIA_LNK_FL_ENABLED: This flag means the link is enabled and is ready for
data transfer.
--MEDIA_LNK_FL_IMMUTABLE: This flag means the link enabled state can't be
modified at runtime.
--MEDIA_LNK_FL_DYNAMIC: This flag means the state of the link can be
modified during streaming. However, this flag is set by drivers but is read-only for
applications.
- reverse: Pointer to the link (the backlink, actually) for the reverse direction of
a pad-to-pad link.
- is_backlink: Tells whether this link is a backlink.
Each entity has a list that points to all links originating at or targeting any of its pads.
A given link is thus stored twice, once in the source entity and once in the target entity.
When you want to link A to B, two links are actually created:
- One that corresponds to what was expected; the link is stored in the source entity,
and the source entity's num_links field is incremented.
- Another one is stored in the sink entity. The sink and source remain the same,
with the difference being that the is_backlink member is set to true.
This corresponds to the reverse of the link you created. The sink entity's num_
backlinks and num_links fields will be incremented. This backlink is then
assigned to the original link's reverse member.

## The Linux media controller framework 391

## At the end, the mdev->topology_version member is incremented twice. This
principle of link and backlink allows the media controller to numerate entities, along with
the possible and current links between entities, such as in the following diagram:

## Figure 8.2 – Media controller entity description
In the preceding diagram, if we consider Entity-1 and Entity-2, then link and backlink
are essentially the same, except that link belongs to Entity-1 and backlink belongs to
Entity-2. You should then consider the backlink as a backup link. We can see that an
entity can be either a sink, a source, or both.
The data structures we have introduced so far may make the media controller framework
sound a bit scary. However, most of those data structures will be managed under the
hood by the framework by means of the APIs it offers. That being said, the complete
framework's documentation can be found in Documentation/media-framework.
txt in the kernel sources.

## Integrating media controller support in the driver
When the support of the media controller is needed, the V4L2 driver must first initialize
```c
struct media_device within struct v4l2_device using the media_
```
device_init() function. Each entity driver must initialize its entities (actually
video_device->entity or v4l2_subdev->entity) and its pad arrays using the
```c
media_entity_pads_init() function and, if needed, create pad-to-pad links using
media_create_pad_link(). After that, entities can be registered. However, the
```
V4L2 framework will handle this registration for you through either the v4l2_device_
register_subdev() or the video_register_device() methods. In both cases,
the underlying registration function that is invoked is media_device_register_
entity().
392 Integrating with V4L2 Async and Media Controller Frameworks
As a final step, the media device has to be registered using media_device_
register(). It's worth mentioning that the media device registration should be
postponed to later in the future when we are sure that every sub-device (or should I say
entities) is registered and ready to be used. It definitely makes sense registering the media
device in the root notifier's .complete callback.

## Initializing and registering pads and entities
The same function is used to initialize both the entity and its pad array:
```c
int media_entity_pads_init(struct media_entity *entity,
```
u16 num_pads, struct media_pad *pads);
In the preceding prototype, *entity is the entity to which the pads to be registered
belong, *pads is the array of pads to be registered, and num_pads is the number of
entities in the array that should be registered. The driver must have set the type of every
pad in the pads array before calling:
```c
struct mydrv_state_struct {
struct v4l2_subdev sd;
struct media_pad pad;
```
[...]
```c
};
static int my_probe(struct i2c_client *client,
```
const struct i2c_device_id *id)
```c
{
struct v4l2_subdev *sd;
struct mydrv_state_struct *my_struct;
```
[...]
sd = &my_struct->sd;
my_struct->pad.flags = MEDIA_PAD_FL_SINK |
MEDIA_PAD_FL_MUST_CONNECT;
ret = media_entity_pads_init(&sd->entity, 1,
&my_struct->pad);
[...]
```c
return 0;
}

```
## The Linux media controller framework 393
Drivers that need to unregister entities must call the following function on the entity to be
unregistered:
```c
media_device_unregister_entity(struct media_entity *entity);
```
Then, in order for a driver to free resources associated with an entity, it should call the
following:
```c
media_entity_cleanup(struct media_entity *entity);
```
When a media device is unregistered, all of its entities are unregistered automatically.

## No unregistration of manual entities is then required.

## Media entity operations
An entity may be provided link-related callbacks, so that these can be invoked by the
media framework upon link creation and validation:
```c
struct media_entity_operations {
int (*get_fwnode_pad)(struct fwnode_endpoint *endpoint);
int (*link_setup)(struct media_entity *entity,
```
const struct media_pad *local,
const struct media_pad *remote,
u32 flags);
```c
int (*link_validate)(struct media_link *link);
};
```
Providing the preceding structure is optional. However, there may be situations where
additional stuff needs to be done or checked either at link setup or link validation. In this
case, note the following descriptions:
- get_fwnode_pad: Returns the pad number based on a fwnode endpoint or
a negative value on error. This operation can be used to map a fwnode to a media
pad number (optional).
- link_setup: Notifies the entity of link changes. This operation can return an
error, in which case the link setup will be canceled (optional).
394 Integrating with V4L2 Async and Media Controller Frameworks
- link_validate: Returns whether a link is valid from the entity point of view.
The media_pipeline_start() function validates all the links this entity is
involved in by calling this operation. This member is optional. However, if it has not
been set, then v4l2_subdev_link_validate_default will be used as the
default callback function, which ensures that the source pad and sink pad width,
height, and media bus pixels code are consistent; otherwise, it will return an error.

## The concept of a media bus
The main purpose of the media framework is to configure and control the pipeline and
its entities. Video sub-devices, such as cameras and decoders, connect to video bridges
or other sub-devices over specialized buses. Data is being transferred over these buses in
various formats. That being said, in order for two entities to actually exchange data, their
pad configs need to be the same.
Applications are responsible for configuring coherent parameters on the whole pipeline
and ensuring that connected pads have compatible formats. The pipeline is checked for
formats that are mismatching at VIDIOC_STREAMON time.
The driver is responsible for applying the configuration of every block in the video
pipeline according to the requested (from the user) format at the pipeline input
and/or output.
Take the following simple data flow, sensor ---> CPHY ---> csi ---> isp
---> stream.
In order for the media framework to be able to configure the bus prior to streaming data,
the driver needs to provide some pad-level setter and getter for the media bus properties,
which are present in the struct v4l2_subdev_pad_ops structure. This structure
implements pad-level operations that have to be defined if the sub-device driver intends to
process the video and integrate with the media framework. The following is its definition:
```c
struct v4l2_subdev_pad_ops {
```
[...]
```c
int (*enum_mbus_code)(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_mbus_code_enum *code);
int (*enum_frame_size)(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_frame_size_enum *fse);
int (*enum_frame_interval)(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,

```
## The Linux media controller framework 395
```c
struct v4l2_subdev_frame_interval_enum *fie);
int (*get_fmt)(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_format *format);
int (*set_fmt)(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_format *format);
```
#ifdef CONFIG_MEDIA_CONTROLLER
```c
int (*link_validate)(struct v4l2_subdev *sd,
struct media_link *link,
struct v4l2_subdev_format *source_fmt,
struct v4l2_subdev_format *sink_fmt);
```
#endif /* CONFIG_MEDIA_CONTROLLER */
[...]
```c
};

```
## The following are the meanings of the members in this structure:
- init_cfg: Initializes the pad config to default values. This is the right place to
initialize cfg->try_fmt, which can be grabbed through v4l2_subdev_get_
try_format().
- enum_mbus_code: Callback for the VIDIOC_SUBDEV_ENUM_MBUS_CODE
ioctl handler code. Enumerates the currently supported data format. This callback
handles pixel format enumeration.
- enum_frame_size: Callback for the VIDIOC_SUBDEV_ENUM_FRAME_SIZE
ioctl handler code. Enumerates the frame (image) size supported by the sub-device.

## Enumerates the currently supported resolution.
- enum_frame_interval: Callback for the VIDIOC_SUBDEV_ENUM_FRAME_

## INTERVAL ioctl handler code.
- get_fmt: Callback for the VIDIOC_SUBDEV_G_FMT ioctl handler code.
- set_fmt: Callback for the VIDIOC_SUBDEV_S_FMT ioctl handler code. Sets the
output data format and resolution.
- get_selection: Callback for the VIDIOC_SUBDEV_G_SELECTION ioctl
handler code.
396 Integrating with V4L2 Async and Media Controller Frameworks
- set_selection: Callback for the VIDIOC_SUBDEV_S_SELECTION ioctl
handler code.
- link_validate: Used by the media controller code to check whether the links
that belong to a pipeline can be used for the stream.
The argument that all of these callbacks have in common is cfg, which is of the struct
```c
v4l2_subdev_pad_config type and is used for storing sub-device pad information.

```
## This structure is defined in include/uapi/linux/v4l2-mediabus.h as follows:
```c
struct v4l2_subdev_pad_config {
struct v4l2_mbus_framefmt try_fmt;
struct v4l2_rect try_crop;
```
[...]
```c
};
```
In the preceding code block, the main field we are interested in is try_fmt, which is of
the struct v4l2_mbus_framefmt type. This data structure is used to describe the
pad-level media bus format and is defined as follows:
```c
struct v4l2_subdev_format {
```
__u32 which;
__u32 pad;
```c
struct v4l2_mbus_framefmt format;
```
[...]
```c
};
```
In the preceding structure, which is the format type (try or active) and pad is the pad
number as reported by the media API. This field is set by user space. format represents
the frame format on the bus. The format term here means a combination of the media
bus data format, frame width, and frame height. It is of the struct v4l2_mbus_
framefmt type and its turn is defined as follows:
```c
struct v4l2_mbus_framefmt {
```
__u32 width;
__u32 height;
__u32 code;
__u32 field;
__u32 colorspace;
[...]
```c
};

```
## The Linux media controller framework 397
In the preceding bus frame format data structure, only the fields that are relevant to
us have been listed. width and height, respectively, represent the image width and
height. code is from enum v4l2_mbus_pixelcode and represents the data
format code. field indicates the used interlacing type, which should be from enum
```c
v4l2_field, and colorspace represents the color space of the data from enum
v4l2_colorspace.
```
Now, let's pay more attention to the get_fmt and set_fmt callbacks. They get and
set, respectively, the data format on a sub-device pad. These ioctl handlers are used to
negotiate the frame format at specific sub-device pads in the image pipeline. To set the
current format applications, set the .pad field of struct v4l2_subdev_format to
the desired pad number as reported by the media API and the which field (which is from
```c
enum v4l2_subdev_format_whence) to either V4L2_SUBDEV_FORMAT_TRY or

```
## V4L2_SUBDEV_FORMAT_ACTIVE, and issue a VIDIOC_SUBDEV_S_FMT ioctl with a
pointer to this structure. This ioctl ends up calling the v4l2_subdev_pad_ops->set_
fmt callback. If which is set to V4L2_SUBDEV_FORMAT_TRY, then the driver should
set the .try_fmt field of the requested pad config with the values of the try format
given in the argument. However, if which is set to V4L2_SUBDEV_FORMAT_ACTIVE,
the driver must then apply the config to the device. It is common in this case to store the
requested "active" format in a driver-state structure and apply it to the underlying device
when the pipeline starts the stream. This way, the right place to actually apply the format
config to the device is from within a callback invoked at the start of the streaming, such
as v4l2_subdev_video_ops.s_stream, for example. The following is an example
from the RCAR CSI driver:
```c
static int rcsi2_set_pad_format(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_format *format)
{
struct v4l2_mbus_framefmt *framefmt;
/* retrieve the private data structure */
struct rcar_csi2 *priv = sd_to_csi2(sd);
```
[...]
```c
/* Store the requested format so that it can be applied to
```
* the device when the pipeline starts
```c
*/
if (format->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
```
priv->mf = format->format;
398 Integrating with V4L2 Async and Media Controller Frameworks
```c
} else { /* V4L2_SUBDEV_FORMAT_TRY */
/* set the .try_fmt of this pad config with the
```
* value of the requested "try" format
```c
*/
```
framefmt = v4l2_subdev_get_try_format(sd, cfg, 0);
*framefmt = format->format;
```c
/* driver is free to update any format->* field */
```
[...]
```c
}
return 0;
}
```
Also, note that the driver is free to change the values in the requested format to the one
it actually supports. It is then up to the application to check for it and adapt its logic
according to the format granted by the driver. Modifying those try formats leaves the
device state untouched.
On the other hand, when it comes to retrieving the current format, applications should
do the same as the preceding and issue a VIDIOC_SUBDEV_G_FMT ioctl. This ioctl will
end up calling the v4l2_subdev_pad_ops->get_fmt callback. The driver fills the
members of the format field either with the currently active format values or with the
last try format stored (most of the time in the driver-state structure):
```c
static int rcsi2_get_pad_format(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_format *format)
{
struct rcar_csi2 *priv = sd_to_csi2(sd);
if (format->which == V4L2_SUBDEV_FORMAT_ACTIVE)
```
format->format = priv->mf;
else
format->format = *v4l2_subdev_get_try_format(sd, cfg, 0);
```c
return 0;
}

```
## The Linux media controller framework 399
It is obvious that the .try_fmt field of the pad config should have been initialized before
it can be passed to the get callback for the first time, and the v4l2_subdev_pad_ops.
init_cfg callback is the right place for this initialization, as in the following example:
```c
/*
```
* Initializes the TRY format to the ACTIVE format on all pads
* of a subdev. Can be used as the .init_cfg pad operation.
```c
*/
int imx_media_init_cfg(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg)
{
struct v4l2_mbus_framefmt *mf_try;
struct v4l2_subdev_format format;
```
unsigned int pad;
```c
int ret;
for (pad = 0; pad < sd->entity.num_pads; pad++) {
```
memset(&format, 0, sizeof(format));
format.pad = pad;
format.which = V4L2_SUBDEV_FORMAT_ACTIVE;
ret = v4l2_subdev_call(sd, pad, get_fmt, NULL, &format);
```c
if (ret)
```
continue;
mf_try = v4l2_subdev_get_try_format(sd, cfg, pad);
*mf_try = format.format;
```c
}
return 0;
}
```
400 Integrating with V4L2 Async and Media Controller Frameworks

## Important note

## The list of supported formats can be found in include/uapi/linux/
videodev2.h from the kernel source, and part of their documentation is
available at this link: https://linuxtv.org/downloads/v4ldvb-apis/userspace-api/v4l/subdev-formats.html.
Now that we are familiar with the concept of media, we can learn how to finally make the
media device part of the system by using the appropriate API to register it.

## Registering the media device
```c
Drivers register media device instances by calling __media_device_register()
```
via the media_device_register() macro and unregister them by calling
```c
media_device_unregister(). Upon successful registration, a character device
```
named media[0-9] + will be created. The device major and minor numbers are
dynamic. media_device_register() accepts a pointer to the media device to be
registered and returns 0 on success or a negative error code on error.
As we said earlier, you're better off registering the media device from within the root
notifier's .complete callback in order to make sure that the actual media device is
registered only after all its entities have been probed. The following is an excerpt from
the TI OMAP3 ISP media driver (the whole code can be found in drivers/media/
platform/omap3isp/isp.c in the kernel sources):
```c
static int isp_subdev_notifier_complete(
struct v4l2_async_notifier *async)
{
struct isp_device *isp =
```
container_of(async, struct isp_device, notifier);
[...]
```c
return media_device_register(&isp->media_dev);
}
static const
struct v4l2_async_notifier_operations isp_subdev_notifier_ops =
{
```
.complete = isp_subdev_notifier_complete,
```c
};

```
## The Linux media controller framework 401
The preceding code shows how you can take benefit of the root notifier's .complete
callback to register the final media device, by means of the media_device_
register() method.
Now that the media device is part of the system, the time has come to leverage it,
particularly from user space. Let's now see how, from the command line, we can take
control of and interact with the media device.

## Media controller from user space
Though it remains the streaming interface, /dev/video0 is not the default pipeline
centerpiece anymore since it is wrapped by /dev/mediaX. The pipeline can be
configured through the media node (/dev/media*), and the control operations, such
as stream on/off, can be performed through the video node (/dev/video*).
Using media-ctl (the v4l-utils package)
The media-ctl application from the v4l-utils package is a user space application
that uses the Linux media controller API to configure pipelines. The following are the flags
to use with it:
- --device <dev> specifies the media device (/dev/media0 by default).
- --entity <name> prints the device name associated with the given entity.
- --set-v4l2 <v4l2> provides a comma-separated list of formats to set up.
- --get-v4l2 <pad> prints an active format on a given pad.
- --set-dv <pad> configures DV timings on a given pad.
- --interactive modifies links interactively.
- --links <linux> provides a comma-separated list of link descriptors to set up.
- --known-mbus-fmts lists known formats and their numeric values.
- --print-topology prints the device topology, or the short version, -p.
- --reset resets all links to inactive.
That being said, the basic configuration steps for a hardware media pipeline are as follows:
1. Reset all links with media-ctl --reset.
2. Configure links with media-ctl --links.
402 Integrating with V4L2 Async and Media Controller Frameworks
3. Configure pad formats with media-ctl --set-v4l2.
4. Configure sub-device properties with v4l2-ctl capture frames on the /dev/
video* device.
Using media-ctl --links to link an entity source pad to an entity sink pad should
follow the following pattern:
media-ctl --links\
"<entitya>:<srcpadn> -> <entityb>:<sinkpadn>[<flags>]
In the preceding line, flags can be either 0 (inactive) or 1 (active). Additionally, to see
the current settings of the media bus, use the following:
$ media-ctl --print-topology
On some systems, media device 0 may not be the default one, in which case you should
use the following:
$ media-ctl --device /dev/mediaN --print-topology
The previous command would print the media topology associated with the specified
media device.
Do note that --print-topology just dumps the media topology on the console in an
ASCII format. However, this topology can be better represented by generating its dot
representation, changing this representation into a graphic image that is more humanfriendly. The following are the commands to use:
$ media-ctl --print-dot > graph.dot
$ dot -Tpng graph.dot > graph.png
For example, in order to set up a media pipe, the following commands have been run on
an UDOO QUAD board. The board has been shipped with an i.MX6 quad core and an

## OV5640 camera plugged into the MIPI CSI-2 connector:
# media-ctl -l "'ov5640 2-003c':0 -> 'imx6-mipi-csi2':0[1]"
# media-ctl -l "'imx6-mipi-csi2':2 -> 'ipu1_csi1':0[1]"
# media-ctl -l "'ipu1_csi1':1 -> 'ipu1_ic_prp':0[1]"
# media-ctl -l "'ipu1_ic_prp':1 -> 'ipu1_ic_prpenc':0[1]"
# media-ctl -l "'ipu1_ic_prpenc':1 -> 'ipu1_ic_prpenc
capture':0[1]"

## The Linux media controller framework 403

## The following is a diagram representing the preceding setup:

## Figure 8.3 – Graph representation of a media device
As you can see, it helps to visualize what the hardware components are. The following are
descriptions of these generated images:
- Dashed lines show possible connections. You can use these to determine the
possibilities.
- Solid lines show active connections.
404 Integrating with V4L2 Async and Media Controller Frameworks
- Green boxes show media entities.
- Yellow boxes show Video4Linux (V4L) endpoints.
After that, you can see that solid lines correspond exactly to the setup that was done
earlier. We have five solid lines, which correspond to the number of commands used to
configure the media device. The following are the meanings of these commands:
- media-ctl -l "'ov5640 2-003c':0 -> 'imx6-mipi-csi2':0[1]"
```c
means linking output pad number 0 of the camera sensor ('ov5640 2-003c':0)
```
to MIPI CSI-2 input pad number 0 ('imx6-mipi-csi2':0) and setting this link
active ([1]).
- media-ctl -l "'imx6-mipi-csi2':2 -> 'ipu1_csi1':0[1]" means
linking output pad number 2 of the MIPI CSI-2 entity ('imx6-mipi-csi2':2)
to the input pad number 0 of the IPU capture sensor interface #1 (' ipu1_
csi1':0) and setting this link active ([1]).
- The same decoding rules apply to other command lines, until the last one,
media-ctl -l "'ipu1_ic_prpenc':1 -> 'ipu1_ic_prpenc
capture':0[1]", which means linking output pad number 1 of ipu1's image
converter preprocessing encode entity ('ipu1_ic_prpenc':1) to the capture
interface input pad number 0 and setting this link to active.
Do not hesitate to go back to the image and read those descriptions several times in order
to understand the concepts of entity, link, and pad.

## Important note
If the dot package is not installed on your target, you can download the .dot
file on your host (assuming it has the package installed) and convert it into an
image.

## WaRP7 with an OV2680 example
The WaRP7 is an i.MX7-based board, which, unlike the i.MX5/6 family, does not contain
an IPU. Because of this, there are fewer capabilities to perform operations or manipulation
of the capture frames. The i.MX7 image capture chain is made up of three units: the
camera censor interface, the video multiplexer, and the MIPI CSI-2 receiver, which
represent the media entities, described as follows:
- imx7-mipi-csi2: This is the MIPI CSI-2 receiver entity. It has one sink pad to
receive the pixel data from the MIPI CSI-2 camera sensor. It has one source pad,
corresponding to virtual channel 0.

## The Linux media controller framework 405
- csi_mux: This is the video multiplexer. It has two sink pads to select from either
camera sensors with a parallel interface or MIPI CSI-2 virtual channel 0. It has
a single source pad that routes to the CSI.
- csi: The CSI allows the chip to connect directly to the external CMOS image
sensor. The CSI can interface directly with parallel and MIPI CSI-2 buses. It has 256
x 64 FIFO to store received image pixel data and embedded DMA controllers to
transfer data from the FIFO through the AHB bus. This entity has one sink pad that
receives from the csi_mux entity and a single source pad that routes video frames
directly to memory buffers. This pad is routed to a capture device node:
|\

## MIPI Camera Input --> MIPI CSI-2 -- > | \
| \
| M |
| U | --> CSI --> Capture
| X |
| /

## Parallel Camera Input --------------> | /
|/
On this platform, an OV2680 MIPI CSI-2 module is connected to the internal MIPI CSI-2
receiver. The following example configures a video capture pipeline with an output of 800
x 600 in BGGR 10-bit Bayer format:
# Setup links
media-ctl --reset
media-ctl -l "'ov2680 1-0036':0 -> 'imx7-mipi-csis.0':0[1]"
media-ctl -l "'imx7-mipi-csis.0':1 -> 'csi_mux':1[1]"
media-ctl -l "'csi_mux':2 -> 'csi':0[1]"
media-ctl -l "'csi':1 -> 'csi capture':0[1]"

## The preceding lines could be merged into one single command, as follows:
media-ctl -r -l '"ov2680 1-0036":0->"imx7-mipi-csis.0":0[1], \
"imx7-mipi-csis.0":1 ->"csi_mux":1[1], \
"csi_mux":2->"csi":0[1], \
"csi":1->"csi capture":0[1]'
406 Integrating with V4L2 Async and Media Controller Frameworks

## In the preceding commands, note the following:
- -r means reset all links to inactive.
- -l sets up links in a comma-separated list of the links' descriptors.
- "ov2680 1-0036":0->"imx7-mipi-csis.0":0[1] links output pad
number 0 of the camera sensor to MIPI CSI-2 input pad number 0 and sets this
link to active.
- "csi_mux":2->"csi":0[1] links output pad number 2 of csi_mux to csi
input pad number 0 and sets this link to active.
- "csi":1->"csi capture":0[1] links output pad number 1 of csi to
capture the interface's input pad number 0 and sets this link to active.
In order to configure the format on each pad, we can use the following commands:
# Configure pads for pipeline
media-ctl -V "'ov2680 1-0036':0 [fmt:SBGGR10_1X10/800x600
field:none]"
media-ctl -V "'csi_mux':1 [fmt:SBGGR10_1X10/800x600
field:none]"
media-ctl -V "'csi_mux':2 [fmt:SBGGR10_1X10/800x600
field:none]"
media-ctl \
-V "'imx7-mipi-csis.0':0 [fmt:SBGGR10_1X10/800x600
field:none]"
media-ctl -V "'csi':0 [fmt:SBGGR10_1X10/800x600 field:none]"
Once again, the preceding command lines could be merged into a single command,
as follows:
media-ctl \
-f '"ov2680 1-0036":0 [SGRBG10 800x600 (32,20)/800x600], \
"csi_mux":1 [SGRBG10 800x600], \
"csi_mux":2 [SGRBG10 800x600], \
"mx7-mipi-csis.0":2 [SGRBG10 800x600], \
"imx7-mipi-csi.0":0 [SGRBG10 800x600], \
"csi":0 [UYVY 800x600]'

## The Linux media controller framework 407

## The preceding command lines could be translated as follows:
- -f: Sets up pad formats into a comma-separated list of format descriptors.
- "ov2680 1-0036":0 [SGRBG10 800x600 (32,20)/800x600]: Sets up
the camera sensor pad number 0 format to a RAW Bayer 10-bit image with
a resolution (capture size) of 800 x 600. Sets the maximum allowed sensor window
width by specifying the crop rectangle.
- "csi_mux":1 [SGRBG10 800x600]: Sets up the csi_mux pad number 1
format to a RAW Bayer 10-bit image with a resolution of 800 x 600.
- "csi_mux":2 [SGRBG10 800x600]: Sets up the csi_mux pad number 2
format to a RAW Bayer 10-bit image with a resolution of 800 x 600.
- "csi":0 [UYVY 800x600]: Sets up the csi pad number 0 format to a

## YUV4:2:2 image with a resolution of 800 x 600.
video_mux, csi, and mipi-csi-2 are all part of the SoC, so they are declared in the
vendor dtsi file (that is, arch/arm/boot/dts/imx7s.dtsi in the kernel sources).
video_mux is declared as follows:
gpr: iomuxc-gpr@30340000 {
[...]
video_mux: csi-mux {
compatible = "video-mux";
mux-controls = <&mux 0>;
#address-cells = <1>;
#size-cells = <0>;
status = "disabled";
port@0 {
reg = <0>;
```c
};
```
port@1 {
reg = <1>;
csi_mux_from_mipi_vc0: endpoint {
remote-endpoint = <&mipi_vc0_to_csi_mux>;
```c
};
};
```
port@2 {
408 Integrating with V4L2 Async and Media Controller Frameworks
reg = <2>;
csi_mux_to_csi: endpoint {
remote-endpoint = <&csi_from_csi_mux>;
```c
};
};
};
};
```
In the preceding code block, we have three ports, where ports 1 are 2 are connected to
remote endpoints. csi and mipi-csi-2 are declared as follows:
mipi_csi: mipi-csi@30750000 {
compatible = "fsl,imx7-mipi-csi2";
[...]
status = "disabled";
port@0 {
reg = <0>;
```c
};
```
port@1 {
reg = <1>;
mipi_vc0_to_csi_mux: endpoint {
remote-endpoint = <&csi_mux_from_mipi_vc0>;
```c
};
};
};
```
[...]
csi: csi@30710000 {
compatible = "fsl,imx7-csi"; [...]
status = "disabled";
port {
csi_from_csi_mux: endpoint {
remote-endpoint = <&csi_mux_to_csi>;

## The Linux media controller framework 409
```c
};
};
};
```
From the csi and mipi-csi-2 nodes, we can see how they are linked to their remote
ports in the video_mux node.

## Important note

## More information on video_mux binding can be found in

## Documentation/devicetree/bindings/media/video-mux.
txt in the kernel sources.
However, most of the vendor-declared nodes are disabled by default, and need to be
enabled from within the board file (the dts file, actually). This is what is done in the
following code block. Moreover, the camera sensor is part of the board, not the SoC.
So, it needs to be declared in the board dts file, which is arch/arm/boot/dts/
imx7s-warp.dts in kernel sources. The following is an excerpt:
&video_mux {
status = "okay";
```c
};
```
&mipi_csi {
clock-frequency = <166000000>;
fsl,csis-hs-settle = <3>;
status = "okay";
port@0 {
reg = <0>;
mipi_from_sensor: endpoint {
remote-endpoint = <&ov2680_to_mipi>;
data-lanes = <1>;
```c
};
};
};
```
410 Integrating with V4L2 Async and Media Controller Frameworks
&i2c2 {
[...]
status = "okay";
ov2680: camera@36 {
compatible = "ovti,ov2680";
[...]
port {
ov2680_to_mipi: endpoint {
remote-endpoint = <&mipi_from_sensor>;
clock-lanes = <0>;
data-lanes = <1>;
```c
};
};
};

```
## Important note

## More on i.MX7 entity binding can be found in both Documentation/
devicetree/bindings/media/imx7-csi.txt and
Documentation/devicetree/bindings/media/imx7-mipicsi2.txt in the kernel sources.
After this, the streaming can start. The v4l2-ctl tool can be used to select any of the
resolutions supported by the sensor:
root@imx7s-warp:~# media-ctl -p

## Media controller API version 4.17.0

## Media device information
------------------------
driver imx7-csi
model imx-media
serial
bus info
hw revision 0x0

## The Linux media controller framework 411
driver version 4.17.0

## Device topology
- entity 1: csi (2 pads, 2 links)
type V4L2 subdev subtype Unknown flags 0
device node name /dev/v4l-subdev0
pad0: Sink
[fmt:SBGGR10_1X10/800x600 field:none]
<- "csi-mux":2 [ENABLED]
pad1: Source
[fmt:SBGGR10_1X10/800x600 field:none]
-> "csi capture":0 [ENABLED]
- entity 4: csi capture (1 pad, 1 link)
type Node subtype V4L flags 0
device node name /dev/video0
pad0: Sink
<- "csi":1 [ENABLED]
- entity 10: csi-mux (3 pads, 2 links)
type V4L2 subdev subtype Unknown flags 0
device node name /dev/v4l-subdev1
pad0: Sink
[fmt:unknown/0x0]
pad1: Sink
[fmt:unknown/800x600 field:none]
<- "imx7-mipi-csis.0":1 [ENABLED]
pad2: Source
[fmt:unknown/800x600 field:none]
-> "csi":0 [ENABLED]
- entity 14: imx7-mipi-csis.0 (2 pads, 2 links)
type V4L2 subdev subtype Unknown flags 0
device node name /dev/v4l-subdev2
pad0: Sink
[fmt:SBGGR10_1X10/800x600 field:none]
<- "ov2680 1-0036":0 [ENABLED]
pad1: Source
[fmt:SBGGR10_1X10/800x600 field:none]
-> "csi-mux":1 [ENABLED]
- entity 17: ov2680 1-0036 (1 pad, 1 link)
type V4L2 subdev subtype Sensor flags 0
device node name /dev/v4l-subdev3
pad0: Source
[fmt:SBGGR10_1X10/800x600 field:none]
-> "imx7-mipi-csis.0":0 [ENABLED]
As data streams from left to right, we can interpret the preceding console logs as follows:
- -> "imx7-mipi-csis.0":0 [ENABLED]: This source pad feeds data to the
entity on its right, which is "imx7-mipi-csis.0":0.
- <- "ov2680 1-0036":0 [ENABLED]: This sink pad is fed by (that is, it
queries data from) the entity to its left, which is "ov2680 1-0036":0.
We are now done with all the aspects of the media controller framework. We started with
its architecture, then described the data structure it is made of, and then learned about its
API in detail. We ended with its use from user space in order to leverage the mode media
pipe.
## Summary
In this chapter, we went through the V4L2 asynchronous interface, which eases video
bridge and sub-device driver probing. This is useful for intrinsically asynchronous and
unordered device registration systems, such as flattened device tree driver probing.
Moreover, we dealt with the media controller framework, which allows leveraging V4L2
video pipelines. What we have seen so far lies in the kernel space.
In the next chapter, we will see how to deal with V4L2 devices from user space, thus
leveraging features exposed by their device drivers350 Integrating with V4L2 Async and Media Controller Frameworks
This chapter will focus on how both the async and media controller frameworks work
and how they are designed, and we will go through their APIs to learn how to leverage
them in Video4Linux2 (V4L2) device driver development.

## In other words, in this chapter, we will cover the following topics:
- The V4L2 async interface and the concept of graph binding
- The V4L2 async and graph-oriented API
- The V4L2 async framework and APIs
- The Linux media controller framework
## Technical requirements

## In this chapter, you'll need the following elements:
- Advanced computer architecture knowledge and C programming skills
- Linux kernel v4.19.X sources, available at https://git.kernel.org/pub/
scm/linux/kernel/git/stable/linux.git/refs/tags

## The V4L2 async interface and the concept of
graph binding
So far, with V4L2 driver development, we have not actually dealt with the probing
order. That being said, we considered the synchronous approach, where bridge device
drivers register devices for all sub-devices synchronously during their probing. However,
this approach cannot be used with intrinsically asynchronous and unordered device
registration systems, such as the flattened device tree. To address this, what we currently
call the async interface has been introduced.
With this new approach, bridge drivers register lists of sub-device descriptors and notifier
callbacks, and sub-device drivers register sub-devices that they are about to probe or
have successfully probed. The async core will take care of matching sub-devices against
hardware descriptors and calling bridge driver callbacks when matches are found.
Another callback is called when the sub-device is unregistered. The async subsystem relies
on device declaration in a special way, called graph binding, which we will deal with in
the next section.

## The V4L2 async interface and the concept of graph binding 351

## Graph binding
Embedded systems have a reduced set of devices, some of which are not discoverable.
The device tree, however, came into the picture to allow describing the actual system
(from a hardware point of view) to the kernel. Sometimes (if not always), these devices
are somehow interconnected.
While the phandle properties pointing to other nodes could be used in the device tree
to describe simple and direct connections, such as parent/child relationships, there was no
way to model compound devices made of several interconnections. There were situations
where the relationship modeling resulted in a quite complete graph – for example, the
i.MX6 Image Processing Unit (IPU), which is a logical device on its own, but made up of
several physical IP blocks whose interconnections may result in a quite complex pipe.
This is where the so-called Open Firmware (OF) graph intervenes, along with its API and
some new concepts, the concepts of port and endpoint:
- A port can be seen as an interface in a device (as in an IP block).
- An endpoint can be seen as a pad, as it describes one end of a connection to a
remote port.
However, phandle properties are still used to refer to other nodes in the tree. More
documentation on this can be found in Documentation/devicetree/bindings/
graph.txt.

## Port and endpoint representations
A port is an interface to a device. A device can have one or several ports. Ports are
represented by port nodes contained in the node of the device they belong to. Each port
node contains an endpoint subnode for each remote device port to which this port is
connected. This means a single port can be connected to more than one port on the
remote device(s) and that each link must be represented by an endpoint child node. Now,
if a device node contains more than one port, if there is more than one endpoint at a port,
or a port node needs to be connected to a selected hardware interface, a popular scheme
using the #address-cells, #size-cells, and reg properties is used to number
the nodes.

## The following excerpt shows how to use the #address-cells, #size-cells, and
reg properties to handle those cases:
device {
...
#address-cells = <1>;
352 Integrating with V4L2 Async and Media Controller Frameworks
#size-cells = <0>;
port@0 {
#address-cells = <1>;
#size-cells = <0>;
reg = <0>;
endpoint@0 {
reg = <0>;
...
```c
};
```
endpoint@1 {
reg = <1>;
...
```c
};
};
```
port@1 {
reg = <1>;
endpoint { ... };
```c
};
};

```
## Complete documentation of this can be found in Documentation/devicetree/
bindings/graph.txt. Now that we are done with port and endpoint representation,
we need to learn how to link each with the other, as explained in the next section.

## Endpoint linking
For two endpoints to be linked together, each of them should contain a remoteendpoint phandle property that points to the corresponding endpoint in the port of
the remote device. In turn, the remote endpoint should contain a remote-endpoint
property. Two endpoints with their remote-endpoint phandles pointing at each other
form a link between the containing ports, as in the following example:
device-1 {
port {
device_1_output: endpoint {

## The V4L2 async interface and the concept of graph binding 353
remote-endpoint = <&device_2_input>;
```c
};
};
};
```
device-2 {
port {
device_2_input: endpoint {
remote-endpoint = <&device_1_output>;
```c
};
};
}
```
Introducing the graph binding concept without talking at all about its API would be
a waste of time. Let's jump to the API that comes along with this new binding method.

## The V4L2 async and graph-oriented API
This section heading must not mislead you since graph binding is not just intended for the
V4L2 subsystem. The Linux DRM subsystem also takes advantage of it. That being said, the
async framework heavily relies on the device tree to describe either media devices along
with their endpoints and connections, or links between those endpoints along with their
bus configuration properties.
From the DT (of_graph_*) API to the generic fwnode graph API
(fwnode_graph_*)
The fwnode graph API is a successful attempt at changing the device tree-only-based
OF graph API to a generic API, merging both ACPI and device tree OF APIs together in
order to have unified and generic APIs. This extends the concept of the graph with ACPI
by using the same APIs. By having a look at the struct device_node and struct
acpi_device structures, you can see the members they have in common: struct
fwnode_handle fwnode:
```c
struct device_node {
```
[...]
```c
struct fwnode_handle fwnode;
```
[...]
```c
};
```
354 Integrating with V4L2 Async and Media Controller Frameworks
The preceding excerpt represents a device node from a device tree point of view, while the
following is related to ACPI:
```c
struct acpi_device {
```
[...]
```c
struct fwnode_handle fwnode;
```
[...]
```c
};
```
The fwnode member, which is of the struct fwnode_handle type, is a lower level
and generic data structure abstracting either device_node or acpi_device as they
both inherit from this data structure. This makes struct fwnode_handle a good
client for graph API homogenization so that an endpoint (by means of its field of the
fwnode_handle type) can refer to either an ACPI device or an OF-based device. This
abstraction model is now used in graph APIs, allowing us to abstract an endpoint by
a generic data structure (struct fwnode_endpoint, described as follows) embedding
a pointer to struct fwnode_handle, which may refer to either an ACPI or OF node.
In addition to the genericity, this allows the underlying sub-device to this endpoint to be
either ACPI- or OF-based:
```c
struct fwnode_endpoint {
```
unsigned int port;
unsigned int id;
const struct fwnode_handle *local_fwnode;
```c
};
```
This structure deprecates the old struct of_endpoint structure and the member of
type device_node* leaves room for a member of the fwnode_handle* type. In the
preceding structure, local_fwnode points to the related firmware node, port is the
port number (that is, it corresponds to 0 in port@0 or 1 in port@1), and id is the index
of this endpoint from within the port (that is, it corresponds to the 0 in endpoint@0
and to the 1 in endpoint@1).
The V4L2 framework uses this model for abstracting V4L2-related endpoints by means
of struct v4l2_fwnode_endpoint, which is built on top of fwnode_endpoint,
as follows:
```c
struct v4l2_fwnode_endpoint {
struct fwnode_endpoint base;
/*
```
* Fields below this line will be zeroed by

## The V4L2 async interface and the concept of graph binding 355
* v4l2_fwnode_endpoint_parse()
```c
*/
enum v4l2_mbus_type bus_type;
```
union {
```c
struct v4l2_fwnode_bus_parallel parallel;
struct v4l2_fwnode_bus_mipi_csi1 mipi_csi1;
struct v4l2_fwnode_bus_mipi_csi2 mipi_csi2;
} bus;
```
u64 *link_frequencies;
unsigned int nr_of_link_frequencies;
```c
};

```
## This structure deprecates and replaces struct v4l2_of_endpoint since kernel
v4.13, formerly used by V4L2 to represent endpoint nodes in the era of the V4L2 OF

## API. In the preceding data structure definition, base represents the struct
fwnode_endpoint structure of the underlying ACPI or device node. Other fields
are V4L2-related, as follows:
- bus_type is the type of media bus through which this sub-device streams data.

## The value of this member determines which underlying bus structure should be
filled with the parsed bus properties from the fwnode endpoint (either device tree
or ACPI). Possible values are listed in enum v4l2_mbus_type, as follows:
```c
enum v4l2_mbus_type {

```
## V4L2_MBUS_PARALLEL,

## V4L2_MBUS_BT656,

## V4L2_MBUS_CSI1,

## V4L2_MBUS_CCP2,

## V4L2_MBUS_CSI2,
```c
};
```
- bus is the structure representing the media bus itself. Possible values are already
present in the union, and bus_type determines which one to consider. These bus
structures are all defined in include/media/v4l2-fwnode.h.
- link_frequencies is the list of frequencies supported by this link.
- nr_of_link_frequencies is the number of elements in link_
frequencies.
356 Integrating with V4L2 Async and Media Controller Frameworks

## Important note

## In kernel v4.19, the bus_type member is exclusively set according to the
bus-type property in fwnode. The driver can check the read value and
adapt its behavior. This means the V4L2 fwnode API will always base its
parsing strategy on this fwnode property. However, as of kernel v5.0, drivers
have to set this member to an expected bus type (prior to calling the parsing
function), which will be compared to the value of the bus-type property
read in fwnode and will raise an error if they don't match. If the bus type is
not known or if the driver can deal with several bus types, the V4L2_MBUS_
UNKNOWN value has to be used. This value is also part of enum v4l2_
mbus_type, as of kernel v5.0.

## In the kernel code, you may find the enum v4l2_fwnode_bus_type
```c
enum type. This is a V4L2 fwnode local enum type that is the counterpart
```
of the global enum v4l2_mbus_type enum type and whose values map
each other. Their respective values are kept in sync as the code evolves.
The V4L2-related binding then requires additional properties. Part of these properties
is used to build v4l2_fwnode_endpoint, while the other part is used to build the
underlying bus (the media bus, actually) structure. All are described in a dedicated and
video-related binding documentation, Documentation/devicetree/bindings/
media/video- interfaces.txt, which I strongly recommend checking out.
The following is a typical binding between a bridge (isc) and a sensor sub-device
(mt9v032):
&i2c1 {
#address-cells = <1>;
#size-cells = <0>;
mt9v032@5c {
compatible = "aptina,mt9v032";
reg = <0x5c>;
port {
mt9v032_out: endpoint {
remote-endpoint = <&isc_0>;
link-frequencies =
/bits/ 64 <13000000 26600000 27000000>;
hsync-active = <1>;
vsync-active = <0>;
pclk-sample = <1>;

## The V4L2 async interface and the concept of graph binding 357
```c
};
};
};
};
```
&isc {
port {
isc_0: endpoint@0 {
remote-endpoint = <&mt9v032_out>;
hsync-active = <1>;
vsync-active = <0>;
pclk-sample = <1>;
```c
};
};
};

```
## In the preceding binding, hsync-active, vsync-active, link-frequencies,
and pclk- sample are all V4L2-specific properties and describe the media bus. Their
values are not coherent here and do not really make sense but fit well for our learning
purpose. This excerpt shows well the concepts of endpoint and remote endpoint; the use
of struct v4l2_fwnode_endpoint is discussed in detail in the The Linux media
controller framework section.

## Important note
The part of V4L2 dealing with the fwnode API is called the V4L2 fwnode
API. It is a replacement of the device tree-only API, the V4L2 OF API. The
former has a set of APIs prefixed with v4l2_fwnode_, while the second's
API set is prefixed with v4l2_of_. Do note that in OF-only-based APIs, an
endpoint is represented by struct of_endpoint, and a V4L2-related
endpoint is represented by struct v4l2_of_endpoint. There are APIs
that allow switching from OF- to fwnode-based models and vice versa.
V4L2 fwnode and V4L2 OF are fully interoperable. For example, a subdevice driver using V4L2 fwnode will work with a media device driver using
V4L2 OF without any effort, and vice versa! However, new drivers must use
the fwnode API, including #include <media/v4l2- fwnode.
h>, which should replace #include <media/v4l2-of.h> in the old
driver when switching to the fwnode API.
358 Integrating with V4L2 Async and Media Controller Frameworks
That being said, struct fwnode_endpoint, which was discussed earlier, is just for
showing the underlying mechanisms. We could have completely skipped it since only
the core deals with this data structure. For a more generic approach, instead of using
```c
struct device_node to refer to the device's firmware node, you're better off using the
```
new struct fwnode_handle. This definitely makes sure that DT and ACPI bindings
are compatible/interoperable using the same code in the driver. The following is a short
excerpt of how changes should look in new drivers:
- struct device_node *of_node;
+ struct fwnode_handle *fwnode;
- of_node = ddev->of_node;
+ fwnode = dev_fwnode(dev);

## Some of the common fwnode node-related APIs are as follows:
[...]
```c
struct fwnode_handle *fwnode_get_parent(
```
const struct fwnode_handle *fwnode);
```c
struct fwnode_handle *fwnode_get_next_child_node(
```
const struct fwnode_handle *fwnode,
```c
struct fwnode_handle *child);
struct fwnode_handle *fwnode_get_next_available_child_node(
```
const struct fwnode_handle *fwnode,
```c
struct fwnode_handle *child);
#define fwnode_for_each_child_node(fwnode, child) \
for (child = fwnode_get_next_child_node(fwnode, NULL);
```
child; \
child = fwnode_get_next_child_node(fwnode, child))
```c
#define fwnode_for_each_available_child_node(fwnode, child) \
for (child = fwnode_get_next_available_child_node(fwnode,
```
NULL);
child; \
child = fwnode_get_next_available_child_node(fwnode, child))

## The V4L2 async interface and the concept of graph binding 359
```c
struct fwnode_handle *fwnode_get_named_child_node(
```
const struct fwnode_handle *fwnode,
const char *childname);
```c
struct fwnode_handle *fwnode_handle_get(struct
```
fwnode_handle *fwnode);
```c
void fwnode_handle_put(struct fwnode_handle *fwnode);

```
## The aforementioned APIs have the following description:
- fwnode_get_parent() returns the parent handle of the node whose fwnode
value is given in an argument, or NULL otherwise.
- fwnode_get_next_child_node() takes a parent node as its first argument
and returns the next child (or NULL otherwise) after a given child (given as the
second argument) in this parent. If child (the second argument) is NULL, then the
first child of this parent will be returned.
- fwnode_get_next_available_child_node() is the same as fwnode_
get_next_child_node() but makes sure that the device actually exists (has
been probed successfully) prior to returning the fwnode handle.
- fwnode_for_each_child_node() iterates over the child in a given node (the
first argument) and the second argument is used as an iterator.
- fwnode_for_each_available_child_node is the same as fwnode_for_
each_child_node() but iterates only over nodes whose device is actually
present on the system.
- fwnode_get_named_child_node() gets a child in a given node by its name.
- fwnode_handle_get() obtains a reference to a device node and fwnode_
handle_put() drops this reference.

## Some of the fwnode-related properties are as follows:
[...]
bool fwnode_device_is_available(const
```c
struct fwnode_handle *fwnode);
```
bool fwnode_property_present(const
```c
struct fwnode_handle *fwnode,
```
const char *propname);
```c
int fwnode_property_read_string(const
```
360 Integrating with V4L2 Async and Media Controller Frameworks
```c
struct fwnode_handle *fwnode,
```
const char *propname,
const char **val);
```c
int fwnode_property_match_string(const
struct fwnode_handle *fwnode,
```
const char *propname,
const char *string);

## Both property- and node-related fwnode APIs are available in include/linux/
property.h. However, there are helpers that allow switching back and forth between

## OF, ACPI, and fwnode. The following is a short example:
```c
/* to switch from fwnode to of */
struct device_node *of_node = to_of_node(fwnode);
/* to switch from of to fw */
struct fwnode_handle *fwnode = of_fwnode_handle(node)
/* to switch from fwnode to acpi handle, the below macro has
```
* been introduced
*
* #define ACPI_HANDLE_FWNODE(fwnode) \
* acpi_device_handle(to_acpi_device_node(fwnode))
*
* and to switch from acpi device to fwnode:
*
* struct fwnode_handle *
* acpi_fwnode_handle(struct acpi_device *adev)
*
```c
*/

```
## The V4L2 async interface and the concept of graph binding 361
Finally, and most important for us, is the fwnode graph API. In the following code
snippet, we enumerate the most important function of this API:
```c
struct fwnode_handle
```
*fwnode_graph_get_next_endpoint(const
```c
struct fwnode_handle *fwnode,
struct fwnode_handle *prev);
struct fwnode_handle
```
*fwnode_graph_get_port_parent(const
```c
struct fwnode_handle *fwnode);
struct fwnode_handle
```
*fwnode_graph_get_remote_port_parent(
const struct fwnode_handle *fwnode);
```c
struct fwnode_handle
```
*fwnode_graph_get_remote_port(const
```c
struct fwnode_handle *fwnode);
struct fwnode_handle
```
*fwnode_graph_get_remote_endpoint(
const struct fwnode_handle *fwnode);
```c
#define fwnode_graph_for_each_endpoint(fwnode, child) \
for (child = NULL; \
```
(child = fwnode_graph_get_next_endpoint(fwnode, child)); )
```c
int fwnode_graph_parse_endpoint(const
struct fwnode_handle *fwnode,
struct fwnode_endpoint *endpoint);
```
[...]
362 Integrating with V4L2 Async and Media Controller Frameworks
Though the preceding function names talk about themselves, the following are better
descriptions of what they do:
- fwnode_graph_get_next_endpoint() returns the next endpoint (or NULL
otherwise) in a given node (the first argument) after a previous endpoint (prev,
the second argument). If prev is NULL, then the first endpoint is returned. This
function obtains a reference to the returned endpoint that must be dropped after
use. See fwnode_handle_put().
- fwnode_graph_get_port_parent() returns the parent of the port node
given in the argument.
- fwnode_graph_get_remote_port_parent() returns the firmware node
of the remote device containing the endpoint whose firmware node is given through
the fwnode argument.
- fwnode_graph_get_remote_endpoint() returns the firmware node of the
remote endpoint corresponding to a local endpoint whose firmware node is given
through the fwnode argument.
- fwnode_graph_parse_endpoint() parses common endpoint node properties
in fwnode (the first argument) representing a graph endpoint node and stores the
information in endpoint (the second and output argument). The V4L2 firmware
node API heavily uses this.
The V4L2 firmware node (V4L2 fwnode) API

## The main data structure in the V4L2 fwnode API is struct v4l2_fwnode_
endpoint. This structure is nothing but struct fwnode_handle augmented
with some V4L2-related properties. However, there is a V4L2-related fwnode graph
function that it is worth talking about here: v4l2_fwnode_endpoint_parse(). This
function's prototype is declared include/media/v4l2-fwnode.h, as follows:
```c
int v4l2_fwnode_endpoint_parse(struct fwnode_handle *fwnode,
struct v4l2_fwnode_endpoint *vep);

```
## The V4L2 async interface and the concept of graph binding 363
Given fwnode_handle (the first argument in the preceding function) of an endpoint,
you can use v4l2_fwnode_endpoint_parse() to parse all the fwnode node
properties. This function also recognizes and takes care of the V4L2-specific properties,
which are, if you remember, those documented in Documentation/devicetree/
bindings/media/video-interfaces.txt. v4l2_fwnode_endpoint_
parse() uses fwnode_graph_parse_endpoint() to parse common fwnode
properties and uses V4L2-specific parser helpers to parse V4L2-related properties. It
returns 0 on success or a negative error code on failure.
If we consider the mt9v032 CMOS image sensor node in dts, we can have the following
code in the probe method:
```c
int err;
struct fwnode_handle *ep;
struct v4l2_fwnode_endpoint bus_cfg;
/* We grab the fwnode corresponding to the device */
struct fwnode_handle *fwnode = dev_fwnode(dev);
/* We grab its endpoint(s) node */
```
ep = fwnode_graph_get_next_endpoint(fwnode, NULL);
```c
/* We parse the endpoint common properties as well as
```
* v4l2 related properties
```c
*/
```
err = v4l2_fwnode_endpoint_parse(ep, &bus_cfg);
```c
if (err) { /* handle error */ }
/* At this point we can access parameters such as bus_type,
```
* bus.flags
* (in case of mipi csi2 or parallel buses), V4L2_MBUS_*
* which are the
* media bus flags
```c
*/
/* we drop the reference on the enpoint */
```
fwnode_handle_put(ep);
364 Integrating with V4L2 Async and Media Controller Frameworks
The preceding code shows how you use the fwnode API, as well as its V4L2 version, for
accessing node and endpoint properties. There are, however, V4L2-specific properties
being parsed upon the v4l2_fwnode_endpoint_parse() call. These properties
describe the so-called media bus through which data is carried from one interface to
another. We will discuss this in the next section.

## V4L2 fwnode or media bus types
Most media devices support a particular media bus type. While endpoints are linked
together, they are actually connected through buses, whose properties need to be
described to the V4L2 framework. For V4L2 to be able to find this information, it
is provided as properties in the device's fwnode (DT or ACPI). As these are specific
properties, the V4L2 fwnode API is able to recognize and parse them. Each bus has its
specificities and properties.
First of all, let's have a look at the currently supported buses, along with their data
structures:
- MIPI CSI-1: This is MIPI Alliance's Camera Serial Interface (CSI) version 1. This
bus is represented as instances of struct v4l2_fwnode_bus_mipi_csi1.
- CCP2: This stands for Compact Camera Port 2, made by the Standard Mobile
Imaging Architecture (SMIA), which is an open standard for companies dealing
with camera modules for use in mobile applications (such as SMIA CCP2). This bus
is represented in this framework with instances of struct v4l2_fwnode_bus_
mipi_csi1 too.
- Parallel bus: This is the classic parallel interface, with HSYNC and VSYNC signals.

## The structure used to represent this bus is struct v4l2_fwnode_bus_
parallel.
- BT656: This is for BT.1120 or any parallel bus that transmits the conventional video
timing and synchronization signal (HSYNC, VSYNC, and BLANK) in the data. These
buses have a reduced number of pins compared to the standard parallel bus. This
framework uses struct v4l2_fwnode_bus_parallel to represent this bus.
- MIPI CSI-2: This is version 2 of MIPI Alliance's CSI interface. This bus is abstracted
by the struct v4l2_fwnode_bus_mipi_csi2 structure. However, this
data structure does not differentiate between D-PHY and C-PHY. This lack of
differentiation is addressed as of kernel v5.0.

## The V4L2 async interface and the concept of graph binding 365
As we will see later in the chapter, in the The concept of a media bus section, this concept
of a bus can be used to detect compatibility between a local endpoint and its remote
counterpart so that two sub-devices can't be linked together if they don't have the same
bus properties, which makes complete sense.

## Earlier, in the The V4L2 fwnode API section, we saw that v4l2_fwnode_endpoint_
parse() is responsible for parsing the endpoint's fwnode and filling the appropriate
bus structure. This function first calls fwnode_graph_parse_endpoint() in order
to parse the common fwnode graph-related properties, and then checks the value of the
bus-type property, as follows, in order to determine the appropriate v4l2_fwnode_
endpoint.bus data type:
u32 bus_type = 0;
fwnode_property_read_u32(fwnode, "bus-type", &bus_type);
Depending on this value, a bus data structure will be chosen. The following are expected
possible values from the fwnode device:
- 0: This means auto-detect. The core will try to guess the bus type according to the
properties present in the fwnode (MIPI CSI-2 D-PHY, parallel, or BT656).
- 1: This means MIPI CSI-2 C-PHY.
- 2: This means MIPI CSI-1.
- 3: This means CCP2.
For the CPP2 bus, for example, the device's fwnode would contain the following line:
bus-type = <3>;

## Important note
As of kernel v5.0, drivers can specify the expected bus type in the bus_type
member of v4l2_fwnode_endpoint prior to giving it as a second
argument to v4l2_fwnode_endpoint_parse(). This way, parsing
will fail if the value returned by the preceding fwnode_property_read_
u32 does not match the expected one, except if the expected bus type was set
to V4L2_MBUS_UNKNOWN.
366 Integrating with V4L2 Async and Media Controller Frameworks

## BT656 and parallel buses

## Those bus types are all represented by struct v4l2_fwnode_bus_parallel, as
follows:
```c
struct v4l2_fwnode_bus_parallel {
```
unsigned int flags;
unsigned char bus_width;
unsigned char data_shift;
```c
};
```
In the preceding data structure, flags represents the flags of the bus. Those flags will
be set according to the properties present in the device's firmware node. bus_width
represents the number of data lines actively used, not necessarily the total number of lines
of the bus. data_shift is used to specify which data lines are really used by specifying
the number of lines to skip prior to reaching the first active data line. The following
are the binding properties of these media buses, which are used to set up struct
```c
v4l2_fwnode_bus_parallel:
```
- hsync-active: Active state of the HSYNC signal; 0/1 for LOW/HIGH,
respectively. If this property's value is 0, then the V4L2_MBUS_HSYNC_ACTIVE_
LOW flag is set in the flags member. Any other value will set the V4L2_MBUS_

## HSYNC_ACTIVE_HIGH flag instead.
- vsync-active: Active state of the VSYNC signal; 0/1 for LOW/HIGH, respectively.

## If this property's value is 0, then the V4L2_MBUS_VSYNC_ACTIVE_LOW flag
is set in the flags member. Any other value will set the V4L2_MBUS_VSYNC_

## ACTIVE_HIGH flag instead.
- field-even-active: The field signal level during the even field data
transmission. This is the same as the preceding, but the concerned flags are

## V4L2_MBUS_FIELD_EVEN_HIGH and V4L2_MBUS_FIELD_EVEN_LOW.
- pclk-sample: Sample data on the rising (1) or falling (0) edge of the pixel clock
signal, V4L2_MBUS_PCLK_SAMPLE_RISING and V4L2_MBUS_PCLK_SAMPLE_

## FALLING.
- data-active: Similar to HSYNC and VSYNC, specifies data line polarity,

## V4L2_MBUS_DATA_ACTIVE_HIGH and V4L2_MBUS_DATA_ACTIVE_LOW.
- slave-mode: This is a Boolean property whose presence indicates that the link
is run in slave mode, and the V4L2_MBUS_SLAVE flag is set. Otherwise, the

## V4L2_MBUS_MASTER flag will be set.

## The V4L2 async interface and the concept of graph binding 367
- data-enable-active: Similar to HSYNC and VSYNC, specifies the data-enable
signal polarity.
- bus-width: This property concerns parallel buses only and represents the
number of data lines actively used. The V4L2_MBUS_DATA_ENABLE_HIGH or

## V4L2_MBUS_DATA_ENABLE_LOW flags are set accordingly.
- data-shift: On parallel data buses where bus-width is used to specify the
number of data lines, this property can be used to specify which data lines are really
used; for example, bus-width=<8>; data-shift=<2>; means that lines 9:2
are used.
- sync-on-green-active: The active state of the Sync-on-Green (SoG) signal;
0/1 for LOW/HIGH, respectively. The V4L2_MBUS_VIDEO_SOG_ACTIVE_HIGH or

## V4L2_MBUS_VIDEO_SOG_ACTIVE_LOW flags are set accordingly.

## The type of these buses is either V4L2_MBUS_PARALLEL or V4L2_MBUS_BT656. The
underlying function responsible for parsing these buses is v4l2_fwnode_endpoint_
parse_parallel_bus().

## MIPI CSI-2 bus
This is version 2 of MIPI Alliance's CSI bus. This bus involves two PHYs: either D- PHY
or C-PHY. D-PHY has been around for a while and targets cameras, displays, and
lower-speed applications. C-PHY is a newer and more complex PHY, where a clock is
embedded into the data, rendering a separate clock lane unnecessary. It has fewer wires,
a smaller number of lanes, and lower power consumption, and can achieve a higher
data rate compared to D-PHY. C-PHY provides high throughput performance over
bandwidth-limited channels.

## Both C-PHY- and D-PHY-enabled buses are represented using one data structure,
```c
struct v4l2_fwnode_bus_mipi_csi2, as follows:
struct v4l2_fwnode_bus_mipi_csi2 {
```
unsigned int flags;
unsigned char data_lanes[V4L2_FWNODE_CSI2_MAX_DATA_LANES];
unsigned char clock_lane;
unsigned short num_data_lanes;
bool lane_polarities[1 + V4L2_FWNODE_CSI2_MAX_DATA_LANES];
```c
};
```
368 Integrating with V4L2 Async and Media Controller Frameworks
In the preceding block, flags represents the flags of the bus and will be set according to
the properties present in the firmware node:
- data-lanes is an array of physical data lane indexes.
- lane-polarities: This property is valid for serial busses only. It is an array of
polarities of the lanes, starting from the clock lane and followed by the data lanes,
in the same order as in the data-lanes property. Valid values are 0 (normal)
and 1 (inverted). The length of this array should be the combined length of the
data-lanes and clock-lanes properties. Valid values are 0 (normal) and
1 (inverted). If the lane-polarities property is omitted, the value must be
interpreted as 0 (normal).
- clock-lanes is the physical lane index of the clock lane. This is the clock lane
position.
- clock-noncontinuous: If present, the V4L2_MBUS_CSI2_

## NONCONTINUOUS_CLOCK flag is set. Otherwise, V4L2_MBUS_CSI2_

## CONTINUOUS_CLOCK is set.

## These buses have the V4L2_MBUS_CSI2 type. Until Linux kernel v4.20, there were
no differences between C-PHY- and D-PHY-enabled CSI buses. However as of Linux
kernel v5.0, this difference has been introduced and V4L2_MBUS_CSI2 has been
replaced with either V4L2_MBUS_CSI2_DPHY or V4L2_MBUS_CSI2_CPHY,
respectively, for D-PHY- or C-PHY-enabled buses.

## The underlying function responsible for parsing these buses is v4l2_fwnode_
endpoint_parse_csi2_bus(). An example is as follows:
[...]
port {
tc358743_out: endpoint {
remote-endpoint = <&mipi_csi2_in>;
clock-lanes = <0>;
data-lanes = <1 2 3 4>;
lane-polarities = <1 1 1 1 1>;
clock-noncontinuous;
```c
};
};
```
370 Integrating with V4L2 Async and Media Controller Frameworks

## V4L2 async
Because of the complexity of video-based hardware that sometimes integrates non-V4L2
devices (sub-devices, actually) sitting on different buses, the need has come for
sub-devices to defer initialization until the bridge driver has been loaded, and on the
other hand, the bridge driver needs to postpone initializing sub-devices until all required
sub-devices have been loaded; that is, V4L2 async.
In asynchronous mode, sub-device probing can be invoked independently of bridge driver
availability. The sub-device driver then has to verify whether all the requirements for
a successful probing are satisfied. This can include a check for master clock availability,
a GPIO, or anything else. If any of the conditions aren't satisfied, the sub-device driver
might decide to return -EPROBE_DEFER to request further re-probing attempts. Once
all the conditions are met, the sub-device will be registered with the V4L2 async core
using the v4l2_async_register_subdev() function. The unregistration is
performed using the v4l2_async_unregister_subdev() call.
We saw earlier where synchronous registration applies. It is a mode where the bridge
driver is aware of the context of all the sub-devices it is responsible for. It has the
responsibility of registering all the sub-devices using v4l2_device_register_
subdev() on each of them during its probing, as is the case with the drivers/media/
platform/exynos4-is/media-dev.c driver.
In the V4L2 async framework, the concept of a sub-device is abstracted. A sub-device
is known in the async framework as an instance of a struct v4l2_async_subdev
structure. Along with this structure, there is another struct v4l2_async_notifier
structure. Both are defined in include/media/v4l2-async.h and somehow form
the center part of the V4L2 async core. Prior to going further, we have to introduce the
center part of the V4L2 async framework, struct v4l2_async_notifier,
as follows:
```c
struct v4l2_async_notifier {
```
const struct v4l2_async_notifier_operations *ops;
unsigned int num_subdevs;
unsigned int max_subdevs;
```c
struct v4l2_async_subdev **subdevs;
struct v4l2_device *v4l2_dev;
struct v4l2_subdev *sd;
struct v4l2_async_notifier *parent;
struct list_head waiting;
struct list_head done;

```
## The V4L2 async interface and the concept of graph binding 371
```c
struct list_head list;
};
```
The preceding structure is mostly used by the bridge drivers and the async core. In some
cases, however, sub-device drivers may need to be notified by some other sub-devices.
In either case, the uses and meanings of the members are the same:
- ops is a set of callbacks to be provided by the owner of this notifier that are invoked
by the async core as and when sub-devices waiting in this notifier are probed.
- v4l2_dev is the V4L2 parent of the bridge driver that registered this notifier.
- sd, if this notifier has been registered by a sub-device, will point to this sub-device.

## We do not address this case here.
- subdevs is an array of sub-devices for which the registrar of this notifier
(either the bridge driver or another sub-device driver) should be notified.
- waiting is a list of the sub-devices in this notifier waiting to be probed.
- done is a list of the sub-devices actually bound to this notifier.
- num_subdevs is the number of sub-devices in **subdevs.
- list is used by the async core during the registration of this notifier in order to
link this notifier to the global list of notifiers, notifier_list.

## Back to our struct v4l2_async_subdev structure, which is defined as follows:
```c
struct v4l2_async_subdev {
enum v4l2_async_match_type match_type;
```
union {
```c
struct fwnode_handle *fwnode;
```
const char *device_name;
```c
struct {
int adapter_id;
```
unsigned short address;
```c
} i2c;
struct {
```
bool (*match)(struct device *,
```c
struct v4l2_async_subdev *);
void *priv;
} custom;
} match;
```
372 Integrating with V4L2 Async and Media Controller Frameworks
```c
/* v4l2-async core private: not to be used by drivers */
struct list_head list;
};
```
The preceding data structure is a sub-device in the eyes of the V4L2 async framework.
Only the bridge driver (which allocates the async sub-device) and the async core can play
with this structure. The sub-device driver is not aware of this at all. The meanings of its
members are as follows:
- match_type is of the enum v4l2_async_match_type type. A match is
a comparison of some criteria (occurring strictly between a sub-device of the
```c
struct v4l2_subdev type and an async sub-device of the struct v4l2_
```
async_subdev type). Since each struct v4l2_async_subdev structure
must be associated with its struct v4l2_subdev structure, this field specifies
the algorithm used by the async core to match both. This field is set by the driver
(which is also responsible for allocating asynchronous sub-devices). Possible values
are as follows:
--V4L2_ASYNC_MATCH_DEVNAME, which instructs the async core to use
the device name for the matching. In this case, the bridge driver must set the
```c
v4l2_async_subdev.match.device_name field so that it can match the
```
sub-device device name (that is, dev_name(v4l2_subdev->dev)) when that
sub-device will be probed.
--V4L2_ASYNC_MATCH_FWNODE, which means the async core should use the
firmware node for the match. In this case, the bridge driver must set v4l2_
async_subdev.match.fwnode with the firmware node handle corresponding
to the sub-device's device node so that they can match.
--V4L2_ASYNC_MATCH_I2C is to be used to perform the match by checking
for the I2C adapter ID and address. Using this, the bridge driver must set both
```c
v4l2_async_subdev.match.i2c.adapter_id and v4l2_async_
```
subdev.match.i2c.address. These values will be compared with the address
and the adapter number of the i2c_client object associated with v4l2_
subdev.dev.
--V4L2_ASYNC_MATCH_CUSTOM is the last possibility and means the async
core should use the matching callback set by the bridge driver in v4l2_async_
subdev.match.custom.match. If this flag is set and there is no custom
matching callback provided, any matching attempt will immediately return true.
- list is used to add this async sub-device waiting to be probed in the waiting list
of a notifier.

## The V4L2 async interface and the concept of graph binding 373
Sub-device registration does not depend on the bridge availability anymore and only
consists of calling the v4l2_async_unregister_subdev() method. However, prior
to registering itself, the bridge driver will have to do the following:
1. Allocate a notifier for later use. It is better to embed this notifier in a larger
device state data structure. This notifier object is of the struct v4l2_async_
notifier type.
2. Parse its port node(s) and create an async sub-device (struct v4l2_async_
subdev) for each sensor (or IP block) specified there and that it needs for its
operations:
a) This parsing is done using the fwnode graph API (old drivers still use the of_
graph API), such as the following:
--fwnode_graph_get_next_endpoint() (or of_graph_get_next_
endpoint() in old drivers) to grab the fw_handle (or the of_node in old
drivers) of an endpoint from within the bridge's port subnode.
--fwnode_graph_get_remote_port_parent() (or of_graph_get_
remote_port_parent() in old drivers) to grab the fw_handle (or the device's
of_node in old drivers) corresponding to the parent of the remote port of the
current endpoint.
Optionally (in old drivers using the OF API), of_fwnode_handle() is used in
order to convert the of_node grabbed in the previous state into an fw_handle.
b) Set up the current async sub-device according to the matching logic that should
be used. It should set the v4l2_async_subdev.match_type and v4l2_
async_subdev.match members.
c) Add this async sub-device to the list of async sub-devices of the notifier. As
of version 4.20 of the kernel, there is a helper, v4l2_async_notifier_add_
subdev(), allowing you to do this.
3. Register the notifier object (this notifier will be stored in the global notifier_
list list defined in drivers/media/v4l2-core/v4l2-async.c) using
the v4l2_async_notifier_register(&big_struct->v4l2_dev,
&big_struct->notifier) call. To unregister the notifier, the driver has to call
```c
v4l2_async_notifier_unregister(&big_struct->notifier).
```
374 Integrating with V4L2 Async and Media Controller Frameworks
When the bridge driver invokes v4l2_async_notifier_register(), the async
core iterates over async sub-devices in the notifier->subdevs array. For each
async sub-device inside, the core checks whether this asd->match_type value is

## V4L2_ASYNC_MATCH_FWNODE. If applicable, the async core makes sure asd is not
present in the notifier->waiting list or in the notifier->done list by comparing
fwnodes. This provides assurance that asd was not already set up for fwnode and it
does not already exist in the given notifier. If asd is not already known, it is added to
notifier->waiting. After this, the async core will test all async sub-devices in the
notifier->waiting list for a match with all sub-devices present in subdev_list,
which is the list of "kind-of " orphan sub-devices, those that were registered prior to their
bridge driver (thus prior to their notifier). The async core uses the asd->match value
of each current asd for this. If a match occurs (the asd->match callback returns true),
the current async sub-device (from notifier->waiting) and the current sub-device
(from subdev_list) will be bound, the async sub-device will be removed from the
notifier->waiting list, the sub-device will be registered with the V4L2 core using
```c
v4l2_device_register_subdev(), and the sub-device will be moved from the
```
global subdev_list list to the notifier->done list.
Finally, the actual notifier being registered will be added to the global list of notifiers,
notifier_list, so that it can be used later for matching attempts whenever a new
sub-device is registered with the async core.

## Important note

## What the async core does when the sub-device driver invokes v4l2_async_
register_subdev() can be guessed from the preceding matching and
bounding logic descriptions. Effectively, upon this call, the async core will
attempt to match the current sub-device with all the async sub-devices waiting
in each notifier present in the notifier_list global list. If no match
occurs, it means this sub-device's bridge has not been probed yet, and the
sub-device is added to the global list of sub-devices, subdev_list.
If a match occurs, the sub-device will not be added to this list at all.
Do also keep in mind that a match test is a comparison of some criteria,
occurring strictly between a sub-device of the struct v4l2_subdev
type and an async sub-device of the struct v4l2_async_subdev type.

## The V4L2 async interface and the concept of graph binding 375
In the preceding paaragraphs, we said the async sub-device and the sub-device are bound.
But what does this mean? Here is where the notifier->ops member comes into
the picture. It is of the struct v4l2_async_notifier_operations type and is
defined as follows:
```c
struct v4l2_async_notifier_operations {
int (*bound)(struct v4l2_async_notifier *notifier,
struct v4l2_subdev *subdev,
struct v4l2_async_subdev *asd);
int (*complete)(struct v4l2_async_notifier *notifier);
void (*unbind)(struct v4l2_async_notifier *notifier,
struct v4l2_subdev *subdev,
struct v4l2_async_subdev *asd);
};
```
The following are the meanings of each callback in this structure despite the fact that all
three callbacks are optional:
- bound: If set, this callback will be invoked by the async core in response to a
successful sub-device probing by its (sub-device) driver. This also implies that an
async sub-device has successfully matched this sub-device. This callback takes as an
argument the notifier that originated the match, as well as the sub-device (subdev)
and the async sub-device (asd) that matched. Most drivers simply print debug
messages here. However, you can perform additional setup on the sub-device
here – that is, v4l2_subdev_call(). If everything seems OK, it should return
a positive value; otherwise, the sub-device is unregistered.
- unbind is invoked when a sub-device is removed from the system. In addition to
printing debug messages here, the bridge driver must unregister the video device
if the unbound sub-device was a requirement for it to work normally – that is,
video_unregister_device().
- complete is invoked when there are no more async sub-devices waiting in the
notifier. The async core can detect when the notifier->waiting list is empty
(which would mean sub-devices have been probed successfully and are all moved
into the notifier->done list). The complete callback is only executed for the
root notifier. Sub-devices that registered notifiers will not have their .complete
callback invoked. The root notifier is usually the one registered by the bridge device.
376 Integrating with V4L2 Async and Media Controller Frameworks
There is no doubt, then, that, prior to registering the notifier object, the bridge driver must
set the notifier's ops member. The most important callback for us is .complete.
While you can call v4l2_device_register() from within the bridge driver's probe
function, it is a common practice to register the actual video device from within the
notifier.complete callback, as all sub-devices would be registered, and the presence
of /dev/videoX would mean it is really usable. The .complete callback is also
suitable for both registering the actual video device's subnode and registering the media
device by means of v4l2_device_register_subdev_nodes() and
```c
media_device_register().
```
Note that v4l2_device_register_subdev_nodes() will create a device
node (/dev/v4l2-subdevX, actually) for every subdev object marked with the

## V4L2_SUBDEV_FL_HAS_DEVNODE flag.

## Async bridge and sub-device probing example
We will go through this section with a simple use case. Consider the following config:
- One bridge device (our CSI controller) – let's say the omap ISP, with foo as
its name.
- One off-chip sub-device, the camera sensor, with bar as its name.

## Both are connected this way: CSI <-- Camera Sensor.

## In the bar driver, we could register an async sub-device as follows:
```c
static int bar_probe(struct device *dev)
{
int ret;
```
ret = v4l2_async_register_subdev(subdev);
```c
if (ret) {
```
dev_err(dev, "ouch\n");
```c
return -ENODEV;
}
return 0;
}

```
## The V4L2 async interface and the concept of graph binding 377

## The probe function of the foo driver could be as follows:
```c
/* struct foo_device */
struct foo_device {
struct media_device mdev;
struct v4l2_device v4l2_dev;
struct video_device *vdev;
struct v4l2_async_notifier notifier;
struct *subdevs[FOO_MAX_SUBDEVS];
};
/* foo_probe() */
static int foo_probe(struct device *dev)
{
struct foo_device *foo = kmalloc(sizeof(*foo));
media_device_init(&bar->mdev);
```
foo->dev = dev;
foo->notifier.subdevs = kcalloc(FOO_MAX_SUBDEVS,
sizeof(struct v4l2_async_subdev));
foo_parse_nodes(foo);
foo->notifier.bound = foo_bound;
foo->notifier.complete = foo_complete;
return
```c
v4l2_async_notifier_register(&foo->v4l2_dev,
```
&foo->notifier);
```c
}
```
In the following code, we implement the foo fwnode (or of_node) parser helper,
foo_parse_nodes():
```c
struct foo_async {
struct v4l2_async_subdev asd;
struct v4l2_subdev *sd;
};
/* Either */
static void foo_parse_nodes(struct device *dev,
struct v4l2_async_notifier *n)
```
378 Integrating with V4L2 Async and Media Controller Frameworks
```c
{
struct device_node *node = NULL;
while ((node = of_graph_get_next_endpoint(dev->of_node,
```
node))) {
```c
struct foo_async *fa = kmalloc(sizeof(*fa));
```
n->subdevs[n->num_subdevs++] = &fa->asd;
fa->asd.match.of.node =
of_graph_get_remote_port_parent(node);
fa->asd.match_type = V4L2_ASYNC_MATCH_OF;
```c
}
}
/* Or */
static void foo_parse_nodes(struct device *dev,
struct v4l2_async_notifier *n)
{
struct fwnode_handle *fwnode = dev_fwnode(dev);
struct fwnode_handle *ep = NULL;
while ((ep = fwnode_graph_get_next_endpoint(ep, fwnode))) {
struct foo_async *fa = kmalloc(sizeof(*fa));
```
n->subdevs[n->num_subdevs++] = &fa->asd;
fa->asd.match.fwnode =
fwnode_graph_get_remote_port_parent(ep);
fa->asd.match_type = V4L2_ASYNC_MATCH_FWNODE;
```c
}
}
```
In the preceding code, both of_graph_get_next_endpoint() and
fwnode_graph_get_next_endpoint() have been used in order to show
how to play with the two. That being said, you're better off using the fwnode version,
as it is much more generic.
In the meantime, we need to write foo's notifier operations, which could look as follows:
```c
/* foo_bound() and foo_complete() */
static int foo_bound(struct v4l2_async_notifier *n,
struct v4l2_subdev *sd,
struct v4l2_async_subdev *asd)
{
struct foo_async *fa = container_of(asd, struct bar_async,

```
## The V4L2 async interface and the concept of graph binding 379
asd);
```c
/* One can use subdev_call here */
```
[...]
fa->sd = sd;
```c
}
static int foo_complete(struct v4l2_async_notifier *n)
{
struct foo_device *foo =
```
container_of(n, struct foo_async, notifier);
```c
struct v4l2_device *v4l2_dev = &isp->v4l2_dev;
/* Create /dev/sub-devX if applicable */
v4l2_device_register_subdev_nodes(&foo->v4l2_dev);
/* setup the video device: fops, queue, ioctls ... */
```
[...]
```c
/* Register the video device */
```
ret = video_register_device(foo->vdev,
VFL_TYPE_GRABBER, -1);
```c
/* Register with the media controller framework */
return media_device_register(&bar->mdev);
}
```
In the device tree, the V4L2 bridge device can be declared as follows:
csi1: csi@1cb4000 {
compatible = "allwinner,sun8i-v3s-csi";
reg = <0x01cb4000 0x1000>;
interrupts = <GIC_SPI 84 IRQ_TYPE_LEVEL_HIGH>;
```c
/* we omit clock and others */
```
[...]
port {
csi1_ep: endpoint {
remote-endpoint = <&ov7740_ep>;
380 Integrating with V4L2 Async and Media Controller Frameworks
```c
/* We omit v4l2 related properties */
```
[...]
```c
};
};
};
```
The camera node from within the I2C controller node can be declared as follows:
&i2c1 {
#address-cells = <1>;
#size-cells = <0>;
ov7740: camera@21 {
compatible = "ovti,ov7740";
reg = <0x21>;
```c
/* We omit clock or pincontrol or everything else */
```
[...]
port {
ov7740_ep: endpoint {
remote-endpoint = <&csi1_ep>;
```c
/* We omit v4l2 related properties */
```
[...]
```c
};
};
};
};
```
Now we are familiar with the V4L2 async framework and we have seen how the
asynchronous sub-device registration eases both the probe and the code. We ended
with a concrete example that highlights each aspect we have discussed. Now we can
move forward and integrate with the media controller framework, which is the last
improvement we can add to our V4L2 drivers.

## The Linux media controller framework
Media devices turn out to be very complex, involving several IP blocks of the SoC and
thus requiring video stream (re)routing.

## The Linux media controller framework 381
Now, let's consider a case where we have a much more sophisticated SoC made of two
more on-chip sub-devices – let's say a resizer and an image converter, called baz and biz.
In the previous example in the V4L2 async section, the setup was made up of one bridge
device and one sub-device (the fact that it is off-chip does not matter), the camera sensor.
This was quite straightforward. Luckily, things worked. But what if now we have to route
the stream through the image converter or the image resizer, or even through both IPs?
Or, say we have to switch from one to the other (dynamically)?
We could achieve this either via sysfs or ioctls, but this would have the following
problems:
- It would be too ugly (no doubt) and probably buggy.
- It would be too hard (a lot of work).
- It would be deeply SoC vendor-dependent, with possibly a lot of code duplication,
no unified user space API and ABI, and no consistency between drivers.
- It would be not a very credible solution.
Many SoCs can reroute internal video streams – for example, capturing them from
a sensor and doing memory-to-memory resizing, or sending the sensor output directly
to the resizer. Since the V4L2 API did not support these advanced devices, SoC
manufacturers made their own custom drivers. However, V4L2 is undisputably the Linux
API for capturing images and is sometimes used for specific display devices (these are
mem2mem devices).
It is becoming clear that we need another subsystem and framework that covers the limits
of V4L2. This is how the Linux media controller framework was born.

## The media controller abstraction model
Discovering a device's internal topology and configuring it at runtime is one of the goals
of the media framework. To achieve this, it comes with a layer of abstraction. With the
media controller framework, hardware devices are represented through an oriented graph
made of entities whose pads are connected via links. This set of elements put together
forms the so-called media device. A source pad can only generate data.
382 Integrating with V4L2 Async and Media Controller Frameworks
The preceding short description deserves some attention. There are three highlighted
words that are of high interest: entity, pad, and link:
- Entities are represented by a struct media_entity instance, defined in
include/media/media-entity.h. The structure is usually embedded into
a higher-level structure, such as a v4l2_subdev or video_device instance,
although drivers can allocate entities directly.
- Pads are the entity's interface to the outside world. These are input- and
output-connectable points of a media entity. However, a pad can be either an input
(sink pad) or an output (source pad), not both. Data streams from one entity's
source pad to another entity's sink pad. Typically, a device such as a sensor or
a video decoder would have only an output pad since it only feeds video into the
system, and a /dev/videoX pad would be modeled as an input pad since it is the
end of the stream.
- Links: These links can be set, fetched, and enumerated through the media device.
The application, for a driver to properly work, is responsible for setting up the links
properly so that the driver understands the source and destination of the video data.
All the entities on the system, along with their pads and the connection links between
them, give the media device shown in the following diagram:

## Figure 8.1 – Media controller abstraction model
In the preceding diagram, Stream would be the equivalent of a /dev/videoX char
device as it is the end of the stream.

## The Linux media controller framework 383

## V4L2 device abstraction

## At a higher level, the media controller uses struct media_device to abstract
```c
struct v4l2_device in the V4L2 framework. That being said, struct media_
```
device is to the media controller what struct v4l2_device is to V4L2, englobing
other lower-level structures. Back to struct v4l2_device, the mdev member is used
by the media controller framework to abstract this structure. The following is an excerpt:
```c
struct v4l2_device {
```
[...]
```c
struct media_device *mdev;
```
[...]
```c
};
```
However, from a media controller point of view, V4L2 video devices and sub-devices
are all seen as media entities, represented in this framework as instances of struct
```c
media_entity. It is then obvious for the video device and sub-device data structures
```
to embed a member of this type, as shown in the following excerpt:
```c
struct video_device
{
```
#if defined(CONFIG_MEDIA_CONTROLLER)
```c
struct media_entity entity;
struct media_intf_devnode *intf_devnode;
struct media_pipeline pipe;
```
#endif
[...]
```c
};
struct v4l2_subdev {
```
#if defined(CONFIG_MEDIA_CONTROLLER)
```c
struct media_entity entity;
```
#endif
[...]
```c
};
```
384 Integrating with V4L2 Async and Media Controller Frameworks
The video device has additional members, intf_devnode and pipe. The former, of the
```c
struct media_intf_devnode type, represents the media controller interface to the
```
video device node. This structure gives the media controller access to information of the
underlying video device node, such as its major and minor numbers. The other additional
member, pipe, which is of the struct media_pipeline type, stores information
related to the streaming pipeline of this video device.

## Media controller data structures
The media controller framework is based on a few data structures, among which is the
```c
struct media_device structure, which is on top of the hierarchy and defined as
```
follows:
```c
struct media_device {
/* dev->driver_data points to this struct. */
struct device *dev;
struct media_devnode *devnode;
```
char model[32];
char driver_name[32];
[...]
char serial[40];
u32 hw_revision;
u64 topology_version;
```c
struct list_head entities;
struct list_head pads;
struct list_head links;
struct list_head entity_notify;
struct mutex graph_mutex;
```
[...]
const struct media_device_ops *ops;
```c
};

```
## The Linux media controller framework 385
This structure represents a high-level media device. It allows easy access to entities and
provides basic media device-level support:
- dev is the parent device for this media device (usually a &pci_dev, &usb_
interface, or &platform_device instance).
- devnode is the media device node, abstracting the underlying /dev/mediaX.
- driver_name is an optional but recommended field, representing the media
device driver name. If not set, it defaults to dev->driver->name.
- model is the model name of this media device. It doesn't have to be unique.
- serial is an optional member that should be set with the device serial number.
hw_revision is the hardware device revision for this media device.
- topology_version: Monotonic counter for storing the version of the graph
topology. Should be incremented each time the topology changes.
- entities is the list of registered entities.
- pads is the list of pads registered with this media device.
- links is the list of links registered with this media device.
- entity_notify is the notify callback list invoked when a new entity is
registered with this media device. Drivers may register this callback to take action
via media_device_unregister_entity_notify() and unregister it
using media_device_register_entity_notify(). All the registered
```c
media_entity_notify callbacks are invoked when a new entity is registered.
```
- graph_mutex: Protects access to struct media_device data. It should, for
example, be held when using media_graph_* family functions.
- ops is of the struct media_device_ops type and represents the operation
handler callbacks for this media device.
386 Integrating with V4L2 Async and Media Controller Frameworks

## In addition to being manipulated by the media controller framework, struct
```c
media_device is essentially used in the bridge driver, where it is initialized and
```
registered. That being said, the media device on its own is made up of several entities. This
concept of entities allows the media controller to be the central authority when it comes to
modern and complex V4L2 drivers that may also support framebuffers, ALSA, I2C, LIRC,
and/or DVB devices at the same time and is used to inform user space of what is what.
A media entity is represented as an instance of struct media_entity, defined in
include/media/media-entity.h as follows:
```c
struct media_entity {
struct media_gobj graph_obj;
```
const char *name;
```c
enum media_entity_type obj_type;
```
u32 function;
unsigned long flags;
u16 num_pads;
u16 num_links;
u16 num_backlinks;
```c
int internal_idx;
struct media_pad *pads;
struct list_head links;
```
const struct media_entity_operations *ops;
```c
int stream_count;
int use_count;
struct media_pipeline *pipe;
```
[...]
```c
};

```
## The Linux media controller framework 387
This is the second data structure in the media framework in terms of hierarchy. The
preceding definition has been shrunk to the minimum that we are interested in. The
following are the meanings of the members in this structure:
- name is the name of this entity. It should be meaningful enough as it is used as it is
in user space with the media-ctl tool.
- type is most of the time set by the core depending on the type of V4L2 video data
structure this struct is embedded in. It is the type of the object that implements
```c
media_entity – for example, set with MEDIA_ENTITY_TYPE_V4L2_SUBDEV
```
at the sub-device initialization by the core. This allows runtime type identification of
media entities and safe casting to the correct object type using the container_of
macro, for instance. Possible values are as follows:
--MEDIA_ENTITY_TYPE_BASE: This means the entity is not embedded in
another.
--MEDIA_ENTITY_TYPE_VIDEO_DEVICE: This indicates the entity is embedded
in a struct video_device instance.
--MEDIA_ENTITY_TYPE_V4L2_SUBDEV: This means the entity is embedded in
a struct v4l2_subdev instance.
- function represents the entity's main function. This must be set by the driver
according to the value defined in include/uapi/linux/media.h. The
following are commonly used values while dealing with video devices:
--MEDIA_ENT_F_IO_V4L: This flag means the entity is a data streaming input
and/or output entity.
--MEDIA_ENT_F_CAM_SENSOR: This flag means this entity is a camera video
sensor entity.
--MEDIA_ENT_F_PROC_VIDEO_SCALER: Means this entity can perform video
scaling. These entities have at least one sink pad, from which they receive frame(s)
(on the active one) and one source pad where they output the scaled frame(s).
388 Integrating with V4L2 Async and Media Controller Frameworks
--MEDIA_ENT_F_PROC_VIDEO_ENCODER: Means this entity is capable of
compressing video. These entities must have one sink pad and at least one
source pad.
--MEDIA_ENT_F_VID_MUX: This is to be used for a video multiplexer. This entity
```c
has at least two sink pads and one source pad and must pass the video frame(s)
```
received from the active sink pad to the source pad.
--MEDIA_ENT_F_VID_IF_BRIDGE: Video interface bridge. A video interface
bridge entity should have at least one sink pad and one source pad. It receives video
frames on its sink pad from an input video bus of one type (HDMI, eDP, MIPI
CSI-2, and so on) and outputs them on its source pad to an output video bus of
another type (eDP, MIPI CSI-2, parallel, and so on).
- flags is set by the driver. It represents the flags for this entity. Possible values are
the MEDIA_ENT_FL_* flag family defined in include/uapi/linux/media.h.
The following link may be of help to you to understand the possible values:
https://linuxtv.org/downloads/v4l-dvb-apis/userspace-api/
mediactl/media-types.html.
- function represents this entity's function and by default is MEDIA_ENT_F_

## V4L2_SUBDEV_UNKNOWN. Possible values are the MEDIA_ENT_F_* function
family defined in include/uapi/linux/media.h. For example, a camera
sensor sub-device driver must contain sd->entity.function = MEDIA_

## ENT_F_CAM_SENSOR;. You can follow this link to find detailed information
on what may be suitable for your media entity: https://linuxtv.org/
downloads/v4l-dvb-apis/uapi/mediactl/media-types.html.
- num_pads is the total number of pads of this entity (sink and source).
- num_links is the total number of links of this entity (forward, back, enabled,
and disabled)
- num_backlinks is the numbers of backlinks of this entity. Backlinks are used to
help graph traversal and are not reported to user space.
- internal_idx: A unique entity number assigned by the media controller core
when the entity is registered.
- pads is the array of pads of this entity. Its size is defined by num_pads.
- links is the list of data links of this entity. See media_add_link().
- ops is of the media_entity_operations type and represents operations for
this entity. This structure will be discussed later.
- stream_count: Stream count for the entity.

## The Linux media controller framework 389
- use_count: The use count for the entity. Used for power management purposes.
- pipe is the media pipeline that this entity belongs to.
Naturally, the next data structure that seems obvious for us to introduce is the struct
```c
media_pad structure, which represents a pad in this framework. A pad is a connection
```
endpoint through which an entity can interact with other entities. Data (not restricted to
video) produced by an entity flows from the entity's output to one or more entity inputs.
Pads should not be confused with the physical pins at chip boundaries. struct
```c
media_pad is defined as follows:
struct media_pad {
```
[...]
```c
struct media_entity *entity;
```
u16 index;
unsigned long flags;
```c
};
```
Pads are identified by their entity and their 0-based index in the entity's pads array. In
the flags field, either MEDIA_PAD_FL_SINK (which indicates that the pad supports
sinking data) or MEDIA_PAD_FL_SOURCE (which indicates that the pad supports
sourcing data) can be set, but not both at the same time, since a pad can't both sink
and source.
Pads are meant to be bound together to allow data flow paths. Two pads, either from the
same entity or from different entities, are bound together by means of point-to-pointoriented connections called links. Links are represented in the media framework as
instances of struct media_link, defined as follows:
```c
struct media_link {
struct media_gobj graph_obj;
struct list_head list;
```
[...]
```c
struct media_pad *source;
struct media_pad *sink;
```
[...]
```c
struct media_link *reverse;
```
unsigned long flags;
bool is_backlink;
```c
};
```
390 Integrating with V4L2 Async and Media Controller Frameworks
In the preceding code block, only a few fields have been listed for the sake of readability.

## The following are the meanings of those fields:
- list: Used to associate this link with the entity or interface owning the link.
- source: Where this link originates from.
- sink: The link target.
- flags: Represents the link flags, as defined in uapi/media.h (with the
MEDIA_LNK_FL_* pattern). The following are the possible values:
--MEDIA_LNK_FL_ENABLED: This flag means the link is enabled and is ready for
data transfer.
--MEDIA_LNK_FL_IMMUTABLE: This flag means the link enabled state can't be
modified at runtime.
--MEDIA_LNK_FL_DYNAMIC: This flag means the state of the link can be
modified during streaming. However, this flag is set by drivers but is read-only for
applications.
- reverse: Pointer to the link (the backlink, actually) for the reverse direction of
a pad-to-pad link.
- is_backlink: Tells whether this link is a backlink.
Each entity has a list that points to all links originating at or targeting any of its pads.
A given link is thus stored twice, once in the source entity and once in the target entity.
When you want to link A to B, two links are actually created:
- One that corresponds to what was expected; the link is stored in the source entity,
and the source entity's num_links field is incremented.
- Another one is stored in the sink entity. The sink and source remain the same,
with the difference being that the is_backlink member is set to true.
This corresponds to the reverse of the link you created. The sink entity's num_
backlinks and num_links fields will be incremented. This backlink is then
assigned to the original link's reverse member.

## The Linux media controller framework 391

## At the end, the mdev->topology_version member is incremented twice. This
principle of link and backlink allows the media controller to numerate entities, along with
the possible and current links between entities, such as in the following diagram:

## Figure 8.2 – Media controller entity description
In the preceding diagram, if we consider Entity-1 and Entity-2, then link and backlink
are essentially the same, except that link belongs to Entity-1 and backlink belongs to
Entity-2. You should then consider the backlink as a backup link. We can see that an
entity can be either a sink, a source, or both.
The data structures we have introduced so far may make the media controller framework
sound a bit scary. However, most of those data structures will be managed under the
hood by the framework by means of the APIs it offers. That being said, the complete
framework's documentation can be found in Documentation/media-framework.
txt in the kernel sources.

## Integrating media controller support in the driver
When the support of the media controller is needed, the V4L2 driver must first initialize
```c
struct media_device within struct v4l2_device using the media_
```
device_init() function. Each entity driver must initialize its entities (actually
video_device->entity or v4l2_subdev->entity) and its pad arrays using the
```c
media_entity_pads_init() function and, if needed, create pad-to-pad links using
media_create_pad_link(). After that, entities can be registered. However, the
```
V4L2 framework will handle this registration for you through either the v4l2_device_
register_subdev() or the video_register_device() methods. In both cases,
the underlying registration function that is invoked is media_device_register_
entity().
392 Integrating with V4L2 Async and Media Controller Frameworks
As a final step, the media device has to be registered using media_device_
register(). It's worth mentioning that the media device registration should be
postponed to later in the future when we are sure that every sub-device (or should I say
entities) is registered and ready to be used. It definitely makes sense registering the media
device in the root notifier's .complete callback.

## Initializing and registering pads and entities
The same function is used to initialize both the entity and its pad array:
```c
int media_entity_pads_init(struct media_entity *entity,
```
u16 num_pads, struct media_pad *pads);
In the preceding prototype, *entity is the entity to which the pads to be registered
belong, *pads is the array of pads to be registered, and num_pads is the number of
entities in the array that should be registered. The driver must have set the type of every
pad in the pads array before calling:
```c
struct mydrv_state_struct {
struct v4l2_subdev sd;
struct media_pad pad;
```
[...]
```c
};
static int my_probe(struct i2c_client *client,
```
const struct i2c_device_id *id)
```c
{
struct v4l2_subdev *sd;
struct mydrv_state_struct *my_struct;
```
[...]
sd = &my_struct->sd;
my_struct->pad.flags = MEDIA_PAD_FL_SINK |
MEDIA_PAD_FL_MUST_CONNECT;
ret = media_entity_pads_init(&sd->entity, 1,
&my_struct->pad);
[...]
```c
return 0;
}

```
## The Linux media controller framework 393
Drivers that need to unregister entities must call the following function on the entity to be
unregistered:
```c
media_device_unregister_entity(struct media_entity *entity);
```
Then, in order for a driver to free resources associated with an entity, it should call the
following:
```c
media_entity_cleanup(struct media_entity *entity);
```
When a media device is unregistered, all of its entities are unregistered automatically.

## No unregistration of manual entities is then required.

## Media entity operations
An entity may be provided link-related callbacks, so that these can be invoked by the
media framework upon link creation and validation:
```c
struct media_entity_operations {
int (*get_fwnode_pad)(struct fwnode_endpoint *endpoint);
int (*link_setup)(struct media_entity *entity,
```
const struct media_pad *local,
const struct media_pad *remote,
u32 flags);
```c
int (*link_validate)(struct media_link *link);
};
```
Providing the preceding structure is optional. However, there may be situations where
additional stuff needs to be done or checked either at link setup or link validation. In this
case, note the following descriptions:
- get_fwnode_pad: Returns the pad number based on a fwnode endpoint or
a negative value on error. This operation can be used to map a fwnode to a media
pad number (optional).
- link_setup: Notifies the entity of link changes. This operation can return an
error, in which case the link setup will be canceled (optional).
394 Integrating with V4L2 Async and Media Controller Frameworks
- link_validate: Returns whether a link is valid from the entity point of view.
The media_pipeline_start() function validates all the links this entity is
involved in by calling this operation. This member is optional. However, if it has not
been set, then v4l2_subdev_link_validate_default will be used as the
default callback function, which ensures that the source pad and sink pad width,
height, and media bus pixels code are consistent; otherwise, it will return an error.

## The concept of a media bus
The main purpose of the media framework is to configure and control the pipeline and
its entities. Video sub-devices, such as cameras and decoders, connect to video bridges
or other sub-devices over specialized buses. Data is being transferred over these buses in
various formats. That being said, in order for two entities to actually exchange data, their
pad configs need to be the same.
Applications are responsible for configuring coherent parameters on the whole pipeline
and ensuring that connected pads have compatible formats. The pipeline is checked for
formats that are mismatching at VIDIOC_STREAMON time.
The driver is responsible for applying the configuration of every block in the video
pipeline according to the requested (from the user) format at the pipeline input
and/or output.
Take the following simple data flow, sensor ---> CPHY ---> csi ---> isp
---> stream.
In order for the media framework to be able to configure the bus prior to streaming data,
the driver needs to provide some pad-level setter and getter for the media bus properties,
which are present in the struct v4l2_subdev_pad_ops structure. This structure
implements pad-level operations that have to be defined if the sub-device driver intends to
process the video and integrate with the media framework. The following is its definition:
```c
struct v4l2_subdev_pad_ops {
```
[...]
```c
int (*enum_mbus_code)(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_mbus_code_enum *code);
int (*enum_frame_size)(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_frame_size_enum *fse);
int (*enum_frame_interval)(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,

```
## The Linux media controller framework 395
```c
struct v4l2_subdev_frame_interval_enum *fie);
int (*get_fmt)(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_format *format);
int (*set_fmt)(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_format *format);
```
#ifdef CONFIG_MEDIA_CONTROLLER
```c
int (*link_validate)(struct v4l2_subdev *sd,
struct media_link *link,
struct v4l2_subdev_format *source_fmt,
struct v4l2_subdev_format *sink_fmt);
```
#endif /* CONFIG_MEDIA_CONTROLLER */
[...]
```c
};

```
## The following are the meanings of the members in this structure:
- init_cfg: Initializes the pad config to default values. This is the right place to
initialize cfg->try_fmt, which can be grabbed through v4l2_subdev_get_
try_format().
- enum_mbus_code: Callback for the VIDIOC_SUBDEV_ENUM_MBUS_CODE
ioctl handler code. Enumerates the currently supported data format. This callback
handles pixel format enumeration.
- enum_frame_size: Callback for the VIDIOC_SUBDEV_ENUM_FRAME_SIZE
ioctl handler code. Enumerates the frame (image) size supported by the sub-device.

## Enumerates the currently supported resolution.
- enum_frame_interval: Callback for the VIDIOC_SUBDEV_ENUM_FRAME_

## INTERVAL ioctl handler code.
- get_fmt: Callback for the VIDIOC_SUBDEV_G_FMT ioctl handler code.
- set_fmt: Callback for the VIDIOC_SUBDEV_S_FMT ioctl handler code. Sets the
output data format and resolution.
- get_selection: Callback for the VIDIOC_SUBDEV_G_SELECTION ioctl
handler code.
396 Integrating with V4L2 Async and Media Controller Frameworks
- set_selection: Callback for the VIDIOC_SUBDEV_S_SELECTION ioctl
handler code.
- link_validate: Used by the media controller code to check whether the links
that belong to a pipeline can be used for the stream.
The argument that all of these callbacks have in common is cfg, which is of the struct
```c
v4l2_subdev_pad_config type and is used for storing sub-device pad information.

```
## This structure is defined in include/uapi/linux/v4l2-mediabus.h as follows:
```c
struct v4l2_subdev_pad_config {
struct v4l2_mbus_framefmt try_fmt;
struct v4l2_rect try_crop;
```
[...]
```c
};
```
In the preceding code block, the main field we are interested in is try_fmt, which is of
the struct v4l2_mbus_framefmt type. This data structure is used to describe the
pad-level media bus format and is defined as follows:
```c
struct v4l2_subdev_format {
```
__u32 which;
__u32 pad;
```c
struct v4l2_mbus_framefmt format;
```
[...]
```c
};
```
In the preceding structure, which is the format type (try or active) and pad is the pad
number as reported by the media API. This field is set by user space. format represents
the frame format on the bus. The format term here means a combination of the media
bus data format, frame width, and frame height. It is of the struct v4l2_mbus_
framefmt type and its turn is defined as follows:
```c
struct v4l2_mbus_framefmt {
```
__u32 width;
__u32 height;
__u32 code;
__u32 field;
__u32 colorspace;
[...]
```c
};

```
## The Linux media controller framework 397
In the preceding bus frame format data structure, only the fields that are relevant to
us have been listed. width and height, respectively, represent the image width and
height. code is from enum v4l2_mbus_pixelcode and represents the data
format code. field indicates the used interlacing type, which should be from enum
```c
v4l2_field, and colorspace represents the color space of the data from enum
v4l2_colorspace.
```
Now, let's pay more attention to the get_fmt and set_fmt callbacks. They get and
set, respectively, the data format on a sub-device pad. These ioctl handlers are used to
negotiate the frame format at specific sub-device pads in the image pipeline. To set the
current format applications, set the .pad field of struct v4l2_subdev_format to
the desired pad number as reported by the media API and the which field (which is from
```c
enum v4l2_subdev_format_whence) to either V4L2_SUBDEV_FORMAT_TRY or

```
## V4L2_SUBDEV_FORMAT_ACTIVE, and issue a VIDIOC_SUBDEV_S_FMT ioctl with a
pointer to this structure. This ioctl ends up calling the v4l2_subdev_pad_ops->set_
fmt callback. If which is set to V4L2_SUBDEV_FORMAT_TRY, then the driver should
set the .try_fmt field of the requested pad config with the values of the try format
given in the argument. However, if which is set to V4L2_SUBDEV_FORMAT_ACTIVE,
the driver must then apply the config to the device. It is common in this case to store the
requested "active" format in a driver-state structure and apply it to the underlying device
when the pipeline starts the stream. This way, the right place to actually apply the format
config to the device is from within a callback invoked at the start of the streaming, such
as v4l2_subdev_video_ops.s_stream, for example. The following is an example
from the RCAR CSI driver:
```c
static int rcsi2_set_pad_format(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_format *format)
{
struct v4l2_mbus_framefmt *framefmt;
/* retrieve the private data structure */
struct rcar_csi2 *priv = sd_to_csi2(sd);
```
[...]
```c
/* Store the requested format so that it can be applied to
```
* the device when the pipeline starts
```c
*/
if (format->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
```
priv->mf = format->format;
398 Integrating with V4L2 Async and Media Controller Frameworks
```c
} else { /* V4L2_SUBDEV_FORMAT_TRY */
/* set the .try_fmt of this pad config with the
```
* value of the requested "try" format
```c
*/
```
framefmt = v4l2_subdev_get_try_format(sd, cfg, 0);
*framefmt = format->format;
```c
/* driver is free to update any format->* field */
```
[...]
```c
}
return 0;
}
```
Also, note that the driver is free to change the values in the requested format to the one
it actually supports. It is then up to the application to check for it and adapt its logic
according to the format granted by the driver. Modifying those try formats leaves the
device state untouched.
On the other hand, when it comes to retrieving the current format, applications should
do the same as the preceding and issue a VIDIOC_SUBDEV_G_FMT ioctl. This ioctl will
end up calling the v4l2_subdev_pad_ops->get_fmt callback. The driver fills the
members of the format field either with the currently active format values or with the
last try format stored (most of the time in the driver-state structure):
```c
static int rcsi2_get_pad_format(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg,
struct v4l2_subdev_format *format)
{
struct rcar_csi2 *priv = sd_to_csi2(sd);
if (format->which == V4L2_SUBDEV_FORMAT_ACTIVE)
```
format->format = priv->mf;
else
format->format = *v4l2_subdev_get_try_format(sd, cfg, 0);
```c
return 0;
}

```
## The Linux media controller framework 399
It is obvious that the .try_fmt field of the pad config should have been initialized before
it can be passed to the get callback for the first time, and the v4l2_subdev_pad_ops.
init_cfg callback is the right place for this initialization, as in the following example:
```c
/*
```
* Initializes the TRY format to the ACTIVE format on all pads
* of a subdev. Can be used as the .init_cfg pad operation.
```c
*/
int imx_media_init_cfg(struct v4l2_subdev *sd,
struct v4l2_subdev_pad_config *cfg)
{
struct v4l2_mbus_framefmt *mf_try;
struct v4l2_subdev_format format;
```
unsigned int pad;
```c
int ret;
for (pad = 0; pad < sd->entity.num_pads; pad++) {
```
memset(&format, 0, sizeof(format));
format.pad = pad;
format.which = V4L2_SUBDEV_FORMAT_ACTIVE;
ret = v4l2_subdev_call(sd, pad, get_fmt, NULL, &format);
```c
if (ret)
```
continue;
mf_try = v4l2_subdev_get_try_format(sd, cfg, pad);
*mf_try = format.format;
```c
}
return 0;
}
```
400 Integrating with V4L2 Async and Media Controller Frameworks

## Important note

## The list of supported formats can be found in include/uapi/linux/
videodev2.h from the kernel source, and part of their documentation is
available at this link: https://linuxtv.org/downloads/v4ldvb-apis/userspace-api/v4l/subdev-formats.html.
Now that we are familiar with the concept of media, we can learn how to finally make the
media device part of the system by using the appropriate API to register it.

## Registering the media device
```c
Drivers register media device instances by calling __media_device_register()
```
via the media_device_register() macro and unregister them by calling
```c
media_device_unregister(). Upon successful registration, a character device
```
named media[0-9] + will be created. The device major and minor numbers are
dynamic. media_device_register() accepts a pointer to the media device to be
registered and returns 0 on success or a negative error code on error.
As we said earlier, you're better off registering the media device from within the root
notifier's .complete callback in order to make sure that the actual media device is
registered only after all its entities have been probed. The following is an excerpt from
the TI OMAP3 ISP media driver (the whole code can be found in drivers/media/
platform/omap3isp/isp.c in the kernel sources):
```c
static int isp_subdev_notifier_complete(
struct v4l2_async_notifier *async)
{
struct isp_device *isp =
```
container_of(async, struct isp_device, notifier);
[...]
```c
return media_device_register(&isp->media_dev);
}
static const
struct v4l2_async_notifier_operations isp_subdev_notifier_ops =
{
```
.complete = isp_subdev_notifier_complete,
```c
};

```
## The Linux media controller framework 401
The preceding code shows how you can take benefit of the root notifier's .complete
callback to register the final media device, by means of the media_device_
register() method.
Now that the media device is part of the system, the time has come to leverage it,
particularly from user space. Let's now see how, from the command line, we can take
control of and interact with the media device.

## Media controller from user space
Though it remains the streaming interface, /dev/video0 is not the default pipeline
centerpiece anymore since it is wrapped by /dev/mediaX. The pipeline can be
configured through the media node (/dev/media*), and the control operations, such
as stream on/off, can be performed through the video node (/dev/video*).
Using media-ctl (the v4l-utils package)
The media-ctl application from the v4l-utils package is a user space application
that uses the Linux media controller API to configure pipelines. The following are the flags
to use with it:
- --device <dev> specifies the media device (/dev/media0 by default).
- --entity <name> prints the device name associated with the given entity.
- --set-v4l2 <v4l2> provides a comma-separated list of formats to set up.
- --get-v4l2 <pad> prints an active format on a given pad.
- --set-dv <pad> configures DV timings on a given pad.
- --interactive modifies links interactively.
- --links <linux> provides a comma-separated list of link descriptors to set up.
- --known-mbus-fmts lists known formats and their numeric values.
- --print-topology prints the device topology, or the short version, -p.
- --reset resets all links to inactive.
That being said, the basic configuration steps for a hardware media pipeline are as follows:
1. Reset all links with media-ctl --reset.
2. Configure links with media-ctl --links.
402 Integrating with V4L2 Async and Media Controller Frameworks
3. Configure pad formats with media-ctl --set-v4l2.
4. Configure sub-device properties with v4l2-ctl capture frames on the /dev/
video* device.
Using media-ctl --links to link an entity source pad to an entity sink pad should
follow the following pattern:
media-ctl --links\
"<entitya>:<srcpadn> -> <entityb>:<sinkpadn>[<flags>]
In the preceding line, flags can be either 0 (inactive) or 1 (active). Additionally, to see
the current settings of the media bus, use the following:
$ media-ctl --print-topology
On some systems, media device 0 may not be the default one, in which case you should
use the following:
$ media-ctl --device /dev/mediaN --print-topology
The previous command would print the media topology associated with the specified
media device.
Do note that --print-topology just dumps the media topology on the console in an
ASCII format. However, this topology can be better represented by generating its dot
representation, changing this representation into a graphic image that is more humanfriendly. The following are the commands to use:
$ media-ctl --print-dot > graph.dot
$ dot -Tpng graph.dot > graph.png
For example, in order to set up a media pipe, the following commands have been run on
an UDOO QUAD board. The board has been shipped with an i.MX6 quad core and an

## OV5640 camera plugged into the MIPI CSI-2 connector:
# media-ctl -l "'ov5640 2-003c':0 -> 'imx6-mipi-csi2':0[1]"
# media-ctl -l "'imx6-mipi-csi2':2 -> 'ipu1_csi1':0[1]"
# media-ctl -l "'ipu1_csi1':1 -> 'ipu1_ic_prp':0[1]"
# media-ctl -l "'ipu1_ic_prp':1 -> 'ipu1_ic_prpenc':0[1]"
# media-ctl -l "'ipu1_ic_prpenc':1 -> 'ipu1_ic_prpenc
capture':0[1]"

## The Linux media controller framework 403

## The following is a diagram representing the preceding setup:

## Figure 8.3 – Graph representation of a media device
As you can see, it helps to visualize what the hardware components are. The following are
descriptions of these generated images:
- Dashed lines show possible connections. You can use these to determine the
possibilities.
- Solid lines show active connections.
404 Integrating with V4L2 Async and Media Controller Frameworks
- Green boxes show media entities.
- Yellow boxes show Video4Linux (V4L) endpoints.
After that, you can see that solid lines correspond exactly to the setup that was done
earlier. We have five solid lines, which correspond to the number of commands used to
configure the media device. The following are the meanings of these commands:
- media-ctl -l "'ov5640 2-003c':0 -> 'imx6-mipi-csi2':0[1]"
```c
means linking output pad number 0 of the camera sensor ('ov5640 2-003c':0)
```
to MIPI CSI-2 input pad number 0 ('imx6-mipi-csi2':0) and setting this link
active ([1]).
- media-ctl -l "'imx6-mipi-csi2':2 -> 'ipu1_csi1':0[1]" means
linking output pad number 2 of the MIPI CSI-2 entity ('imx6-mipi-csi2':2)
to the input pad number 0 of the IPU capture sensor interface #1 (' ipu1_
csi1':0) and setting this link active ([1]).
- The same decoding rules apply to other command lines, until the last one,
media-ctl -l "'ipu1_ic_prpenc':1 -> 'ipu1_ic_prpenc
capture':0[1]", which means linking output pad number 1 of ipu1's image
converter preprocessing encode entity ('ipu1_ic_prpenc':1) to the capture
interface input pad number 0 and setting this link to active.
Do not hesitate to go back to the image and read those descriptions several times in order
to understand the concepts of entity, link, and pad.

## Important note
If the dot package is not installed on your target, you can download the .dot
file on your host (assuming it has the package installed) and convert it into an
image.

## WaRP7 with an OV2680 example
The WaRP7 is an i.MX7-based board, which, unlike the i.MX5/6 family, does not contain
an IPU. Because of this, there are fewer capabilities to perform operations or manipulation
of the capture frames. The i.MX7 image capture chain is made up of three units: the
camera censor interface, the video multiplexer, and the MIPI CSI-2 receiver, which
represent the media entities, described as follows:
- imx7-mipi-csi2: This is the MIPI CSI-2 receiver entity. It has one sink pad to
receive the pixel data from the MIPI CSI-2 camera sensor. It has one source pad,
corresponding to virtual channel 0.

## The Linux media controller framework 405
- csi_mux: This is the video multiplexer. It has two sink pads to select from either
camera sensors with a parallel interface or MIPI CSI-2 virtual channel 0. It has
a single source pad that routes to the CSI.
- csi: The CSI allows the chip to connect directly to the external CMOS image
sensor. The CSI can interface directly with parallel and MIPI CSI-2 buses. It has 256
x 64 FIFO to store received image pixel data and embedded DMA controllers to
transfer data from the FIFO through the AHB bus. This entity has one sink pad that
receives from the csi_mux entity and a single source pad that routes video frames
directly to memory buffers. This pad is routed to a capture device node:
|\

## MIPI Camera Input --> MIPI CSI-2 -- > | \
| \
| M |
| U | --> CSI --> Capture
| X |
| /

## Parallel Camera Input --------------> | /
|/
On this platform, an OV2680 MIPI CSI-2 module is connected to the internal MIPI CSI-2
receiver. The following example configures a video capture pipeline with an output of 800
x 600 in BGGR 10-bit Bayer format:
# Setup links
media-ctl --reset
media-ctl -l "'ov2680 1-0036':0 -> 'imx7-mipi-csis.0':0[1]"
media-ctl -l "'imx7-mipi-csis.0':1 -> 'csi_mux':1[1]"
media-ctl -l "'csi_mux':2 -> 'csi':0[1]"
media-ctl -l "'csi':1 -> 'csi capture':0[1]"

## The preceding lines could be merged into one single command, as follows:
media-ctl -r -l '"ov2680 1-0036":0->"imx7-mipi-csis.0":0[1], \
"imx7-mipi-csis.0":1 ->"csi_mux":1[1], \
"csi_mux":2->"csi":0[1], \
"csi":1->"csi capture":0[1]'
406 Integrating with V4L2 Async and Media Controller Frameworks

## In the preceding commands, note the following:
- -r means reset all links to inactive.
- -l sets up links in a comma-separated list of the links' descriptors.
- "ov2680 1-0036":0->"imx7-mipi-csis.0":0[1] links output pad
number 0 of the camera sensor to MIPI CSI-2 input pad number 0 and sets this
link to active.
- "csi_mux":2->"csi":0[1] links output pad number 2 of csi_mux to csi
input pad number 0 and sets this link to active.
- "csi":1->"csi capture":0[1] links output pad number 1 of csi to
capture the interface's input pad number 0 and sets this link to active.
In order to configure the format on each pad, we can use the following commands:
# Configure pads for pipeline
media-ctl -V "'ov2680 1-0036':0 [fmt:SBGGR10_1X10/800x600
field:none]"
media-ctl -V "'csi_mux':1 [fmt:SBGGR10_1X10/800x600
field:none]"
media-ctl -V "'csi_mux':2 [fmt:SBGGR10_1X10/800x600
field:none]"
media-ctl \
-V "'imx7-mipi-csis.0':0 [fmt:SBGGR10_1X10/800x600
field:none]"
media-ctl -V "'csi':0 [fmt:SBGGR10_1X10/800x600 field:none]"
Once again, the preceding command lines could be merged into a single command,
as follows:
media-ctl \
-f '"ov2680 1-0036":0 [SGRBG10 800x600 (32,20)/800x600], \
"csi_mux":1 [SGRBG10 800x600], \
"csi_mux":2 [SGRBG10 800x600], \
"mx7-mipi-csis.0":2 [SGRBG10 800x600], \
"imx7-mipi-csi.0":0 [SGRBG10 800x600], \
"csi":0 [UYVY 800x600]'

## The Linux media controller framework 407

## The preceding command lines could be translated as follows:
- -f: Sets up pad formats into a comma-separated list of format descriptors.
- "ov2680 1-0036":0 [SGRBG10 800x600 (32,20)/800x600]: Sets up
the camera sensor pad number 0 format to a RAW Bayer 10-bit image with
a resolution (capture size) of 800 x 600. Sets the maximum allowed sensor window
width by specifying the crop rectangle.
- "csi_mux":1 [SGRBG10 800x600]: Sets up the csi_mux pad number 1
format to a RAW Bayer 10-bit image with a resolution of 800 x 600.
- "csi_mux":2 [SGRBG10 800x600]: Sets up the csi_mux pad number 2
format to a RAW Bayer 10-bit image with a resolution of 800 x 600.
- "csi":0 [UYVY 800x600]: Sets up the csi pad number 0 format to a

## YUV4:2:2 image with a resolution of 800 x 600.
video_mux, csi, and mipi-csi-2 are all part of the SoC, so they are declared in the
vendor dtsi file (that is, arch/arm/boot/dts/imx7s.dtsi in the kernel sources).
video_mux is declared as follows:
gpr: iomuxc-gpr@30340000 {
[...]
video_mux: csi-mux {
compatible = "video-mux";
mux-controls = <&mux 0>;
#address-cells = <1>;
#size-cells = <0>;
status = "disabled";
port@0 {
reg = <0>;
```c
};
```
port@1 {
reg = <1>;
csi_mux_from_mipi_vc0: endpoint {
remote-endpoint = <&mipi_vc0_to_csi_mux>;
```c
};
};
```
port@2 {
408 Integrating with V4L2 Async and Media Controller Frameworks
reg = <2>;
csi_mux_to_csi: endpoint {
remote-endpoint = <&csi_from_csi_mux>;
```c
};
};
};
};
```
In the preceding code block, we have three ports, where ports 1 are 2 are connected to
remote endpoints. csi and mipi-csi-2 are declared as follows:
mipi_csi: mipi-csi@30750000 {
compatible = "fsl,imx7-mipi-csi2";
[...]
status = "disabled";
port@0 {
reg = <0>;
```c
};
```
port@1 {
reg = <1>;
mipi_vc0_to_csi_mux: endpoint {
remote-endpoint = <&csi_mux_from_mipi_vc0>;
```c
};
};
};
```
[...]
csi: csi@30710000 {
compatible = "fsl,imx7-csi"; [...]
status = "disabled";
port {
csi_from_csi_mux: endpoint {
remote-endpoint = <&csi_mux_to_csi>;

## The Linux media controller framework 409
```c
};
};
};
```
From the csi and mipi-csi-2 nodes, we can see how they are linked to their remote
ports in the video_mux node.

## Important note

## More information on video_mux binding can be found in

## Documentation/devicetree/bindings/media/video-mux.
txt in the kernel sources.
However, most of the vendor-declared nodes are disabled by default, and need to be
enabled from within the board file (the dts file, actually). This is what is done in the
following code block. Moreover, the camera sensor is part of the board, not the SoC.
So, it needs to be declared in the board dts file, which is arch/arm/boot/dts/
imx7s-warp.dts in kernel sources. The following is an excerpt:
&video_mux {
status = "okay";
```c
};
```
&mipi_csi {
clock-frequency = <166000000>;
fsl,csis-hs-settle = <3>;
status = "okay";
port@0 {
reg = <0>;
mipi_from_sensor: endpoint {
remote-endpoint = <&ov2680_to_mipi>;
data-lanes = <1>;
```c
};
};
};
```
410 Integrating with V4L2 Async and Media Controller Frameworks
&i2c2 {
[...]
status = "okay";
ov2680: camera@36 {
compatible = "ovti,ov2680";
[...]
port {
ov2680_to_mipi: endpoint {
remote-endpoint = <&mipi_from_sensor>;
clock-lanes = <0>;
data-lanes = <1>;
```c
};
};
};

```
## Important note

## More on i.MX7 entity binding can be found in both Documentation/
devicetree/bindings/media/imx7-csi.txt and
Documentation/devicetree/bindings/media/imx7-mipicsi2.txt in the kernel sources.
After this, the streaming can start. The v4l2-ctl tool can be used to select any of the
resolutions supported by the sensor:
root@imx7s-warp:~# media-ctl -p

## Media controller API version 4.17.0

## Media device information
------------------------
driver imx7-csi
model imx-media
serial
bus info
hw revision 0x0

## The Linux media controller framework 411
driver version 4.17.0

## Device topology
- entity 1: csi (2 pads, 2 links)
type V4L2 subdev subtype Unknown flags 0
device node name /dev/v4l-subdev0
pad0: Sink
[fmt:SBGGR10_1X10/800x600 field:none]
<- "csi-mux":2 [ENABLED]
pad1: Source
[fmt:SBGGR10_1X10/800x600 field:none]
-> "csi capture":0 [ENABLED]
- entity 4: csi capture (1 pad, 1 link)
type Node subtype V4L flags 0
device node name /dev/video0
pad0: Sink
<- "csi":1 [ENABLED]
- entity 10: csi-mux (3 pads, 2 links)
type V4L2 subdev subtype Unknown flags 0
device node name /dev/v4l-subdev1
pad0: Sink
[fmt:unknown/0x0]
pad1: Sink
[fmt:unknown/800x600 field:none]
<- "imx7-mipi-csis.0":1 [ENABLED]
pad2: Source
[fmt:unknown/800x600 field:none]
-> "csi":0 [ENABLED]
- entity 14: imx7-mipi-csis.0 (2 pads, 2 links)
type V4L2 subdev subtype Unknown flags 0
device node name /dev/v4l-subdev2
pad0: Sink
[fmt:SBGGR10_1X10/800x600 field:none]
<- "ov2680 1-0036":0 [ENABLED]
pad1: Source
[fmt:SBGGR10_1X10/800x600 field:none]
-> "csi-mux":1 [ENABLED]
- entity 17: ov2680 1-0036 (1 pad, 1 link)
type V4L2 subdev subtype Sensor flags 0
device node name /dev/v4l-subdev3
pad0: Source
[fmt:SBGGR10_1X10/800x600 field:none]
-> "imx7-mipi-csis.0":0 [ENABLED]
As data streams from left to right, we can interpret the preceding console logs as follows:
- -> "imx7-mipi-csis.0":0 [ENABLED]: This source pad feeds data to the
entity on its right, which is "imx7-mipi-csis.0":0.
- <- "ov2680 1-0036":0 [ENABLED]: This sink pad is fed by (that is, it
queries data from) the entity to its left, which is "ov2680 1-0036":0.
We are now done with all the aspects of the media controller framework. We started with
its architecture, then described the data structure it is made of, and then learned about its
API in detail. We ended with its use from user space in order to leverage the mode media
pipe.
## Summary
In this chapter, we went through the V4L2 asynchronous interface, which eases video
bridge and sub-device driver probing. This is useful for intrinsically asynchronous and
unordered device registration systems, such as flattened device tree driver probing.
Moreover, we dealt with the media controller framework, which allows leveraging V4L2
video pipelines. What we have seen so far lies in the kernel space.
In the next chapter, we will see how to deal with V4L2 devices from user space, thus
leveraging features exposed by their device drivers