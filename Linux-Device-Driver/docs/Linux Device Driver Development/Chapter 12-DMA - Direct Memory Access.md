```bash
# Chapter 12 - DMA - Direct Memory Access
```
## Setting up DMA mappings
For any type of DMA transfer, one needs to provide source and destination addresses, as well as the number of words to transfer. In the case of a peripheral DMA, the peripheral's
FIFO serves as either the source or the destination. When the peripheral serves as the source, a memory location (internal or external) serves as the destination address. When the peripheral serves as the destination, a memory location (internal or external) serves as the source address.
With a peripheral DMA, we specify either the source or the destination, depending on the direction of the transfer. In others words, a DMA transfer requires suitable memory mappings. This is what we will discuss in the following sections.
## Cache coherency and DMA
As discussed in Chapter 11, Kernel Memory Management, copies of recently accessed memory areas are stored in the cache. This applies to DMA memory too. The reality is that memory shared between two independent devices is generally the source of cache coherency problems. Cache incoherence is an issue coming from the fact that other devices may not be aware of an update from a writing device. On the other hand, cache coherency ensures that every write operation appears to occur instantaneously, so that all devices sharing the same memory region see exactly the same sequence of changes.
A well-explained coherency issue scenario is illustrated in the following excerpt from
LDD3:
Let's imagine a CPU equipped with a cache and an external memory that can be accessed directly by devices using DMA. When the CPU accesses location X in the memory, the current value will be stored in the cache. Subsequent operations on X will update the cached copy of X, but not the external memory version of X, assuming a write-back cache.
If the cache is not flushed to the memory before the next time a device tries to access X, the device will receive a stale value of X. Similarly, if the cached copy of X is not invalidated when a device writes a new value to the memory, then the CPU will operate on a stale value of X.
There are actually two ways to address this issue:
A hardware-based solution. Such systems are coherent systems.
A software-based solution, where the OS is responsible for ensuring cache coherency. One calls such systems non-coherent systems.
## DMA mappings
Any suitable DMA transfer requires suitable memory mapping. A DMA mapping consists of allocating a DMA buffer and generating a bus address for it. Devices actually use bus addresses. Bus addresses are each instance of the dma_addr_t type.
One distinguishes two types of mapping: coherent DMA mappings and streaming DMA
mappings. One can use the former over several transfers, which automatically addresses cache coherency issues. Therefore, it is too expensive. Streaming mapping has a lot of constraints and does not automatically address coherency issues, although, there is a solution for that, which consists of several function calls between each transfer. Coherent mapping usually exists for the life of the driver, whereas a streaming mapping is usually unmapped once the DMA transfer completes.
One should use streaming mapping when one can and coherent mapping when one must.
Back to the code; the main header should include the following to handle DMA mapping:
```c
#include <linux/dma-mapping.h>
```
## Coherent mapping
The following function sets up a coherent mapping:
```c
void *dma_alloc_coherent(struct device *dev, size_t size,
dma_addr_t *dma_handle, gfp_t flag)
```
This function handles both the allocation and the mapping of the buffer, and returns a kernel virtual address for that buffer, which is size bytes-wide and accessible by the CPU.
dev is your device structure. The third argument is an output parameter that points to the associated bus address. Memory allocated for the mapping is guaranteed to be physically contiguous, and flag determines how memory should be allocated, which is usually by GFP_KERNEL, or GFP_ATOMIC (if we are in an atomic context).
Do note that this mapping is said to be:
Consistent (coherent), since it allocates uncached unbuffered memory for a device to perform DMA
Synchronous, because a write by either the device or the CPU can be immediately read by either without worrying about cache coherency
In order to free a mapping, one can use the following function:
```c
void dma_free_coherent(struct device *dev, size_t size,
void *cpu_addr, dma_addr_t dma_handle);
```
Here, cpu_addr corresponds to the kernel virtual address returned by dma_alloc_coherent(). This mapping is expensive, and the minimum it can allocate is a page. In fact, it only allocates the number of pages that is the power of 2. The order of pages is obtained with int order = get_order(size). One should use this mapping for buffers that last the life of the device.
## Streaming DMA mapping
Streaming mapping has more constraints, and is different from coherent mapping for the following reasons:
Mappings need to work with a buffer that has already been allocated.
Mappings may accept several non-contiguous and scattered buffers.
A mapped buffer belongs to the device and not to the CPU anymore. Before the
CPU can use the buffer, it should be unmapped first (after dma_unmap_single() or dma_unmap_sg()). This is for caching purposes.
For write transactions (CPU to device), the driver should place data in the buffer before the mapping.
The direction the data should move has to be specified, and the data should only be used based on this direction.
One may wonder why one should not access the buffer until it is unmapped. The reason is simple: CPU mapping is cacheable. dma_map_*() family functions, which are used for streaming mapping, will first clean/invalidate the caches related to the buffer and rely on the CPU not to access it until the corresponding dma_unmap_*(). That will then invalidate
(if necessary) the caches again, in case of any speculative fetches in the meantime, before the CPU can read any data written to memory by the device. Now the CPU can access the buffer.
There are actually two forms of streaming mapping:
Single buffer mapping, which allow only one-page mapping
Scatter/gather mapping, which allows passing several buffers (scattered over memory)
For either mapping, the direction should be specified by a symbol of type enum dma_data_direction, defined in include/linux/dma-direction.h:
```c
enum dma_data_direction {
DMA_BIDIRECTIONAL = 0,
DMA_TO_DEVICE = 1,
DMA_FROM_DEVICE = 2,
DMA_NONE = 3,
};
```
## Single-buffer mapping
This is for occasional mapping. One can set up a single buffer with this:
```c
dma_addr_t dma_map_single(struct device *dev, void *ptr,
```
size_t size, enum dma_data_direction direction);
The direction should be DMA_TO_DEVICE, DMA_FROM_DEVICE, or DMA_BIDIRECTIONAL, as described in the preceding code. ptr is the kernel virtual address of the buffer, and dma_addr_t is the returned bus address for the device. Make sure to use the direction that really fits your needs, not just always DMA_BIDIRECTIONAL.
One should free the mapping with this:
```c
void dma_unmap_single(struct device *dev, dma_addr_t dma_addr,
```
size_t size, enum dma_data_direction direction);
## Scatter/gather mapping
Scatter/gather mappings are a special type of streaming DMA mapping where one can transfer several buffer regions in a single shot, instead of mapping each buffer individually and transferring them one by one. Suppose you have several buffers that might not be physically contiguous, all of which need to be transferred at the same time to or from the device. This situation may occur due to:
A readv or writev system call
A disk I/O request
Or, simply a list of pages in a mapped kernel I/O buffer
The kernel represents the scatterlist as a coherent structure, struct scatterlist:
```c
struct scatterlist {
unsigned long page_link;
unsigned int offset;
unsigned int length;
dma_addr_t dma_address;
unsigned int dma_length;
};
```
In order to set up a scatterlist mapping, one should:
Allocate your scattered buffers.
Create an array of the scatter list and fill it with allocated memory using sg_set_buf(). Note that scatterlist entries must be of page size (except ends).
Call dma_map_sg() on the scatterlist.
Once done with DMA, call dma_unmap_sg() to unmap the scatterlist.
While one can send the contents of several buffers over DMA one at a time by individually mapping each of them, scatter/gather can send them all at once by sending the pointer to the scatterlist to the device, along with a length, which is the number of entries in the list:
```c
u32 *wbuf, *wbuf2, *wbuf3;
```
wbuf = kzalloc(SDMA_BUF_SIZE, GFP_DMA);
wbuf2 = kzalloc(SDMA_BUF_SIZE, GFP_DMA);
wbuf3 = kzalloc(SDMA_BUF_SIZE/2, GFP_DMA);
```c
struct scatterlist sg[3];
sg_init_table(sg, 3);
sg_set_buf(&sg[0], wbuf, SDMA_BUF_SIZE);
sg_set_buf(&sg[1], wbuf2, SDMA_BUF_SIZE);
sg_set_buf(&sg[2], wbuf3, SDMA_BUF_SIZE/2);
```
ret = dma_map_sg(NULL, sg, 3, DMA_MEM_TO_MEM);
The same rules described in the Single-buffer mapping section apply to scatter/gather mapping:
DMA scatter/gather dma_map_sg() and dma_unmap_sg() take care of cache coherency. But if one needs to use the same mapping to access (read/write) the data between the DMA transfer, the buffers must be synced between each transfer in an appropriate manner, by either dma_sync_sg_for_cpu(), if the CPU needs to access the buffers, or dma_sync_sg_for_device() if it is the device. Similar functions for single-region mapping are dma_sync_single_for_cpu() and dma_sync_single_for_device():
```c
void dma_sync_sg_for_cpu(struct device *dev,
struct scatterlist *sg,
int nents,
enum dma_data_direction direction);
void dma_sync_sg_for_device(struct device *dev,
struct scatterlist *sg, int nents,
enum dma_data_direction direction);
void dma_sync_single_for_cpu(struct device *dev, dma_addr_t addr,
```
size_t size,
```c
enum dma_data_direction dir)
void dma_sync_single_for_device(struct device *dev,
dma_addr_t addr, size_t size,
enum dma_data_direction dir)
```
There is no need to call the preceding functions again after the buffer(s) has been unmapped. You can just read the content.
## The concept of completion
This section will briefly describe completion and the necessary part of its API that the DMA
transfer uses. For a complete description, please feel free to have a look at the kernel documentation at Documentation/scheduler/completion.txt. A common pattern in kernel programming involves initiating some activity outside the current thread, then waiting for that activity to complete.
Completion is a good alternative to sleep() when waiting for a buffer to be used. It is suitable for sensing data, which is exactly what the DMA callback does.
Working with completion requires this header:
<linux/completion.h>
Like other kernel facility data structures, one can create instances of the struct completion structure either statically or dynamically:
Static declaration and initialization look like this:
```c
DECLARE_COMPLETION(my_comp);
```
Dynamic allocation looks like this:
```c
struct completion my_comp;
init_completion(&my_comp);
```
When the driver begins some work whose completion must be waited for (a DMA
transaction in our case), it just has to pass the completion event to the wait_for_completion() function:
```c
void wait_for_completion(struct completion *comp);
```
When some other part of the code has decided that the completion has happened
(transaction completes), it can wake up anybody (actually the code that needs to access the
DMA buffer) who is waiting with one of the following:
```c
void complete(struct completion *comp);
void complete_all(struct completion *comp);
```
As one can guess, complete() will wake up only one waiting process, while complete_all() will wake up every process waiting for that event. Completions are implemented in such a way that they will work properly even if complete() is called before wait_for_completion().
By means of code samples used in the next sections, we will have a better understanding of how this works.
## The DMA engine API
The DMA engine is a generic kernel framework for developing a DMA controller driver.
The main goal of DMA is offloading the CPU when it comes to copy memory. One delegates a transaction (I/O data transfers) to the DMA engine by the use of channels. A
DMA engine, through its driver/API, exposes a set of channels that can be used by other devices (slaves). The DMA Engine layout is shown in the following diagram:
DMA Engine layout
Here, we will simply walk through the (slave) API, which is applicable for slave DMA
usage only. The mandatory header here is as follows:
```c
#include <linux/dmaengine.h>
```
Using the slave DMA is straightforward, and consists of the following steps:
1. Allocate a DMA slave channel.
2. Set slave- and controller-specific parameters.
3. Get a descriptor for the transaction.
4. Submit the transaction.
5. Issue pending requests and wait for callback notification.
One can see a DMA channel as a highway for I/O data transfer.
## Allocating a DMA slave channel
One requests a channel using dma_request_channel(). Its prototype is as follows:
```c
struct dma_chan *dma_request_channel(const dma_cap_mask_t *mask,
dma_filter_fn fn, void *fn_param);
```
mask is a bitmap mask that represents the capabilities the channel must satisfy. One uses it essentially to specify the transfer types the driver needs to perform:
```c
enum dma_transaction_type {
DMA_MEMCPY, /* Memory to memory copy */
DMA_XOR, /* Memory to memory XOR*/
DMA_PQ, /* Memory to memory P+Q computation */
DMA_XOR_VAL, /* Memory buffer parity check using XOR */
DMA_PQ_VAL, /* Memory buffer parity check using P+Q */
DMA_INTERRUPT, /* The device is able to generate dummy transfer that will generate interrupts */
DMA_SG, /* Memory to memory scatter gather */
DMA_PRIVATE, /* channels are not to be used for global memcpy.
```
Usually used with DMA_SLAVE */
```c
DMA_SLAVE, /* Memory to device transfers */
DMA_CYCLIC, /* Device is able to handle cyclic transfers */
DMA_INTERLEAVE, /* Memory to memory interleaved transfer */
}
```
The dma_cap_zero() and dma_cap_set() functions are used to clear the mask and set the capability we need. For example:
```c
dma_cap_mask my_dma_cap_mask;
struct dma_chan *chan;
dma_cap_zero(my_dma_cap_mask);
dma_cap_set(DMA_MEMCPY, my_dma_cap_mask); /* Memory to memory copy */
```
chan = dma_request_channel(my_dma_cap_mask, NULL, NULL);
In the preceding excerpt, dma_filter_fn is defined as follows:
typedef bool (*dma_filter_fn)(struct dma_chan *chan,
```c
void *filter_param);
```
If the filter_fn parameter (which is optional) is NULL, dma_request_channel() will simply return the first channel that satisfies the capability mask. Otherwise, when the mask parameter is insufficient for specifying the necessary channel, one can use the filter_fn routine as a filter for the available channels in the system. The kernel calls the filter_fn routine once for each free channel in the system. Upon seeing a suitable channel,
filter_fn should return DMA_ACK, which will tag the given channel to be the return value from dma_request_channel().
A channel allocated through this interface is exclusive to the caller, until dma_release_channel() is called:
```c
void dma_release_channel(struct dma_chan *chan)
```
## Setting slave- and controller-specific parameters
This step introduces a new data structure, struct dma_slave_config, which represents the runtime configuration for the DMA slave channel. This allows clients to specify settings,
such as the DMA direction, DMA addresses, bus width, DMA burst lengths, and so on, for the peripheral.
```c
int dmaengine_slave_config(struct dma_chan *chan,
struct dma_slave_config *config)
```
The struct dma_slave_config structure looks like this:
/*
* Please refer to the complete description in
* include/linux/dmaengine.h
*/
```c
struct dma_slave_config {
enum dma_transfer_direction direction;
```
phys_addr_t src_addr;
phys_addr_t dst_addr;
```c
enum dma_slave_buswidth src_addr_width;
enum dma_slave_buswidth dst_addr_width;
u32 src_maxburst;
u32 dst_maxburst;
```
[...]
```c
};
```
The following is the meaning of each element in the structure:
direction: This indicates whether the data should go in or out on this slave channel, right now. The possible values are:
/* dma transfer mode and direction indicator */
```c
enum dma_transfer_direction {
DMA_MEM_TO_MEM, /* Async/Memcpy mode */
DMA_MEM_TO_DEV, /* From Memory to Device */
DMA_DEV_TO_MEM, /* From Device to Memory */
DMA_DEV_TO_DEV, /* From Device to Device */
```
[...]
```c
};
```
src_addr: This is the physical address (actually the bus address) of the buffer where the DMA slave data should be read (RX). This element is ignored if the source is memory. dst_addr is the physical address (actually the bus address) of the buffer where the DMA slave data should be written (TX) and is ignored if the source is memory.
src_addr_width: This is the width in bytes of the source (RX) register where the
DMA data should be read. If the source is memory, this may be ignored depending on the architecture. Legal values are 1, 2, 4, or 8. Therefore,
dst_addr_width is the same as src_addr_width but for the destination target
(TX).
Any bus width must be one of the following enumerations:
```c
enum dma_slave_buswidth {
DMA_SLAVE_BUSWIDTH_UNDEFINED = 0,
DMA_SLAVE_BUSWIDTH_1_BYTE = 1,
DMA_SLAVE_BUSWIDTH_2_BYTES = 2,
DMA_SLAVE_BUSWIDTH_3_BYTES = 3,
DMA_SLAVE_BUSWIDTH_4_BYTES = 4,
DMA_SLAVE_BUSWIDTH_8_BYTES = 8,
DMA_SLAVE_BUSWIDTH_16_BYTES = 16,
DMA_SLAVE_BUSWIDTH_32_BYTES = 32,
DMA_SLAVE_BUSWIDTH_64_BYTES = 64,
};
```
src_maxburst: This is the maximum number of words (here, consider words as units of the src_addr_width member, not in bytes) that can be sent in one burst to the device, typically, something like half the FIFO depth on I/O peripherals so you do not overflow it. This may or may not be applicable on memory sources.
dst_maxburst is the same as src_maxburst but for the destination target.
For example:
```c
struct dma_chan *my_dma_chan;
dma_addr_t dma_src, dma_dst;
struct dma_slave_config my_dma_cfg = {0};
```
/* No filter callback, neither filter param */
my_dma_chan = dma_request_channel(my_dma_cap_mask, 0, NULL);
/* scr_addr and dst_addr are ignored in this structure for mem to mem copy
*/
my_dma_cfg.direction = DMA_MEM_TO_MEM;
my_dma_cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_32_BYTES;
```c
dmaengine_slave_config(my_dma_chan, &my_dma_cfg);
```
char *rx_data, *tx_data;
/* No error check */
rx_data = kzalloc(BUFFER_SIZE, GFP_DMA);
tx_data = kzalloc(BUFFER_SIZE, GFP_DMA);
```c
feed_data(tx_data);
```
/* get dma addresses */
```c
dma_src_addr = dma_map_single(NULL, tx_data,
```
BUFFER_SIZE, DMA_MEM_TO_MEM);
```c
dma_dst_addr = dma_map_single(NULL, rx_data,
```
BUFFER_SIZE, DMA_MEM_TO_MEM);
In the preceding excerpt, one calls the dma_request_channel() function in order to take the ownership of the DMA channel, on which one calls dmaengine_slave_config() to apply its configuration. dma_map_single() is called in order to map the rx and tx buffers, so that these can be used for DMA.
## Getting a descriptor for transaction
If you remember the first step of this section, when one requests a DMA channel, the return value is an instance of the struct dma_chan structure. If one looks at its definition in include/linux/dmaengine.h, one will notice that it contains a struct dma_device
*device field, which represents the DMA device (the controller actually) that supplied the channel. The kernel driver of this controller is responsible (this is a rule imposed by the kernel API for DMA controller drivers) for exposing a set of functions to prepare DMA
transactions, where each of them correspond to a DMA transaction type (enumerated in step 1). Depending on the transaction type, one has no choice but to choose the dedicated function. Some of these functions are:
device_prep_dma_memcpy(): Prepares a memcpy operation device_prep_dma_sg(): Prepare a scatter/gather memcpy operation device_prep_dma_xor(): For a xor operation device_prep_dma_xor_val(): Prepares a xor validation operation device_prep_dma_pq(): Prepares a pq operation device_prep_dma_pq_val(): Prepares a pqzero_sum operation device_prep_dma_memset(): Prepares a memset operation device_prep_dma_memset_sg(): For a memset operation over a scatterlist device_prep_slave_sg(): Prepares a slave DMA operation device_prep_interleaved_dma(): Transfers an expression in a generic way
Let's have a look at drivers/dma/imx-sdma.c, which is the i.MX6 DMA controller
(SDMA) driver. Each of these functions returns a pointer to a struct dma_async_tx_descriptor structure, which corresponds to the transaction descriptor.
With memory-to-memory copy, one will use device_prep_dma_memcpy:
```c
struct dma_device *dma_dev = my_dma_chan->device;
struct dma_async_tx_descriptor *tx = NULL;
tx = dma_dev->device_prep_dma_memcpy(my_dma_chan, dma_dst_addr,
dma_src_addr, BUFFER_SIZE, 0);
if (!tx) {
printk(KERN_ERR "%s: Failed to prepare DMA transfer\n",
```
__FUNCTION__);
/* dma_unmap_* the buffer */
```c
}
```
In fact, we should have used the dmaengine_prep_* DMA engine API. Just note that these functions internally do what we just performed earlier. For example, for memory-tomemory, one could have used the dmaengine_prep_dma_memcpy () function:
```c
static inline struct dma_async_tx_descriptor *dmaengine_prep_dma_memcpy(
struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
```
size_t len, unsigned long flags)
Our sample becomes the following:
```c
struct dma_async_tx_descriptor *tx = NULL;
```
tx = dmaengine_prep_dma_memcpy(my_dma_chan, dma_dst_addr,
```c
dma_src_addr, BUFFER_SIZE, 0);
if (!tx) {
printk(KERN_ERR "%s: Failed to prepare DMA transfer\n",
```
__FUNCTION__);
/* dma_unmap_* the buffer */
```c
}
```
Please have a look at include/linux/dmaengine.h, in the definition of a struct dma_device structure, to see how all of these hooks are implemented.
## Submitting the transaction
To put the transaction in the driver pending queue, one uses dmaengine_submit(). Once the descriptor has been prepared and the callback information added, one should place it on the DMA engine driver pending queue:
```c
dma_cookie_t dmaengine_submit(struct dma_async_tx_descriptor *desc)
```
This function returns a cookie that one can use to check the progress of DMA activity through other DMA engines. dmaengine_submit() will not start the DMA operation, it merely adds it to the pending queue. How to start the transaction is discussed in the next step:
```c
struct completion transfer_ok;
init_completion(&transfer_ok);
tx->callback = my_dma_callback;
```
/* Submit our dma transfer */
```c
dma_cookie_t cookie = dmaengine_submit(tx);
if (dma_submit_error(cookie)) {
printk(KERN_ERR "%s: Failed to start DMA transfer\n", __FUNCTION__);
```
/* Handle that */
[...]
```c
}
```
Issuing pending DMA requests and waiting for callback notification
Starting the transaction is the last step in the DMA transfer setup. One activates transactions in the pending queue of a channel by calling dma_async_issue_pending()
on that channel. If the channel is idle, then the first transaction in the queue is started and subsequent ones are queued up. On completion of a DMA operation, the next one in the queue is started and a tasklet triggered. This tasklet is in charge of calling the client driver completion callback routine for notification, if set:
```c
void dma_async_issue_pending(struct dma_chan *chan);
```
An example would look like this:
```c
dma_async_issue_pending(my_dma_chan);
wait_for_completion(&transfer_ok);
dma_unmap_single(my_dma_chan->device->dev, dma_src_addr,
```
BUFFER_SIZE, DMA_MEM_TO_MEM);
```c
dma_unmap_single(my_dma_chan->device->dev, dma_src_addr,
```
BUFFER_SIZE, DMA_MEM_TO_MEM);
/* Process buffer through rx_data and tx_data virtualaddresses. */
The wait_for_completion() function will block until our DMA callback gets called,
which will update (complete) our completion variable in order to resume the previously blocked code. It is a suitable alternative to while (!done) msleep(SOME_TIME);.
```c
static void my_dma_callback()
{
complete(transfer_ok);
```
return;
```c
}
```
The DMA engine API function that actually issues pending transactions is dmaengine_issue_pending(struct dma_chan *chan), which is a wrap around dma_async_issue_pending().
## Putting it all together – NXP SDMA (i.MX6)
The SDMA engine is a programmable controller in i.MX6, and each peripheral has its own copy function in this controller. One uses this enum to determine their addresses:
```c
enum sdma_peripheral_type {
```
IMX_DMATYPE_SSI, /* MCU domain SSI */
IMX_DMATYPE_SSI_SP, /* Shared SSI */
IMX_DMATYPE_MMC, /* MMC */
IMX_DMATYPE_SDHC, /* SDHC */
IMX_DMATYPE_UART, /* MCU domain UART */
IMX_DMATYPE_UART_SP, /* Shared UART */
IMX_DMATYPE_FIRI, /* FIRI */
IMX_DMATYPE_CSPI, /* MCU domain CSPI */
IMX_DMATYPE_CSPI_SP, /* Shared CSPI */
IMX_DMATYPE_SIM, /* SIM */
IMX_DMATYPE_ATA, /* ATA */
IMX_DMATYPE_CCM, /* CCM */
IMX_DMATYPE_EXT, /* External peripheral */
IMX_DMATYPE_MSHC, /* Memory Stick Host Controller */
IMX_DMATYPE_MSHC_SP, /* Shared Memory Stick Host Controller */
IMX_DMATYPE_DSP, /* DSP */
IMX_DMATYPE_MEMORY, /* Memory */
IMX_DMATYPE_FIFO_MEMORY,/* FIFO type Memory */
IMX_DMATYPE_SPDIF, /* SPDIF */
IMX_DMATYPE_IPU_MEMORY, /* IPU Memory */
IMX_DMATYPE_ASRC, /* ASRC */
IMX_DMATYPE_ESAI, /* ESAI */
IMX_DMATYPE_SSI_DUAL, /* SSI Dual FIFO */
IMX_DMATYPE_ASRC_SP, /* Shared ASRC */
IMX_DMATYPE_SAI, /* SAI */
```c
};
```
Despite the generic DMA engine API, any constructor may provide its own custom data structure. This is the case for the imx_dma_data structure, which is private data (used to describe the DMA device type one needs to use) that is to be passed to the .private field of the struct dma_chan in the filter callback:
```c
struct imx_dma_data {
int dma_request; /* DMA request line */
int dma_request2; /* secondary DMA request line */
enum sdma_peripheral_type peripheral_type;
int priority;
};
enum imx_dma_prio {
DMA_PRIO_HIGH = 0,
DMA_PRIO_MEDIUM = 1,
DMA_PRIO_LOW = 2
};
```
These structures and enum are all specific to i.MX, and are defined in include/linux/platform_data/dma-imx.h. Now, let's write our kernel DMA module.
It allocates two buffers (source and destination). Fill the source with predefined data, and perform a transaction in order to copy src into dst. One can improve this module by using data coming from the user space (copy_from_user()). This driver is inspired by the one provided in the imx-test package:
```c
#include <linux/module.h>
#include <linux/slab.h> /* for kmalloc */
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/version.h>
```
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,35))
```c
#include <linux/platform_data/dma-imx.h>
```
#else
```c
#include <mach/dma.h>
```
#endif
```c
#include <linux/dmaengine.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/delay.h>
static int gMajor; /* major number of device */
static struct class *dma_tm_class;
u32 *wbuf; /* source buffer */
u32 *rbuf; /* destination buffer */
struct dma_chan *dma_m2m_chan; /* our dma channel */
struct completion dma_m2m_ok; /* completion variable used in the DMA
```
callback */
```c
#define SDMA_BUF_SIZE 1024
```
Let's define the filter function. When one requests a DMA channel, the controller driver may perform a lookup in a list of channels (which it has). For fine-grained lookup, one can provide a callback method that will be called on each channel found. It is then up to the callback to choose a suitable channel to use:
```c
static bool dma_m2m_filter(struct dma_chan *chan, void *param)
{
if (!imx_dma_is_general_purpose(chan))
return false;
chan->private = param;
return true;
}
```
imx_dma_is_general_purpose is a special function that checks the controller driver's name. The open function will allocate the buffer and request the DMA channel, given our filter function as callback:
```c
int sdma_open(struct inode * inode, struct file * filp)
{
dma_cap_mask_t dma_m2m_mask;
struct imx_dma_data m2m_dma_data = {0};
init_completion(&dma_m2m_ok);
dma_cap_zero(dma_m2m_mask);
dma_cap_set(DMA_MEMCPY, dma_m2m_mask); /* Set channel capacities */
```
m2m_dma_data.peripheral_type = IMX_DMATYPE_MEMORY; /* choose the dma device type. This is proper to i.MX */
m2m_dma_data.priority = DMA_PRIO_HIGH; /* we need high priority */
```c
dma_m2m_chan = dma_request_channel(dma_m2m_mask, dma_m2m_filter,
```
&m2m_dma_data);
```c
if (!dma_m2m_chan) {
printk("Error opening the SDMA memory to memory channel\n");
return -EINVAL;
}
```
wbuf = kzalloc(SDMA_BUF_SIZE, GFP_DMA);
```c
if(!wbuf) {
printk("error wbuf !!!!!!!!!!!\n");
return -1;
}
```
rbuf = kzalloc(SDMA_BUF_SIZE, GFP_DMA);
```c
if(!rbuf) {
printk("error rbuf !!!!!!!!!!!\n");
return -1;
}
return 0;
}
```
The release function simply does the reverse of the open function; it frees the buffer and releases the DMA channel:
```c
int sdma_release(struct inode * inode, struct file * filp)
{
dma_release_channel(dma_m2m_chan);
dma_m2m_chan = NULL;
kfree(wbuf);
kfree(rbuf);
return 0;
}
```
In the read function, we just compare the source and destination buffer and inform the user about the result.
ssize_t sdma_read (struct file *filp, char __user * buf,
size_t count, loff_t * offset)
```c
{
int i;
for (i=0; i<SDMA_BUF_SIZE/4; i++) {
if (*(rbuf+i) != *(wbuf+i)) {
printk("Single DMA buffer copy falled!,r=%x,w=%x,%d\n",
```
*(rbuf+i), *(wbuf+i), i);
```c
return 0;
}
}
printk("buffer copy passed!\n");
return 0;
}
```
We use completion in order to get notified (woken up) when the transaction has terminated. This callback is called after our transaction has finished and sets our completion variable to the complete state:
```c
static void dma_m2m_callback(void *data)
{
printk("in %s\n",__func__);
complete(&dma_m2m_ok);
return ;
}
```
In the write function, we fill our source buffer with the data, perform DMA mapping in order to get physical addresses that correspond to our source and destination buffer, and call device_prep_dma_memcpy to get a transaction descriptor. That transaction descriptor is then submitted to the DMA engine with dmaengine_submit, which does not perform our transaction yet. It is only after we have called dma_async_issue_pending on our
DMA channel that our pending transaction will be processed:
ssize_t sdma_write(struct file * filp, const char __user * buf,
size_t count, loff_t * offset)
```c
{
u32 i;
struct dma_slave_config dma_m2m_config = {0};
struct dma_async_tx_descriptor *dma_m2m_desc; /* transaction descriptor
```
*/
```c
dma_addr_t dma_src, dma_dst;
```
/* No copy_from_user, we just fill the source buffer with predefined data */
```c
for (i=0; i<SDMA_BUF_SIZE/4; i++) {
```
*(wbuf + i) = 0x56565656;
```c
}
dma_m2m_config.direction = DMA_MEM_TO_MEM;
dma_m2m_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
dmaengine_slave_config(dma_m2m_chan, &dma_m2m_config);
dma_src = dma_map_single(NULL, wbuf, SDMA_BUF_SIZE, DMA_TO_DEVICE);
dma_dst = dma_map_single(NULL, rbuf, SDMA_BUF_SIZE, DMA_FROM_DEVICE);
dma_m2m_desc =
dma_m2m_chan->device->device_prep_dma_memcpy(dma_m2m_chan, dma_dst,
dma_src, SDMA_BUF_SIZE,0);
if (!dma_m2m_desc)
printk("prep error!!\n");
dma_m2m_desc->callback = dma_m2m_callback;
dmaengine_submit(dma_m2m_desc);
dma_async_issue_pending(dma_m2m_chan);
wait_for_completion(&dma_m2m_ok);
dma_unmap_single(NULL, dma_src, SDMA_BUF_SIZE, DMA_TO_DEVICE);
dma_unmap_single(NULL, dma_dst, SDMA_BUF_SIZE, DMA_FROM_DEVICE);
return 0;
}
struct file_operations dma_fops = {
```
open: sdma_open,
release: sdma_release,
read: sdma_read,
write: sdma_write,
```c
};
```
The full code is available in the book's repository chapter-12/imx-sdma/imx-sdmasingle.c. There is also a module with which to perform the same task, but using scatter/gather mapping: chapter-12/imx-sdma/imx-sdma-scatter-gather.c.
## DMA DT binding
DT binding for the DMA channel depends on the DMA controller node, which is SoCdependent, and some parameters (such as DMA cells) may vary from one SoC to another.
This example only focuses on the i.MX SDMA controller, which one can find in the kernel source, at Documentation/devicetree/bindings/dma/fsl-imx-sdma.txt.
## Consumer binding
According to the SDMA event-mapping table, the following code shows DMA request signals for peripherals in i.MX 6Dual/ 6Quad:
```c
uart1: serial@02020000 {
dts
compatible = "fsl,imx6sx-uart", "fsl,imx21-uart";
reg = <0x02020000 0x4000>;
interrupts = <GIC_SPI 26 IRQ_TYPE_LEVEL_HIGH>;
clocks = <&clks IMX6SX_CLK_UART_IPG>,
```
<&clks IMX6SX_CLK_UART_SERIAL>;
```dts
clock-names = "ipg", "per";
dmas = <&sdma 25 4 0>, <&sdma 26 4 0>;
dma-names = "rx", "tx";
status = "disabled";
c
};
```
The second cells (25 and 26) in the DMA property correspond to the DMA request/event
ID. Those values come from the SoC manuals (i.MX53 in our case). Please have a look at https://community.nxp.com/servlet/JiveServlet/download/614186-1-373516/iMX6_
Firmware_Guide.pdf, and the Linux reference manual at https://community.nxp.com/servlet/JiveServlet/download/614186-1-373515/i.MX_Lin ux_Reference_Manual.pdf.
The third cell indicates the priority to use. The driver code to request a specified parameter is defined next. One can find the complete code in drivers/tty/serial/imx.c in the kernel source tree:
```c
static int imx_uart_dma_init(struct imx_port *sport)
{
struct dma_slave_config slave_config = {};
struct device *dev = sport->port.dev;
int ret;
```
/* Prepare for RX : */
```c
sport->dma_chan_rx = dma_request_slave_channel(dev, "rx");
if (!sport->dma_chan_rx) {
```
[...] /* cannot get the DMA channel. handle error */
```c
}
```
slave_config.direction = DMA_DEV_TO_MEM;
```c
slave_config.src_addr = sport->port.mapbase + URXD0;
```
slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
/* one byte less than the watermark level to enable the aging timer */
slave_config.src_maxburst = RXTL_DMA - 1;
```c
ret = dmaengine_slave_config(sport->dma_chan_rx, &slave_config);
if (ret) {
```
[...] /* handle error */
```c
}
sport->rx_buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
if (!sport->rx_buf) {
```
[...] /* handle error */
```c
}
```
/* Prepare for TX : */
```c
sport->dma_chan_tx = dma_request_slave_channel(dev, "tx");
if (!sport->dma_chan_tx) {
```
[...] /* cannot get the DMA channel. handle error */
```c
}
```
slave_config.direction = DMA_MEM_TO_DEV;
```c
slave_config.dst_addr = sport->port.mapbase + URTX0;
```
slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
slave_config.dst_maxburst = TXTL_DMA;
```c
ret = dmaengine_slave_config(sport->dma_chan_tx, &slave_config);
if (ret) {
```
[...] /* handle error */
```c
}
```
[...]
```c
}
```
The magic call here is the dma_request_slave_channel() function, which will parse the device node (in the DT) using of_dma_request_slave_channel() to gather channel settings, according to the DMA name (refer to the named resource in Chapter 6, The
Concept of a Device Tree).
## Summary
DMA is a feature that one finds in many modern CPUs. This chapter introduced the necessary steps to get the most out of this device, using the kernel DMA mapping and
DMA engine APIs. After this chapter, I have no doubt you will be able to set up at least a memory-to-memory DMA transfer. You can find further information at
Documentation/dmaengine/, in the kernel source tree. The next chapter deals with an entirely different subject—the Linux device model