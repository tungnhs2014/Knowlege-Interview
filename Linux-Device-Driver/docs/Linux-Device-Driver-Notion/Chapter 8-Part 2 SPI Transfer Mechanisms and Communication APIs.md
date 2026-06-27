# Part 2. SPI Transfer Mechanisms and Communication APIs

This part covers the core SPI communication APIs, including `spi_transfer`, `spi_message`, synchronous/asynchronous transfers, and helper functions with complete examples.

---

## 8.1 SPI I/O Model Overview

### 8.1.1 Message-Based Communication

**SPI uses a queued message model:**

```
┌────────────────────────────────────────────────┐
│  SPI I/O Model - Queued Messages               │
├────────────────────────────────────────────────┤
│                                                │
│  Driver submits:                               │
│  ┌──────────────────────────────────────┐      │
│  │ struct spi_message                   │      │
│  │  ├── Transfer 1 (struct spi_transfer)│      │
│  │  ├── Transfer 2 (struct spi_transfer)│      │
│  │  └── Transfer 3 (struct spi_transfer)│      │
│  └──────────────────────────────────────┘      │
│            ↓                                   │
│  ┌──────────────────────────────────────┐      │
│  │ SPI Core Queue Manager               │      │
│  └──────────────────────────────────────┘      │
│            ↓                                   │
│  ┌──────────────────────────────────────┐      │
│  │ SPI Controller Driver                │      │
│  │ Processes transfers atomically       │      │
│  └──────────────────────────────────────┘      │
│            ↓                                   │
│  ┌──────────────────────────────────────┐      │
│  │ Hardware SPI Bus                     │      │
│  │ MOSI/MISO/SCK/CS physical transfer   │      │
│  └──────────────────────────────────────┘      │
│                                                │
└────────────────────────────────────────────────┘
```

**Key concepts:**

- **Message** = One or more transfers to the same device
- **Transfer** = A single full-duplex SPI transaction
- **Atomic** = All transfers in a message execute without interruption
- **Queued** = Multiple messages can be queued

---

## 8.2 struct spi_transfer - Single Transfer

### 8.2.1 The spi_transfer Structure

**Definition from `include/linux/spi/spi.h`:**

```c
struct spi_transfer {
    /* Buffers */
    const void *tx_buf;        /* Data to transmit */
    void *rx_buf;              /* Buffer for received data */
    unsigned len;              /* Buffer length in bytes */

    /* DMA addresses (if using DMA) */
    dma_addr_t tx_dma;
    dma_addr_t rx_dma;

    /* Transfer parameters */
    u32 speed_hz;              /* Override device speed */
    u16 delay_usecs;           /* Delay after this transfer */
    u8 bits_per_word;          /* Override device word size */

    /* Control flags */
    unsigned cs_change:1;      /* CS behavior after transfer */
    unsigned tx_nbits:3;       /* Number of bits for TX */
    unsigned rx_nbits:3;       /* Number of bits for RX */

#define SPI_NBITS_SINGLE  0x01  /* 1 bit transfer (normal) */
#define SPI_NBITS_DUAL    0x02  /* 2 bits transfer */
#define SPI_NBITS_QUAD    0x04  /* 4 bits transfer */
};
```

### 8.2.2 Field Descriptions

```
┌──────────────────────────────────────────────────┐
│  struct spi_transfer Fields                      │
├──────────────────────────────────────────────────┤
│                                                  │
│  tx_buf          ← Data to send (or NULL)        │
│                    Must be DMA-safe if using DMA │
│                                                  │
│  rx_buf          ← Receive buffer (or NULL)      │
│                    Must be DMA-safe if using DMA │
│                                                  │
│  len             ← Size in bytes (same for both) │
│                    TX and RX must be equal       │
│                                                  │
│  speed_hz        ← Override device max_speed_hz  │
│                    Use 0 for device default      │
│                                                  │
│  bits_per_word   ← Override device bits_per_word │
│                    Use 0 for device default      │
│                                                  │
│  delay_usecs     ← Delay after transfer (µs)     │
│                    Before optional CS change     │
│                                                  │
│  cs_change       ← CS behavior:                  │
│                    0 = keep CS asserted          │
│                    1 = toggle CS after transfer  │
│                                                  │
└──────────────────────────────────────────────────┘
```

### 8.2.3 Buffer Rules

**Important buffer constraints:**

```c
/* RULE 1: Full-duplex means same size */
transfer.len = 4;           /* Both buffers are 4 bytes */
transfer.tx_buf = tx_data;  /* Send these 4 bytes */
transfer.rx_buf = rx_data;  /* Receive 4 bytes simultaneously */

/* RULE 2: Write-only - set rx_buf = NULL */
transfer.tx_buf = command;
transfer.rx_buf = NULL;     /* Don't care about received data */
transfer.len = 2;

/* RULE 3: Read-only - set tx_buf = NULL */
transfer.tx_buf = NULL;     /* Send dummy bytes (usually 0xFF) */
transfer.rx_buf = data;
transfer.len = 8;

/* RULE 4: Buffers must be DMA-safe if using DMA */
/* Use kmalloc(), not stack variables! */
u8 *tx_buf = kmalloc(64, GFP_KERNEL | GFP_DMA);  // Good
u8 tx_buf[64];  // BAD - stack not DMA-safe!
```

### 8.2.4 Transfer Examples

**Example 1: Full-duplex transfer**

```c
struct spi_transfer xfer;
u8 tx[4] = {0x12, 0x34, 0x56, 0x78};
u8 rx[4];

memset(&xfer, 0, sizeof(xfer));
xfer.tx_buf = tx;
xfer.rx_buf = rx;
xfer.len = 4;
xfer.speed_hz = 1000000;  /* 1 MHz for this transfer */
xfer.bits_per_word = 8;

/* After transfer: rx[] contains received data */
```

**Example 2: Write-only (command)**

```c
struct spi_transfer xfer = {
    .tx_buf = (u8[]){0x9F},  /* Read ID command */
    .len = 1,
    .cs_change = 1,          /* Deassert CS after */
};
```

**Example 3: Read-only**

```c
struct spi_transfer xfer;
u8 rx_data[16];

memset(&xfer, 0, sizeof(xfer));
xfer.tx_buf = NULL;          /* Send 0x00 or 0xFF */
xfer.rx_buf = rx_data;
xfer.len = 16;
```

---

## 8.3 struct spi_message - Message Container

### 8.3.1 The spi_message Structure

**Definition:**

```c
struct spi_message {
    struct list_head transfers;    /* List of spi_transfer */
    struct spi_device *spi;        /* Target device */

    unsigned is_dma_mapped:1;      /* DMA mapping done? */

    /* Completion callback */
    void (*complete)(void *context);
    void *context;

    /* Status and statistics */
    unsigned frame_length;         /* Total bytes in message */
    unsigned actual_length;        /* Bytes actually transferred */
    int status;                    /* 0 on success, -errno otherwise */
};
```

### 8.3.2 Field Descriptions

```
┌──────────────────────────────────────────────────┐
│  struct spi_message Fields                       │
├──────────────────────────────────────────────────┤
│                                                  │
│  transfers        ← List of spi_transfer structs │
│                     Added via spi_message_add_tail│
│                                                  │
│  spi              ← Target spi_device            │
│                     Set automatically            │
│                                                  │
│  is_dma_mapped    ← 1 if driver handles DMA      │
│                     Usually 0 (core handles it)  │
│                                                  │
│  complete()       ← Callback when done           │
│                     Called in completion context │
│                                                  │
│  context          ← Callback parameter           │
│                                                  │
│  frame_length     ← Auto-calculated total bytes  │
│                                                  │
│  actual_length    ← Bytes successfully sent      │
│                                                  │
│  status           ← 0 = success, -errno = error  │
│                                                  │
└──────────────────────────────────────────────────┘
```

### 8.3.3 Message Lifecycle

```
┌────────────────────────────────────────────────┐
│  SPI Message Lifecycle                         │
├────────────────────────────────────────────────┤
│                                                │
│  1. Initialize message                         │
│     spi_message_init(&msg);                    │
│                                                │
│  2. Add transfers to message                   │
│     spi_message_add_tail(&xfer1, &msg);        │
│     spi_message_add_tail(&xfer2, &msg);        │
│                                                │
│  3. Submit message (sync or async)             │
│     spi_sync(spi, &msg);   // Blocking         │
│       OR                                       │
│     spi_async(spi, &msg);  // Non-blocking     │
│                                                │
│  4. Message processed atomically               │
│     - All transfers execute in FIFO order      │
│     - CS stays asserted (unless cs_change=1)   │
│     - No other device can interrupt            │
│                                                │
│  5. Completion                                 │
│     - Status updated                           │
│     - Callback called (if async)               │
│     - Caller woken up (if sync)                │
│                                                │
└────────────────────────────────────────────────┘
```

---

## 8.4 Synchronous Transfer - spi_sync()

### 8.4.1 The spi_sync() Function

**Prototype:**

```c
int spi_sync(struct spi_device *spi, struct spi_message *message);
```

**Characteristics:**

- **Blocking**: Sleeps until transfer completes
- **Cannot use in IRQ context**: May sleep
- **No callback needed**: Function returns when done
- **Wrapper around `spi_async()`**: Adds wait queue

**Return value:**

- `0` on success
- Negative errno on error

### 8.4.2 Single Transfer Example

```c
static int my_simple_transfer(struct spi_device *spi)
{
    struct spi_message msg;
    struct spi_transfer xfer;
    u8 tx_buf[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    u8 rx_buf[5] = {0};
    int ret;

    /* Initialize transfer */
    memset(&xfer, 0, sizeof(xfer));
    xfer.tx_buf = tx_buf;
    xfer.rx_buf = rx_buf;
    xfer.len = sizeof(tx_buf);
    xfer.bits_per_word = 8;

    /* Initialize message and add transfer */
    spi_message_init(&msg);
    spi_message_add_tail(&xfer, &msg);

    /* Execute synchronously */
    ret = spi_sync(spi, &msg);
    if (ret < 0) {
        dev_err(&spi->dev, "SPI transfer failed: %d\n", ret);
        return ret;
    }

    /* Process received data */
    dev_info(&spi->dev, "Received: %02x %02x %02x %02x %02x\n",
             rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4]);

    return 0;
}
```

### 8.4.3 Multi-Transfer Example

```c
static int my_multi_transfer(struct spi_device *spi)
{
    struct spi_message msg;
    struct spi_transfer xfer[3];
    u8 cmd_buf = 0x9F;         /* Read ID command */
    u8 addr_buf[2] = {0x02, 0xB5};
    u8 data_buf[10] = {0};
    int ret;

    /* Transfer 1: Send command */
    memset(&xfer[0], 0, sizeof(xfer[0]));
    xfer[0].tx_buf = &cmd_buf;
    xfer[0].len = 1;
    xfer[0].cs_change = 1;      /* Deassert CS briefly */
    xfer[0].delay_usecs = 50;   /* Wait 50µs */

    /* Transfer 2: Send address */
    memset(&xfer[1], 0, sizeof(xfer[1]));
    xfer[1].tx_buf = addr_buf;
    xfer[1].len = 2;
    xfer[1].cs_change = 0;      /* Keep CS asserted */

    /* Transfer 3: Read data */
    memset(&xfer[2], 0, sizeof(xfer[2]));
    xfer[2].rx_buf = data_buf;
    xfer[2].len = sizeof(data_buf);

    /* Build message */
    spi_message_init(&msg);
    spi_message_add_tail(&xfer[0], &msg);
    spi_message_add_tail(&xfer[1], &msg);
    spi_message_add_tail(&xfer[2], &msg);

    /* Execute all transfers atomically */
    ret = spi_sync(spi, &msg);
    if (ret < 0) {
        dev_err(&spi->dev, "Multi-transfer failed: %d\n", ret);
        return ret;
    }

    /* Check status */
    if (msg.status) {
        dev_err(&spi->dev, "Transfer error: %d\n", msg.status);
        return msg.status;
    }

    dev_info(&spi->dev, "Transferred %u bytes\n", msg.actual_length);

    return 0;
}
```

---

## 8.5 Asynchronous Transfer - spi_async()

### 8.5.1 The spi_async() Function

**Prototype:**

```c
int spi_async(struct spi_device *spi, struct spi_message *message);
```

**Characteristics:**

- **Non-blocking**: Returns immediately
- **Can use in IRQ context**: Does not sleep
- **Requires callback**: To know when done
- **Message queued**: Processed later

**Callback signature:**

```c
void complete_callback(void *context);
```

### 8.5.2 Async Transfer Example

```c
struct my_spi_data {
    struct spi_device *spi;
    struct spi_message msg;
    struct spi_transfer xfer;
    struct completion done;
    u8 tx_buf[16];
    u8 rx_buf[16];
};

/* Completion callback */
static void my_transfer_complete(void *context)
{
    struct my_spi_data *data = context;

    dev_info(&data->spi->dev, "Async transfer complete\n");
    dev_info(&data->spi->dev, "Status: %d\n", data->msg.status);
    dev_info(&data->spi->dev, "Actual length: %u\n",
             data->msg.actual_length);

    /* Signal completion */
    complete(&data->done);
}

static int my_async_transfer(struct spi_device *spi)
{
    struct my_spi_data *data;
    int ret;

    /* Allocate data */
    data = kzalloc(sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->spi = spi;
    init_completion(&data->done);

    /* Prepare data */
    memset(data->tx_buf, 0xAA, sizeof(data->tx_buf));

    /* Setup transfer */
    data->xfer.tx_buf = data->tx_buf;
    data->xfer.rx_buf = data->rx_buf;
    data->xfer.len = 16;

    /* Setup message */
    spi_message_init(&data->msg);
    data->msg.complete = my_transfer_complete;
    data->msg.context = data;
    spi_message_add_tail(&data->xfer, &data->msg);

    /* Submit asynchronously */
    ret = spi_async(spi, &data->msg);
    if (ret < 0) {
        dev_err(&spi->dev, "spi_async failed: %d\n", ret);
        kfree(data);
        return ret;
    }

    /* Wait for completion (or return immediately) */
    wait_for_completion(&data->done);

    /* Process result */
    if (data->msg.status == 0) {
        dev_info(&spi->dev, "Transfer successful\n");
    }

    kfree(data);
    return 0;
}
```

### 8.5.3 When to Use Async

**Use `spi_async()` when:**

- In interrupt context
- Need to avoid blocking
- Doing multiple concurrent operations
- Want to queue many messages

**Use `spi_sync()` when:**

- In process context (can sleep)
- Simple sequential operations
- Easier error handling

---

## 8.6 Helper Functions for Simple Transfers

### 8.6.1 Available Helper Functions

**The kernel provides simple wrappers:**

```c
/* Write data */
int spi_write(struct spi_device *spi,
              const void *buf, size_t len);

/* Read data */
int spi_read(struct spi_device *spi,
             void *buf, size_t len);

/* Write then read (two separate transfers) */
int spi_write_then_read(struct spi_device *spi,
                        const void *txbuf, unsigned n_tx,
                        void *rxbuf, unsigned n_rx);
```

**⚠️ Important:** These are for **small amounts of data** only!

### 8.6.2 spi_write() Example

```c
static int my_device_send_command(struct spi_device *spi, u8 cmd)
{
    int ret;

    ret = spi_write(spi, &cmd, 1);
    if (ret < 0) {
        dev_err(&spi->dev, "Failed to send command: %d\n", ret);
        return ret;
    }

    return 0;
}

static int my_device_write_data(struct spi_device *spi,
                                 const u8 *data, size_t len)
{
    int ret;

    /* Don't use for large transfers! */
    if (len > 64) {
        dev_warn(&spi->dev, "Use spi_sync for large transfers\n");
        return -EINVAL;
    }

    ret = spi_write(spi, data, len);
    if (ret < 0)
        return ret;

    return 0;
}
```

### 8.6.3 spi_read() Example

```c
static int my_device_read_register(struct spi_device *spi,
                                    u8 reg, u8 *value)
{
    int ret;
    u8 tx_buf[2] = {0x03, reg};  /* Read command + address */

    /* Send read command */
    ret = spi_write(spi, tx_buf, 2);
    if (ret < 0)
        return ret;

    /* Read value */
    ret = spi_read(spi, value, 1);
    if (ret < 0)
        return ret;

    return 0;
}
```

### 8.6.4 spi_write_then_read() Example

```c
static int my_device_read_id(struct spi_device *spi, u8 *id, size_t len)
{
    u8 cmd = 0x9F;  /* Read JEDEC ID command */
    int ret;

    /* Send command, then read ID */
    ret = spi_write_then_read(spi, &cmd, 1, id, len);
    if (ret < 0) {
        dev_err(&spi->dev, "Failed to read ID: %d\n", ret);
        return ret;
    }

    dev_info(&spi->dev, "Device ID: %02x %02x %02x\n",
             id[0], id[1], id[2]);

    return 0;
}
```

**⚠️ Limitation:** For more complex operations, use `spi_sync()` directly!

---

## 8.7 Complete Driver Example: SPI Flash

### 8.7.1 Flash Commands

```c
/* Flash commands */
#define CMD_WRITE_ENABLE    0x06
#define CMD_WRITE_DISABLE   0x04
#define CMD_READ_STATUS     0x05
#define CMD_WRITE_STATUS    0x01
#define CMD_READ_DATA       0x03
#define CMD_FAST_READ       0x0B
#define CMD_PAGE_PROGRAM    0x02
#define CMD_SECTOR_ERASE    0x20
#define CMD_CHIP_ERASE      0xC7
#define CMD_READ_ID         0x9F

/* Status register bits */
#define SR_WIP              0x01  /* Write In Progress */
#define SR_WEL              0x02  /* Write Enable Latch */
```

### 8.7.2 Flash Driver Structure

```c
struct spi_flash {
    struct spi_device *spi;
    struct mutex lock;

    u32 chip_size;
    u16 page_size;
    u16 sector_size;

    u8 manufacturer_id;
    u8 device_id;
};
```

### 8.7.3 Read Flash ID

```c
static int spi_flash_read_id(struct spi_flash *flash)
{
    struct spi_transfer xfer[2];
    struct spi_message msg;
    u8 cmd = CMD_READ_ID;
    u8 id[3];
    int ret;

    /* Transfer 1: Send command */
    memset(&xfer[0], 0, sizeof(xfer[0]));
    xfer[0].tx_buf = &cmd;
    xfer[0].len = 1;

    /* Transfer 2: Read ID */
    memset(&xfer[1], 0, sizeof(xfer[1]));
    xfer[1].rx_buf = id;
    xfer[1].len = 3;

    /* Build and execute message */
    spi_message_init(&msg);
    spi_message_add_tail(&xfer[0], &msg);
    spi_message_add_tail(&xfer[1], &msg);

    ret = spi_sync(flash->spi, &msg);
    if (ret < 0)
        return ret;

    flash->manufacturer_id = id[0];
    flash->device_id = (id[1] << 8) | id[2];

    dev_info(&flash->spi->dev, "Flash ID: %02x %04x\n",
             flash->manufacturer_id, flash->device_id);

    return 0;
}
```

### 8.7.4 Read Status Register

```c
static int spi_flash_read_status(struct spi_flash *flash, u8 *status)
{
    return spi_write_then_read(flash->spi,
                               (u8[]){CMD_READ_STATUS}, 1,
                               status, 1);
}

static int spi_flash_wait_ready(struct spi_flash *flash,
                                 unsigned long timeout_ms)
{
    unsigned long deadline = jiffies + msecs_to_jiffies(timeout_ms);
    u8 status;
    int ret;

    do {
        ret = spi_flash_read_status(flash, &status);
        if (ret < 0)
            return ret;

        if (!(status & SR_WIP))
            return 0;  /* Ready */

        usleep_range(1000, 2000);

    } while (time_before(jiffies, deadline));

    return -ETIMEDOUT;
}
```

### 8.7.5 Write Enable

```c
static int spi_flash_write_enable(struct spi_flash *flash)
{
    u8 cmd = CMD_WRITE_ENABLE;
    int ret;

    ret = spi_write(flash->spi, &cmd, 1);
    if (ret < 0)
        return ret;

    /* Verify write enable */
    u8 status;
    ret = spi_flash_read_status(flash, &status);
    if (ret < 0)
        return ret;

    if (!(status & SR_WEL)) {
        dev_err(&flash->spi->dev, "Write enable failed\n");
        return -EIO;
    }

    return 0;
}
```

### 8.7.6 Read Data

```c
static int spi_flash_read(struct spi_flash *flash,
                          u32 addr, u8 *buf, size_t len)
{
    struct spi_transfer xfer[2];
    struct spi_message msg;
    u8 cmd[4];
    int ret;

    /* Wait until ready */
    ret = spi_flash_wait_ready(flash, 1000);
    if (ret < 0)
        return ret;

    /* Prepare command: CMD + 24-bit address */
    cmd[0] = CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    /* Transfer 1: Send command + address */
    memset(&xfer[0], 0, sizeof(xfer[0]));
    xfer[0].tx_buf = cmd;
    xfer[0].len = 4;

    /* Transfer 2: Read data */
    memset(&xfer[1], 0, sizeof(xfer[1]));
    xfer[1].rx_buf = buf;
    xfer[1].len = len;

    /* Execute */
    spi_message_init(&msg);
    spi_message_add_tail(&xfer[0], &msg);
    spi_message_add_tail(&xfer[1], &msg);

    mutex_lock(&flash->lock);
    ret = spi_sync(flash->spi, &msg);
    mutex_unlock(&flash->lock);

    if (ret < 0) {
        dev_err(&flash->spi->dev, "Read failed at 0x%06x\n", addr);
        return ret;
    }

    return 0;
}
```

### 8.7.7 Program Page

```c
static int spi_flash_program_page(struct spi_flash *flash,
                                   u32 addr, const u8 *buf, size_t len)
{
    struct spi_transfer xfer[2];
    struct spi_message msg;
    u8 cmd[4];
    int ret;

    /* Check alignment */
    if (len > flash->page_size) {
        dev_err(&flash->spi->dev, "Data exceeds page size\n");
        return -EINVAL;
    }

    mutex_lock(&flash->lock);

    /* Enable write */
    ret = spi_flash_write_enable(flash);
    if (ret < 0)
        goto out_unlock;

    /* Prepare command */
    cmd[0] = CMD_PAGE_PROGRAM;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    /* Transfer 1: Command + address */
    memset(&xfer[0], 0, sizeof(xfer[0]));
    xfer[0].tx_buf = cmd;
    xfer[0].len = 4;

    /* Transfer 2: Data */
    memset(&xfer[1], 0, sizeof(xfer[1]));
    xfer[1].tx_buf = buf;
    xfer[1].len = len;

    /* Execute */
    spi_message_init(&msg);
    spi_message_add_tail(&xfer[0], &msg);
    spi_message_add_tail(&xfer[1], &msg);

    ret = spi_sync(flash->spi, &msg);
    if (ret < 0) {
        dev_err(&flash->spi->dev, "Program failed at 0x%06x\n", addr);
        goto out_unlock;
    }

    /* Wait for completion */
    ret = spi_flash_wait_ready(flash, 5000);

out_unlock:
    mutex_unlock(&flash->lock);
    return ret;
}
```

### 8.7.8 Erase Sector

```c
static int spi_flash_erase_sector(struct spi_flash *flash, u32 addr)
{
    u8 cmd[4];
    int ret;

    /* Align to sector boundary */
    addr &= ~(flash->sector_size - 1);

    mutex_lock(&flash->lock);

    /* Enable write */
    ret = spi_flash_write_enable(flash);
    if (ret < 0)
        goto out_unlock;

    /* Prepare command */
    cmd[0] = CMD_SECTOR_ERASE;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    /* Send erase command */
    ret = spi_write(flash->spi, cmd, 4);
    if (ret < 0) {
        dev_err(&flash->spi->dev, "Erase failed at 0x%06x\n", addr);
        goto out_unlock;
    }

    /* Wait for erase to complete (can take seconds!) */
    ret = spi_flash_wait_ready(flash, 30000);

out_unlock:
    mutex_unlock(&flash->lock);
    return ret;
}
```

---

## 8.8 Complete Transfer Patterns

### 8.8.1 Pattern 1: Command Only

```c
/* Send single command byte */
static int send_command(struct spi_device *spi, u8 cmd)
{
    return spi_write(spi, &cmd, 1);
}
```

### 8.8.2 Pattern 2: Command + Address

```c
/* Command with 16-bit address */
static int send_command_addr16(struct spi_device *spi,
                                u8 cmd, u16 addr)
{
    u8 buf[3] = {
        cmd,
        (addr >> 8) & 0xFF,
        addr & 0xFF
    };

    return spi_write(spi, buf, 3);
}
```

### 8.8.3 Pattern 3: Command + Data Read

```c
/* Send command, receive data */
static int command_read(struct spi_device *spi,
                        u8 cmd, u8 *data, size_t len)
{
    struct spi_transfer xfer[2];
    struct spi_message msg;
    int ret;

    memset(xfer, 0, sizeof(xfer));

    xfer[0].tx_buf = &cmd;
    xfer[0].len = 1;

    xfer[1].rx_buf = data;
    xfer[1].len = len;

    spi_message_init(&msg);
    spi_message_add_tail(&xfer[0], &msg);
    spi_message_add_tail(&xfer[1], &msg);

    return spi_sync(spi, &msg);
}
```

### 8.8.4 Pattern 4: Register Write (Command + Addr + Data)

```c
/* Write to register: CMD + ADDR + DATA */
static int write_register(struct spi_device *spi,
                          u8 cmd, u8 addr, u8 value)
{
    u8 buf[3] = {cmd, addr, value};
    return spi_write(spi, buf, 3);
}
```

### 8.8.5 Pattern 5: Burst Read

```c
/* Read multiple bytes from address */
static int burst_read(struct spi_device *spi,
                      u8 cmd, u16 addr,
                      u8 *data, size_t len)
{
    struct spi_transfer xfer[2];
    struct spi_message msg;
    u8 header[3] = {
        cmd,
        (addr >> 8) & 0xFF,
        addr & 0xFF
    };

    memset(xfer, 0, sizeof(xfer));

    xfer[0].tx_buf = header;
    xfer[0].len = 3;

    xfer[1].rx_buf = data;
    xfer[1].len = len;

    spi_message_init(&msg);
    spi_message_add_tail(&xfer[0], &msg);
    spi_message_add_tail(&xfer[1], &msg);

    return spi_sync(spi, &msg);
}
```

---

## Summary

In this part, we covered:

**Key Concepts:**

1. **SPI I/O Model**
    - Message-based queued transfers
    - Atomic message processing
    - FIFO order execution
2. **struct spi_transfer**
    - Single full-duplex transfer
    - tx_buf, rx_buf, len
    - Override speed, bits_per_word
    - cs_change for CS control
3. **struct spi_message**
    - Container for multiple transfers
    - Completion callback
    - Status tracking
4. **Transfer Functions**
    - `spi_sync()`: Blocking, for process context
    - `spi_async()`: Non-blocking, for IRQ context
    - Helper functions for simple operations
5. **Complete Examples**
    - SPI flash driver
    - Read/write/erase operations
    - Common transfer patterns