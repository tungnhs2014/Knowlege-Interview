```bash
# Chapter 22 - Network Interface Card Drivers
```
We all know that networking is inherent to the Linux kernel. Some years ago, Linux was only used for its network performance, but things have changed now; Linux is much more than a server, and runs on billions of embedded devices. Over the years, Linux has gained the reputation of being the best network operating system. In spite of all this, Linux cannot do everything. Given the huge variety of Ethernet controllers that exist, Linux has found no other way than to expose an API to developers who need a writing driver for their network device, or who need to perform kernel networking development in a general manner. This
API offers a sufficient abstraction layer, guaranteeing the generosity of the code developed,
as well as porting on other architectures. This chapter will walk-through the part of this
API that deals with Network Interface Card (NIC) driver development, and discuss its data structures and methods.
In this chapter, we will cover the following topics:
NIC driver data structure and a walk through its main socket buffer structure
NIC driver architecture and methods description, as well as packet transmission and reception
Developing a dummy NIC driver for testing purposes
## Driver data structures
When you deal with NIC devices, there are two data structures that you need to play with:
The struct sk_buff structure, defined in include/linux/skbuff.h, which is the fundamental data structure in the Linux networking code, and which should be included in your code:
```c
#include <linux/skbuff.h>
```
Each packet sent or received is handled using this data structure.
The struct net_device structure; this is the structure by which any NIC
device is represented in the kernel. It is the interface by which data transit takes place. It is defined in include/linux/netdevice.h, which should also be included in your code:
```c
#include <linux/netdevice.h>
```
Other files that you should include in the code are include/linux/etherdevice.h for
MAC and Ethernet-related functions (such as alloc_etherdev()) and include/linux/ethtool.h for ethtool support:
```c
#include <linux/ethtool.h>
#include <linux/etherdevice.h>
```
## The socket buffer structure
This structure wraps any packet that transits through an NIC:
```c
struct sk_buff {
struct sk_buff * next;
struct sk_buff * prev;
```
ktime_t tstamp;
```c
struct rb_node rbnode; /* used in netem & tcp stack */
struct sock * sk;
struct net_device * dev;
unsigned int len;
unsigned int data_len;
```
__u16 mac_len;
__u16 hdr_len;
```c
unsigned int len;
unsigned int data_len;
```
__u16 mac_len;
__u16 hdr_len;
__u32 priority;
```c
dma_cookie_t dma_cookie;
```
sk_buff_data_t tail;
sk_buff_data_t end;
```c
unsigned char * head;
unsigned char * data;
unsigned int truesize;
```
atomic_t users;
```c
};
```
The following explain the elements in the structure:
next and prev : These represent the next and previous buffers in the list.
sk: This is the socket associated with this packet.
tstamp: This is the time when the packet arrived/left.
rbnode: This is an alternative to next/prev, represented in a red-black tree.
dev: This represents the device this packet arrived on/is leaving by. This field is associated with two other fields not listed here. These are input_dev and real_dev. They track devices associated with the packet. Therefore, input_dev always refers to a device the packet is received from.
len: This is the total number of bytes in the packet. Socket buffers (SKBs) are composed of a linear data buffer and, optionally, a set of one or more regions,
called rooms. If there are such rooms, data_len will hold the total number of bytes of the data area.
mac_len: This holds the length of the MAC header.
csum: This contains the checksum of the packet.
priority: This represents the packet priority in QoS.
truesize: This keeps track of how many bytes of system memory are consumed by a packet, including the memory occupied by the struct sk_buff structure itself.
users: This is used for reference counting for the SKB objects.
Head: Head, data, and tail are pointers to different regions (rooms) in the socket buffer.
end: This points to the end of the socket buffer.
Only a few elements of this structure have been discussed here. A full description is available in include/linux/skbuff.h., which is the header file you should include to deal with socket buffers.
## Socket buffer allocation
The allocation of a socket buffer is a bit tricky, since it needs at least three different functions:
First of all, the whole memory allocation should be done using the netdev_alloc_skb() function
Increase and align header room with the skb_reserve() function
Extend the used data area of the buffer (which will contain the packet) using the skb_put() function.
Let's have a look at the following diagram:
Socket buffers allocation process
1. We allocate a buffer large enough to contain a packet along with the Ethernet header by means of the netdev_alloc_skb() function:
```c
struct sk_buff *netdev_alloc_skb(struct net_device *dev,
unsigned int length)
```
This function returns NULL on failure. Therefore, even if it allocates memory,
netdev_alloc_skb() can be called from an atomic context.
Since the Ethernet header is 14 bytes long, it needs to have some alignment so that the CPU does not encounter any performance issues while accessing that part of the buffer. The appropriate name of the header_len parameter should be header_alignment, since this parameter is used for alignment. The usual value is 2, and it is the reason why the kernel defined a dedicated macro for this purpose, NET_IP_ALIGN, in include/linux/skbuff.h:
```c
#define NET_IP_ALIGN 2
```
2. The second step reserves aligned memory for the header by reducing the tail room. The function that does this is skb_reserve():
```c
void skb_reserve(struct sk_buff *skb, int len)
```
3. The last step consists of extending the used data area of the buffer so that it is as large as the packet size, by means of the skb_put() function. This function returns a pointer to the first byte of the data area:
```c
unsigned char *skb_put(struct sk_buff *skb, unsigned int len)
```
The allocated socket buffer should be forwarded to the kernel-networking layer.
This is the last step of the socket buffer's life cycle. One should use the netif_rx_ni() function for that:
```c
int netif_rx_ni(struct sk_buff *skb)
```
We will discuss how to use the preceding steps in the section of this chapter that deals with packet reception.
## Network interface structure
A network interface is represented in the kernel as an instance of the struct net_device structure, defined in include/linux/netdevice.h:
```c
struct net_device {
```
char name[IFNAMSIZ];
char *ifalias;
```c
unsigned long mem_end;
unsigned long mem_start;
unsigned long base_addr;
int irq;
```
netdev_features_t features;
netdev_features_t hw_features;
netdev_features_t wanted_features;
```c
int ifindex;
struct net_device_stats stats;
```
atomic_long_t rx_dropped;
atomic_long_t tx_dropped;
const struct net_device_ops *netdev_ops;
const struct ethtool_ops *ethtool_ops;
```c
unsigned int flags;
unsigned int priv_flags;
unsigned char link_mode;
unsigned char if_port;
unsigned char dma;
unsigned int mtu;
unsigned short type;
```
/* Interface address info. */
```c
unsigned char perm_addr[MAX_ADDR_LEN];
unsigned char addr_assign_type;
unsigned char addr_len;
unsigned short neigh_priv_len;
unsigned short dev_id;
unsigned short dev_port;
unsigned long last_rx;
```
/* Interface address info used in eth_type_trans() */
```c
unsigned char *dev_addr;
struct device dev;
struct phy_device *phydev;
};
```
The struct net_device structure belongs to the kernel data structures that need to be allocated dynamically, having their own allocation function. A NIC is allocated in the kernel by means of the alloc_etherdev() function:
```c
struct net_device *alloc_etherdev(int sizeof_priv);
```
The function returns NULL on failure. The sizeof_priv parameter represents the memory size to be allocated for a private data structure, attached to this NIC, and which can be extracted with the netdev_priv() function:
```c
void *netdev_priv(const struct net_device *dev)
```
Given the struct priv_struct structure, which is our private structure, the following is an implementation of how you allocate a network device along with the private data structure:
```c
struct net_device *net_dev;
struct priv_struct *priv_net_struct;
```
net_dev = alloc_etherdev(sizeof(struct priv_struct));
my_priv_struct = netdev_priv(dev);
Unused network devices should be freed with the free_netdev() function, which also frees memory allocated for private data. You should call this method only after the device has been unregistered from the kernel:
```c
void free_netdev(struct net_device *dev)
```
After your net_device structure has been completed and filled in, you should call register_netdev() on it. This function is explained later in this chapter, in the Driver methods section. Just keep in mind that this function registers our network device with the kernel, so that it can be used. That being said, you should make sure the device really can process network operations before calling this function:
```c
int register_netdev(struct net_device *dev)
```
## Device methods
Network devices fall into the category of devices not appearing in the /dev directory
(unlike block, input, or char devices). Therefore, like all of those kinds of devices, the NIC
```c
driver exposes a set of facilities in order to perform. The kernel exposes operations that can be performed on the network interfaces by means of the struct net_device_ops structure, which is a field of the struct net_device structure, representing the network device (dev->netdev_ops). The struct net_device_ops fields are described as follows:
struct net_device_ops {
int (*ndo_init)(struct net_device *dev);
void (*ndo_uninit)(struct net_device *dev);
int (*ndo_open)(struct net_device *dev);
int (*ndo_stop)(struct net_device *dev);
```
netdev_tx_t (*ndo_start_xmit) (struct sk_buff *skb,
```c
struct net_device *dev);
void (*ndo_change_rx_flags)(struct net_device *dev, int flags);
void (*ndo_set_rx_mode)(struct net_device *dev);
int (*ndo_set_mac_address)(struct net_device *dev, void *addr);
int (*ndo_validate_addr)(struct net_device *dev);
int (*ndo_do_ioctl)(struct net_device *dev,
struct ifreq *ifr, int cmd);
int (*ndo_set_config)(struct net_device *dev, struct ifmap *map);
int (*ndo_change_mtu)(struct net_device *dev, int new_mtu);
void (*ndo_tx_timeout) (struct net_device *dev);
struct net_device_stats* (*ndo_get_stats)(
struct net_device *dev);
};
```
Let's see what the meaning of each element in the structure is:
```c
int (*ndo_init)(struct net_device *dev) and void(*ndo_uninit)(struct net_device *dev): These are extra initialization/unitialization functions, respectively, executed when the driver calls register_netdev()/unregister_netdev() in order to register/unregister the network device with the kernel. Most drivers do not provide those functions,
```
since the real job is done by the ndo_open() and ndo_stop() functions.
```c
int (*ndo_open)(struct net_device *dev): This prepares and opens the interface. The interface is opened whenever the ip or ifconfig utilities activate it. In this method, the driver should request/map/register any system resource it needs (I/O ports, IRQ, DMA, and so on), turn on the hardware, and perform any other setup the device requires.
int (*ndo_stop)(struct net_device *dev):The kernel executes this function when the interface is brought down (for example, ifconfig <name>
```
down and so on). This function should perform reverse operations of what has been done in ndo_open().
```c
int (*ndo_start_xmit) (struct sk_buff *skb, struct net_device
```
*dev): This method is called whenever the kernel wants to send a packet through this interface.
```c
void (*ndo_set_rx_mode)(struct net_device *dev): This method is called to change the interface address list filter mode, multicast, or promiscuous.
```
It is recommended to provide this function.
```c
void (*ndo_tx_timeout)(struct net_device *dev): The kernel calls this method when a packet transmission fails to complete within a reasonable period,
usually for dev->watchdog ticks. The driver should check what happened,
```
handle the problem, and resume the packet transmission.
```c
struct net_device_stats *(*get_stats)(struct net_device *dev):
```
This method returns the device statistic. It is what one can see when netstat -i or ifconfig is run.
The preceding descriptions miss out a lot of elements. The complete structure description is available in the include/linux/netdevice.h file. Actually, only ndo_start_xmit is mandatory, but it is a good practice to provide as many helper hooks as your device has features.
## Opening and closing
The ndo_open() function is called by the kernel whenever this network interface is configured by authorized users (admin, for example) who make use of any user space utilities such as ifconfig or ip.
Like other network device operations, the ndo_open() function receives a struct net_device object as its parameter, from which the driver should get the device-specific object stored in the priv field at the time of allocating the net_device object.
The network controller usually raises an interrupt whenever it receives or completes a packet transmission. The driver needs to register an interrupt handler, which will be called whenever the controller raises an interrupt. The driver can register the interrupt handler either in the init()/probe() routine or in the open function. Some devices need the interrupt to be enabled by setting this in a special register in the hardware. In this case, one can request the interrupt in the probe function and just set/clear the enable bit in the open/close method.
Let's summarize what the open function should do:
1. Update the interface MAC address (in case the user changed it, and if your device allows this).
2. Reset the hardware if necessary, and take it out of the low-power mode.
3. Request any resources (I/O memory, DMA channels, IRQ).
4. Map IRQ and register interrupt handlers.
5. Check the interface link status.
6. Call netif_start_queue() on the device in order to let the kernel know that your device is ready to transmit packets.
An example of the open function is as follows:
/*
* This routine should set everything up new at each open, even
* registers that should only need to be set once at boot, so that
* there is non-reboot way to recover if something goes wrong.
*/
```c
static int enc28j60_net_open(struct net_device *dev)
{
struct priv_net_struct *priv = netdev_priv(dev);
if (!is_valid_ether_addr(dev->dev_addr)) {
```
[...] /* Maybe print a debug message ? */
```c
return -EADDRNOTAVAIL;
}
```
/*
* Reset the hardware here and take it out of low
* power mode
*/
```c
my_netdev_lowpower(priv, false);
if (!my_netdev_hw_init(priv)) {
```
[...] /* handle hardware reset failure */
```c
return -EINVAL;
}
```
/* Update the MAC address (in case user has changed it)
```c
* The new address is stored in netdev->dev_addr field
```
*/
set_hw_macaddr_registers(netdev, MAC_REGADDR_START,
```c
netdev->addr_len, netdev->dev_addr);
```
/* Enable interrupts */
```c
my_netdev_hw_enable(priv);
```
/* We are now ready to accept transmit requests from
* the queueing layer of the networking.
*/
```c
netif_start_queue(dev);
return 0;
}
netif_start_queue() simply allows upper layers to call the device ndo_start_xmit routine. In other words, it informs the kernel that the device is ready to handle transmit requests.
```
The closing method on the other side just has to do the reverse of the operations done when the device was opened:
/* The inverse routine to net_open(). */
```c
static int enc28j60_net_close(struct net_device *dev)
{
struct priv_net_struct *priv = netdev_priv(dev);
my_netdev_hw_disable(priv);
my_netdev_lowpower(priv, true);
```
/**
* netif_stop_queue - stop transmitted packets
*
* Stop upper layers calling the device ndo_start_xmit routine.
* Used for flow control when transmit resources are unavailable.
*/
```c
netif_stop_queue(dev);
return 0;
}
netif_stop_queue() simply does the reverse of netif_start_queue(), telling the kernel to stop calling the device ndo_start_xmit routine. We can't handle transmit requests anymore.
```
## Packet handling
Packet handling consists of the transmission and reception of packets. This is the main task of any network interface driver. Transmission refers only to sending outgoing frames,
whereas reception refers to frames coming in.
There are two ways to drive networking data exchange: by polling or by interrupting.
Polling, which is a kind of timer-driven interrupt, consists of a kernel continuously checking at given intervals for any change to the device. On the other hand, the interrupt mode consists of the kernel doing nothing, listening to an IRQ line, and waiting for the device to notify a change, by means of the IRQ. Interrupt-driven data exchange can increase system overhead during times of high traffic; that is why some drivers mix the two methods. The part of the kernel that allows the mixing of the two methods is called New
API (NAPI), which consists of using polling during times of high traffic and using interrupt
IRQ-driven management when traffic becomes normal. New drivers should use NAPI if the hardware can support it. However, NAPI is not discussed in this chapter, which will focus on an interrupt-driven method.
## Packet reception
```c
When a packet arrives at the network interface card, the driver must build a new socket buffer around it and copy the packet into the sk_buff->data field. The kind of copy does not really matter, and DMA can be used too. The driver is generally aware of new data arriving by means of interrupts. When the NIC receives a packet, it raises an interrupt,
```
which will be handled by the driver, which has to check the interrupt status register of the device and check the real reason why the interrupt was raised (it could be RX ok, RX error,
and so on). Bit(s) that correspond to the event that raised the interrupt will be set in the status register.
The tricky part will be allocating and building the socket buffer. But, fortunately, we already discussed that in the first section of this chapter. So let's not waste time and let's jump to a sample RX handler. The driver has to perform as many sk_buff allocations as the number of packets it receives:
/*
* RX handler
* This function is called in the work responsible of packet
* reception (bottom half) handler. We use work because access to
* our device (which sit on a SPI bus) may sleep
*/
```c
static int my_rx_interrupt(struct net_device *ndev)
{
struct priv_net_struct *priv = netdev_priv(ndev);
int pk_counter, ret;
```
/* Let's get the number of packet our device received */
pk_counter = my_device_reg_read(priv, REG_PKT_CNT);
```c
if (pk_counter > priv->max_pk_counter) {
```
/* update statistics */
```c
priv->max_pk_counter = pk_counter;
}
```
ret = pk_counter;
/* set receive buffer start */
```c
priv->next_pk_ptr = KNOWN_START_REGISTER;
while (pk_counter-- > 0)
```
/*
* By calling this internal helper function in a "while"
* loop, packets get extracted one by one from the device
* and forwarder to the network layer.
*/
```c
my_hw_rx(ndev);
return ret;
}
```
The following helper is responsible for getting one packet from the device, forwarding it to the kernel network, and decrementing the packet counter:
/*
* Hardware receive function.
* Read the buffer memory, update the FIFO pointer to
* free the buffer.
* This function decrements the packet counter.
*/
```c
static void my_hw_rx(struct net_device *ndev)
{
struct priv_net_struct *priv = netdev_priv(ndev);
struct sk_buff *skb = NULL;
```
u16 erxrdpt, next_packet, rxstat;
u8 rsv[RSV_SIZE];
```c
int packet_len;
```
packet_len = my_device_read_current_packet_size();
/* Can't cross boundaries */
```c
if ((priv->next_pk_ptr > RXEND_INIT)) {
```
/* packet address corrupted: reset RX logic */
[...]
/* Update RX errors stats */
```c
ndev->stats.rx_errors++;
```
return;
```c
}
```
/* Read next packet pointer and rx status vector
* This is device-specific
*/
```c
my_device_reg_read(priv, priv->next_pk_ptr, sizeof(rsv), rsv);
```
/* Check for errors in the device RX status reg,
* and update error stats accordingly
*/
if(an_error_is_detected_in_device_status_registers())
/* Depending on the error,
* stats.rx_errors++;
```c
* ndev->stats.rx_crc_errors++;
* ndev->stats.rx_frame_errors++;
* ndev->stats.rx_over_errors++;
```
*/
```c
} else {
```
skb = netdev_alloc_skb(ndev, len + NET_IP_ALIGN);
```c
if (!skb) {
ndev->stats.rx_dropped++;
} else {
skb_reserve(skb, NET_IP_ALIGN);
```
/*
* copy the packet from the device' receive buffer
* to the socket buffer data memory.
* Remember skb_put() return a pointer to the
* beginning of data region.
*/
my_netdev_mem_read(priv,
```c
rx_packet_start(priv->next_pk_ptr),
```
len, skb_put(skb, len));
/* Set the packet's protocol ID */
```c
skb->protocol = eth_type_trans(skb, ndev);
```
/* update RX statistics */
```c
ndev->stats.rx_packets++;
ndev->stats.rx_bytes += len;
```
/* Submit socket buffer to the network layer */
```c
netif_rx_ni(skb);
}
}
```
/* Move the RX read pointer to the start of the next
* received packet.
*/
```c
priv->next_pk_ptr = my_netdev_update_reg_next_pkt();
}
```
Of course, the only reason we call the RX handler from within a deferred job is because we sit on an SPI bus. All of the preceding operations could be performed from within the hwriq in the case of an MMIO device. Have a look at the NXP FEC driver in drivers/net/ethernet/freescale/fec.c to see how this is achieved.
## Packet transmission
When the kernel needs to send packets out of the interface, it calls the driver's ndo_start_xmit method, which should return NETDEV_TX_OK on success, or
NETDEV_TX_BUSY on failure, and in this case you can't do anything to the socket buffer since it is still owned by the network queuing layer when the error is returned. This means you cannot modify any SKB fields, or free the SKB, and so on. This function is protected from the concurrent call by a spinlock.
```c
Packet transmission is done asynchronously in most cases. The sk_buff of the transmitted packet is filled by the upper layers. Its data field contains packets to be sent. Drivers should extract a packet from sk_buff->data and write it into the device hardware FIFO,
```
or put it into a temporary TX buffer (if the device needs a certain amount of data before sending it) before writing it into the device hardware FIFO. Data is really only sent once the
FIFO reaches a threshold value (usually defined by the driver, or provided in a device datasheet) or when the driver intentionally starts the transmission, by setting a bit (a kind of trigger) in a special register of the device. That being said, the driver needs to inform the kernel not to start any transmissions until the hardware is ready to accept new data. This notification is done by means of the netif_stop_queue() function:
```c
void netif_stop_queue(struct net_device *dev)
```
After sending the packet, the network interface card will raise an interrupt. The interrupt handler should check why the interrupt has occurred. In the case of transmission interrupt,
```c
it should update its statistics (net_device->stats.tx_errors and net_device->stats.tx_packets), and notify the kernel that the device is free to send new packets. This notification is done by means of netif_wake_queue():
void netif_wake_queue(struct net_device *dev)
```
To summarize, packet transmission is split into two parts:
The ndo_start_xmit operation, which notifies the kernel that the device is busy, sets up everything, and starts the transfer
The TX interrupt handler, which updates TX statistics and notifies the kernel that the device is available again
The ndo_start_xmit function must roughly contain the following steps:
1. Call netif_stop_queue() on the network device in order to inform the kernel that the device will be busy in the data transmission.
```c
2. Write sk_buff->data content into the device FIFO.
```
3. Trigger the transmission (instruct the device to start the transmission).
Operations (2) and (3) may lead to devices sitting on slow buses sleeping
(SPI, for example) and may need to be deferred to the work structure. This is the case for our sample.
Once the packet is transferred, the TX interrupt handler should perform the following steps:
4. Depending on the device being memory mapped or whether it is sitting on a bus whose access functions may sleep, the following operations should be performed directly in the hwirq handler or scheduled in a job (or threaded IRQ):
1. Check whether the interrupt is a transmission interrupt
2. Read the transmission descriptor status register and see what the status of the packet is
3. Increment error statistics if there are any problems in the transmission
4. Increment statistics of successfully transmitted packets
5. Start the transmission queue, allowing the kernel to call the driver's ndo_start_xmit method again, by means of the netif_wake_queue()
function.
Let's summarize all this in a short sample code snippet:
/* Somewhere in the code */
```c
INIT_WORK(&priv->tx_work, my_netdev_hw_tx);
static netdev_tx_t my_netdev_start_xmit(struct sk_buff *skb,
struct net_device *dev)
{
struct priv_net_struct *priv = netdev_priv(dev);
```
/* Notify the kernel our device will be busy */
```c
netif_stop_queue(dev);
```
/* Remember the skb for deferred processing */
```c
priv->tx_skb = skb;
/* This work will copy data from sk_buffer->data to
```
* the hardware's FIFO and start transmission
*/
```c
schedule_work(&priv->tx_work);
```
/* Everything is OK */
```c
return NETDEV_TX_OK;
}
```
The work is described below:
/*
* Hardware transmit function.
* Fill the buffer memory and send the contents of the
* transmit buffer onto the network
*/
```c
static void my_netdev_hw_tx(struct priv_net_struct *priv)
{
```
/* Write packet to hardware device TX buffer memory */
```c
my_netdev_packet_write(priv, priv->tx_skb->len,
priv->tx_skb->data);
```
/*
* does this network device support write-verify?
* Perform it
*/
[...];
/* set TX request flag,
* so that the hardware can perform transmission.
* This is device-specific
*/
```c
my_netdev_reg_bitset(priv, ECON1, ECON1_TXRTS);
}
```
TX interrupt management will be discussed in the next section.
## Driver example
We can summarize the concepts discussed previously in the following fake Ethernet driver:
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/of.h> /* For DT*/
#include <linux/platform_device.h> /* For platform devices */
struct eth_struct {
int bar;
int foo;
struct net_device *dummy_ndev;
};
static int fake_eth_open(struct net_device *dev) {
printk("fake_eth_open called\n");
```
/* We are now ready to accept transmit requests from
* the queueing layer of the networking.
*/
```c
netif_start_queue(dev);
return 0;
}
static int fake_eth_release(struct net_device *dev) {
pr_info("fake_eth_release called\n");
netif_stop_queue(dev);
return 0;
}
static int fake_eth_xmit(struct sk_buff *skb, struct net_device *ndev) {
pr_info("dummy xmit called...\n");
ndev->stats.tx_bytes += skb->len;
ndev->stats.tx_packets++;
skb_tx_timestamp(skb);
dev_kfree_skb(skb);
return NETDEV_TX_OK;
}
static int fake_eth_init(struct net_device *dev)
{
pr_info("fake eth device initialized\n");
return 0;
};
static const struct net_device_ops my_netdev_ops = {
```
.ndo_init = fake_eth_init,
.ndo_open = fake_eth_open,
.ndo_stop = fake_eth_release,
.ndo_start_xmit = fake_eth_xmit,
.ndo_validate_addr = eth_validate_addr,
```c
};
static const struct of_device_id fake_eth_dt_ids[] = {
dts
{ .compatible = "packt,fake-eth", },
c
{ /* sentinel */ }
};
static int fake_eth_probe(struct platform_device *pdev)
{
int ret;
struct eth_struct *priv;
struct net_device *dummy_ndev;
priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
if (!priv)
return -ENOMEM;
```
dummy_ndev = alloc_etherdev(sizeof(struct eth_struct));
```c
dummy_ndev->if_port = IF_PORT_10BASET;
dummy_ndev->netdev_ops = &my_netdev_ops;
/* If needed, dev->ethtool_ops = &fake_ethtool_ops; */
```
ret = register_netdev(dummy_ndev);
```c
if(ret) {
pr_info("dummy net dev: Error %d initializing card ...", ret);
return ret;
}
priv->dummy_ndev = dummy_ndev;
platform_set_drvdata(pdev, priv);
return 0;
}
static int fake_eth_remove(struct platform_device *pdev)
{
struct eth_struct *priv;
```
priv = platform_get_drvdata(pdev);
```c
pr_info("Cleaning Up the Module\n");
unregister_netdev(priv->dummy_ndev);
free_netdev(priv->dummy_ndev);
return 0;
}
static struct platform_driver mypdrv = {
```
.probe = fake_eth_probe,
.remove = fake_eth_remove,
```c
.driver = {
```
.name = "fake-eth",
.of_match_table = of_match_ptr(fake_eth_dt_ids),
.owner = THIS_MODULE,
```c
},
};
module_platform_driver(mypdrv);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_DESCRIPTION("Fake Ethernet driver");
```
Once the module is loaded and a device has been matched, an Ethernet interface will be created on the system. First, let's see what the dmesg command shows us:
```bash
# dmesg
```
[...]
[146698.060074] fake eth device initialized
[146698.087297] IPv6: ADDRCONF(NETDEV_UP): eth0: link is not ready
If you run the ifconfig -a command, the interface will be printed on the screen:
```bash
# ifconfig -a
```
[...]
eth0 Link encap:Ethernet HWaddr 00:00:00:00:00:00
BROADCAST MULTICAST MTU:1500 Metric:1
RX packets:0 errors:0 dropped:0 overruns:0 frame:0
TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
collisions:0 txqueuelen:1000
RX bytes:0 (0.0 B) TX bytes:0 (0.0 B)
You can finally configure the interface, assigning an IP address, so that it can be shown by using ifconfig:
```bash
# ifconfig eth0 192.168.1.45
# ifconfig
```
[...]
eth0 Link encap:Ethernet HWaddr 00:00:00:00:00:00
inet addr:192.168.1.45 Bcast:192.168.1.255 Mask:255.255.255.0
BROADCAST MULTICAST MTU:1500 Metric:1
RX packets:0 errors:0 dropped:0 overruns:0 frame:0
TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
collisions:0 txqueuelen:1000
RX bytes:0 (0.0 B) TX bytes:0 (0.0 B)
## Status and control
Device control refers to a situation where the kernel needs to change properties of the interface on its own initiative, or in response to a user action. It can then use either operations exposed through the struct net_device_ops structure, as discussed, or use another control tool, ethtool, which requires the driver to introduce a new set of hooks,
which we will discuss in the next section. Conversely, the status reports the state of the interface.
## The interrupt handler
So far, we have only dealt with two different interrupts: when a new packet has arrived and when the transmission of an outgoing packet is complete. But nowadays hardware interfaces are becoming smart, and are able to report their status either for sanity purposes,
or for data transfer purposes. This way, network interfaces can also generate interrupts to signal errors, link status changes, and so on. They should all be handled in the interrupt handler.
This is what our hwrirq handler looks like:
```c
static irqreturn_t my_netdev_irq(int irq, void *dev_id)
{
struct priv_net_struct *priv = dev_id;
```
/*
* Can't do anything in interrupt context because we need to
* block (spi_sync() is blocking) so fire of the interrupt
* handling workqueue.
* Remember, we access our netdev registers through SPI bus
* via spi_sync() call.
*/
```c
schedule_work(&priv->irq_work);
return IRQ_HANDLED;
}
```
Because our device sits on an SPI bus, everything is deferred into a work_struct, which is defined as follows:
```c
static void my_netdev_irq_work_handler(struct work_struct *work)
{
struct priv_net_struct *priv =
container_of(work, struct priv_net_struct, irq_work);
struct net_device *ndev = priv->netdev;
int intflags, loop;
```
/* disable further interrupts */
```c
my_netdev_reg_bitclear(priv, EIE, EIE_INTIE);
do {
```
loop = 0;
intflags = my_netdev_regb_read(priv, EIR);
/* DMA interrupt handler (not currently used) */
```c
if ((intflags & EIR_DMAIF) != 0) {
```
loop++;
```c
handle_dma_complete();
clear_dma_interrupt_flag();
}
```
/* LINK changed handler */
```c
if ((intflags & EIR_LINKIF) != 0) {
```
loop++;
```c
my_netdev_check_link_status(ndev);
clear_link_interrupt_flag();
}
```
/* TX complete handler */
```c
if ((intflags & EIR_TXIF) != 0) {
```
bool err = false;
loop++;
```c
priv->tx_retry_count = 0;
if (locked_regb_read(priv, ESTAT) & ESTAT_TXABRT)
clear_tx_interrupt_flag();
```
/* TX Error handler */
```c
if ((intflags & EIR_TXERIF) != 0) {
```
loop++;
/*
* Reset TX logic by setting/clearing appropriate
* bit in the right register
*/
[...]
/* Transmit Late collision check for retransmit */
```c
if (my_netdev_cpllision_bit_set())
```
/* Handlecollision */
[...]
```c
}
```
/* RX Error handler */
```c
if ((intflags & EIR_RXERIF) != 0) {
```
loop++;
/* Check free FIFO space to flag RX overrun */
[...]
```c
}
```
/* RX handler */
```c
if (my_rx_interrupt(ndev))
```
loop++;
```c
} while (loop);
```
/* re-enable interrupts */
```c
my_netdev_reg_bitset(priv, EIE, EIE_INTIE);
}
```
## Ethtool support
Ethtool is a small utility for examining and tuning the settings of Ethernet-based network interfaces. With ethtool, it is possible to control various parameters, such as:
Speed
Media type
Duplex operation
Getting/setting EEPROM register content
Hardware check summing
Wake-on-LAN
Drivers that need support from ethtool should include <linux/ethtool.h>. It relies on the struct ethtool_ops structure, which is the core of this feature, and contains a set of methods for ethtool operation support. Most of these methods are relatively straightforward; see include/linux/ethtool.h for details.
For ethtool support to fully be part of the driver, the driver should fill in an ethtool_ops structure and assign it to the .ethtool_ops field of the struct net_device structure:
```c
my_netdev->ethtool_ops = &my_ethtool_ops;
```
The macro SET_ETHTOOL_OPS can be used for this purpose too. Do note that your ethtool methods can be called even when the interface is down.
For example, the following drivers implement ethtool support:
drivers/net/ethernet/microchip/enc28j60.c drivers/net/ethernet/freescale/fec.c drivers/net/usb/rtl8150.c
## Driver methods
Driver methods are the probe() and remove() functions. They are responsible for registering and unregistering the network device with the kernel. The driver has to provide its functionalities to the kernel through the device methods by means of the struct net_device structure. These are the operations that can be performed on the network interface:
```c
static const struct net_device_ops my_netdev_ops = {
```
.ndo_open = my_netdev_open,
.ndo_stop = my_netdev_close,
.ndo_start_xmit = my_netdev_start_xmit,
.ndo_set_rx_mode = my_netdev_set_multicast_list,
.ndo_set_mac_address = my_netdev_set_mac_address,
.ndo_tx_timeout = my_netdev_tx_timeout,
.ndo_change_mtu = eth_change_mtu,
.ndo_validate_addr = eth_validate_addr,
```c
};
```
The preceding operations are the operations that most drivers implement.
## The probe function
The probe function is quite basic, and only needs to perform a device's early init, and then register our network device with the kernel.
In other words, the probe function has to:
1. Allocate the network device along with its private data using the alloc_etherdev() function (helped by netdev_priv()).
2. Initialize private data fields (mutexes, spinlock, work_queue, and so on). You should use work queues (and mutexes) if the device sits on a bus whose access functions may sleep (SPI, for example). In this case, the hwirq just has to acknowledge the kernel code, and schedule the job that will perform operations on the device. An alternative solution is to use threaded IRQs. If the device is
MMIO, you can use spinlock to protect critical sections and get rid of work queues.
3. Initialize bus-specific parameters and functionalities (SPI, USB, PCI, and so on).
4. Request and map resources (I/O memory, DMA channel, and IRQ).
5. If necessary, generate a random MAC address and assign it to the device.
6. Fill in the mandatory (or useful) netdev properties: if_port, irq, netdev_ops,
ethtool_ops, and so on.
7. Put the device into a low-power state (the open() function will remove it from this mode).
8. Finally, call register_netdev() on the device.
With an SPI network device, the probe function will look like this:
```c
static int my_netdev_probe(struct spi_device *spi)
{
struct net_device *dev;
struct priv_net_struct *priv;
int ret = 0;
```
/* Allocate network interface */
dev = alloc_etherdev(sizeof(struct priv_net_struct));
```c
if (!dev)
```
[...] /* handle -ENOMEM error */
/* Private data */
priv = netdev_priv(dev);
/* set private data and bus-specific parameter */
[...]
/* Initialize some works */
```c
INIT_WORK(&priv->tx_work, data_tx_work_handler);
```
[...]
/* Devicerealy init, only few things */
```c
if (!my_netdev_chipset_init(dev))
```
[...] /* handle -EIO error */
/* Generate and assign random MAC address to the device */
```c
eth_hw_addr_random(dev);
my_netdev_set_hw_macaddr(dev);
```
/* Board setup must set the relevant edge trigger type;
* level triggers won't currently work.
*/
```c
ret = request_irq(spi->irq, my_netdev_irq, 0, DRV_NAME, priv);
if (ret < 0)
```
[...]; /* Handle irq request failure */
/* Fill some netdev mandatory or useful properties */
```c
dev->if_port = IF_PORT_10BASET;
dev->irq = spi->irq;
dev->netdev_ops = &my_netdev_ops;
dev->ethtool_ops = &my_ethtool_ops;
```
/* Put device into sleep mode */
```c
My_netdev_lowpower(priv, true);
```
/* Register our device with the kernel */
```c
if (register_netdev(dev))
```
[...]; /* Handle registration failure error */
```c
dev_info(&dev->dev, DRV_NAME " driver registered\n");
return 0;
}
```
This whole chapter has been heavily inspired by the enc28j60 from
Microchip. You can have a look at its code in drivers/net/ethernet/microchip/enc28j60.c.
The register_netdev() function takes a completed struct net_device object and adds it to the kernel interfaces; 0 is returned on success and a negative error code is returned on failure. The struct net_device object should be stored in your bus device structure so that it can be accessed later. That being said, if your net device is part of a global private structure, it is that structure that you should register.
Do note that a duplicate device name may lead to registration failure.
## Module unloading
This is a cleaning function, which relies on two functions. Our driver release function should look like this:
```c
static int my_netdev_remove(struct spi_device *spi)
{
struct priv_net_struct *priv = spi_get_drvdata(spi);
unregister_netdev(priv->netdev);
free_irq(spi->irq, priv);
free_netdev(priv->netdev);
return 0;
}
The unregister_netdev() function removes the interface from the system, and the kernel can no longer call its methods; free_netdev() frees the memory used by the struct net_device structure itself along with the memory allocated for private data, as well as any internally allocated memory related to the network device. Do note that you should never free netdev->priv by yourself.
```
## Summary
This chapter has explained everything needed to write an NIC device driver. Even though the chapter relies on a network interface sitting on an SPI bus, the principle is the same for
USB or PCI network interfaces. You can also use the dummy driver provided for testing purposes. After reading this chapter, NIC drivers should no longer be a mystery to you