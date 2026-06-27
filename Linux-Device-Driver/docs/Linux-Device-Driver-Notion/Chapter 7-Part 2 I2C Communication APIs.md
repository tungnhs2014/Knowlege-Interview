# Part 2. I2C Communication APIs

This part covers the actual I2C communication methods - how to read and write data to/from I2C devices using both plain I2C and SMBus APIs.

---

## 7.1 Introduction to I2C Communication

### 7.1.1 Communication Overview

**Two types of I2C APIs in Linux:**

```
┌────────────────────────────────────────────────┐
│         I2C Communication APIs                 │
├────────────────────────────────────────────────┤
│                                                │
│  1. Plain I2C API                              │
│     - i2c_master_send()                        │
│     - i2c_master_recv()                        │
│     - i2c_transfer()                           │
│     - Full I2C protocol support                │
│                                                │
│  2. SMBus API                                  │
│     - i2c_smbus_read_byte()                    │
│     - i2c_smbus_write_byte()                   │
│     - i2c_smbus_read_word_data()               │
│     - i2c_smbus_write_word_data()              │
│     - Simplified, higher-level                 │
│                                                │
└────────────────────────────────────────────────┘
```

**Choosing between Plain I2C and SMBus:**

- **Use SMBus API if:**
    - Device follows SMBus specification
    - Simple byte/word operations needed
    - Want simpler, safer API
    - **Important:** I2C devices are SMBus-compatible, but NOT the reverse!
- **Use Plain I2C API if:**
    - Need full control over transactions
    - Complex multi-message transfers
    - Device requires specific I2C protocol sequences

---

## 7.2 Plain I2C Communication

### 7.2.1 Basic Transfer Functions

**API functions:**

```c
#include <linux/i2c.h>

/* Send data to I2C device */
int i2c_master_send(struct i2c_client *client,
                   const char *buf,
                   int count);

/* Receive data from I2C device */
int i2c_master_recv(struct i2c_client *client,
                   char *buf,
                   int count);
```

**Parameters:**

- `client`: Pointer to I2C client structure
- `buf`: Buffer for data
- `count`: Number of bytes to transfer

**Return value:**

- Success: Number of bytes transferred
- Error: Negative error code

### 7.2.2 Simple Write Example

```c
static int my_i2c_write(struct i2c_client *client, u8 reg, u8 value)
{
    u8 buf[2];
    int ret;

    /* Prepare buffer: [register address, value] */
    buf[0] = reg;
    buf[1] = value;

    /* Send 2 bytes to device */
    ret = i2c_master_send(client, buf, 2);
    if (ret < 0) {
        dev_err(&client->dev, "I2C write failed: %d\n", ret);
        return ret;
    }

    /* Check we sent exactly 2 bytes */
    if (ret != 2) {
        dev_err(&client->dev, "Short write: %d\n", ret);
        return -EIO;
    }

    return 0;
}
```

**Transaction visualization:**

```
┌─────────────────────────────────────────┐
│  i2c_master_send(client, buf, 2)        │
├─────────────────────────────────────────┤
│                                         │
│  START                                  │
│    ▼                                    │
│  [Slave Addr | W] → ACK                 │
│  [Register]       → ACK                 │
│  [Value]          → ACK                 │
│  STOP                                   │
│                                         │
└─────────────────────────────────────────┘
```

### 7.2.3 Simple Read Example

```c
static int my_i2c_read(struct i2c_client *client, u8 reg, u8 *value)
{
    int ret;

    /* Step 1: Write register address */
    ret = i2c_master_send(client, &reg, 1);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to send register address\n");
        return ret;
    }

    /* Step 2: Read value from device */
    ret = i2c_master_recv(client, value, 1);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read value\n");
        return ret;
    }

    if (ret != 1)
        return -EIO;

    return 0;
}
```

**Transaction visualization:**

```
┌──────────────────────────────────────────┐
│  Two-step I2C read                       │
├──────────────────────────────────────────┤
│                                          │
│  Step 1: i2c_master_send(client, &reg, 1)│
│    START                                 │
│    [Slave Addr | W] → ACK                │
│    [Register]       → ACK                │
│    STOP                                  │
│                                          │
│  Step 2: i2c_master_recv(client, val, 1) │
│    START                                 │
│    [Slave Addr | R] → ACK                │
│    [Data]           ← device             │
│    NACK (from master)                    │
│    STOP                                  │
│                                          │
└──────────────────────────────────────────┘
```

### 7.2.4 Advanced Transfer - i2c_transfer()

**For complex multi-message transactions:**

```c
int i2c_transfer(struct i2c_adapter *adap,
                struct i2c_msg *msg,
                int num);
```

**Parameters:**

- `adap`: I2C adapter (from `client->adapter`)
- `msg`: Array of I2C messages
- `num`: Number of messages

**The i2c_msg structure:**

```c
struct i2c_msg {
    __u16 addr;      /* Slave address */
    __u16 flags;     /* Message flags */
    __u16 len;       /* Message length */
    __u8 *buf;       /* Pointer to message data */
};
```

**Important flags:**

```c
#define I2C_M_RD            0x0001  /* Read data (slave to master) */
#define I2C_M_TEN           0x0010  /* 10-bit address mode */
#define I2C_M_RECV_LEN      0x0400  /* First byte is length */
#define I2C_M_NO_RD_ACK     0x0800  /* Don't ACK on read */
#define I2C_M_IGNORE_NAK    0x1000  /* Ignore NAK from slave */
#define I2C_M_REV_DIR_ADDR  0x2000  /* Reverse direction flag */
#define I2C_M_NOSTART       0x4000  /* Don't send START condition */
#define I2C_M_STOP          0x8000  /* Send STOP after message */

/* For writes: flags = 0 */
/* For reads: flags = I2C_M_RD */
```

### 7.2.5 Real-World Example: EEPROM 24LC512

**Complete read function for 24LC512 EEPROM:**

```c
/*
 * EEPROM read using i2c_transfer
 * 24LC512 has 16-bit address space
 */
static ssize_t eep_read(struct file *filp, char __user *buf,
                       size_t count, loff_t *f_pos)
{
    struct eeprom_dev *dev = filp->private_data;
    int ret;
    int _reg_addr = dev->current_pointer;
    u8 reg_addr[2];
    struct i2c_msg msg[2];

    /* 16-bit address: split into 2 bytes */
    reg_addr[0] = (u8)(_reg_addr >> 8);    /* High byte */
    reg_addr[1] = (u8)(_reg_addr & 0xFF);  /* Low byte */

    /* Message 1: Write register address */
    msg[0].addr = dev->client->addr;
    msg[0].flags = 0;              /* Write operation */
    msg[0].len = 2;                /* 2-byte address */
    msg[0].buf = reg_addr;

    /* Message 2: Read data */
    msg[1].addr = dev->client->addr;
    msg[1].flags = I2C_M_RD;       /* Read operation */
    msg[1].len = count;
    msg[1].buf = dev->data;

    /* Execute combined transaction */
    ret = i2c_transfer(dev->client->adapter, msg, 2);
    if (ret < 0) {
        pr_err("ee24lc512: i2c_transfer failed\n");
        return ret;
    }

    /* Copy to user space */
    if (copy_to_user(buf, dev->data, count) != 0) {
        return -EIO;
    }

    return count;
}
```

**Why use i2c_transfer instead of separate send/recv?**

```
┌───────────────────────────────────────────────┐
│  i2c_transfer() with 2 messages               │
├───────────────────────────────────────────────┤
│                                               │
│  START                                        │
│  [Slave Addr | W] → ACK                       │
│  [Addr High]      → ACK                       │
│  [Addr Low]       → ACK                       │
│  REPEATED START (no STOP in between!)         │
│  [Slave Addr | R] → ACK                       │
│  [Data Byte 1]    ← device → ACK              │
│  [Data Byte 2]    ← device → ACK              │
│  ...                                          │
│  [Data Byte N]    ← device → NACK             │
│  STOP                                         │
│                                               │
│  Benefits:                                    │
│  - Atomic operation                           │
│  - No STOP between write and read             │
│  - Prevents other masters from interrupting   │
│  - Required by many devices                   │
└───────────────────────────────────────────────┘
```

### 7.2.6 EEPROM Write Example

```c
static ssize_t eep_write(struct file *filp, const char __user *buf,
                        size_t count, loff_t *f_pos)
{
    struct eeprom_dev *dev = filp->private_data;
    int ret;
    int _reg_addr = dev->current_pointer;
    u8 *write_buf;

    /* Allocate buffer: 2 bytes address + data */
    write_buf = kmalloc(count + 2, GFP_KERNEL);
    if (!write_buf)
        return -ENOMEM;

    /* Prepare buffer */
    write_buf[0] = (u8)(_reg_addr >> 8);    /* Address high */
    write_buf[1] = (u8)(_reg_addr & 0xFF);  /* Address low */

    /* Copy data from user space */
    if (copy_from_user(&write_buf[2], buf, count) != 0) {
        kfree(write_buf);
        return -EIO;
    }

    /* Send everything in one transaction */
    ret = i2c_master_send(dev->client, write_buf, count + 2);
    kfree(write_buf);

    if (ret < 0) {
        pr_err("ee24lc512: i2c_master_send failed\n");
        return ret;
    }

    /* EEPROM needs time to write (5-10ms typical) */
    msleep(10);

    return count;
}
```

### 7.2.7 Important Buffer Size Limitation

**Critical note:**

```c
struct i2c_msg {
    __u16 len;  /* This is u16! */
    /* ... */
};
```

**Maximum transfer size:**

```
┌──────────────────────────────────────────┐
│  Buffer Size Limitation                  │
├──────────────────────────────────────────┤
│                                          │
│  msg.len is __u16 (16-bit)               │
│                                          │
│  Maximum size = 2^16 - 1 = 65535 bytes   │
│                        = 64 KB           │
│                                          │
│  Always ensure:                          │
│    buffer_size < 65536                   │
│                                          │
└──────────────────────────────────────────┘
```

**Safety check:**

```c
if (count > 65535) {
    dev_err(&client->dev, "Transfer size too large\n");
    return -EINVAL;
}
```

---

## 7.3 SMBus API - System Management Bus

### 7.3.1 What is SMBus?

**SMBus** (System Management Bus) is a subset of I2C developed by Intel:

```
┌────────────────────────────────────────────┐
│  I2C vs SMBus                              │
├────────────────────────────────────────────┤
│                                            │
│  I2C        ⊃  SMBus                       │
│  (superset)    (subset)                    │
│                                            │
│  I2C devices → SMBus compatible ✓          │
│  SMBus devices → I2C compatible ✗          │
│                                            │
│  Use SMBus API for maximum compatibility!  │
└────────────────────────────────────────────┘
```

**Why use SMBus API?**

- Simpler API
- Better error checking
- Broader hardware support
- I2C devices work with SMBus API

### 7.3.2 SMBus API Functions

**Header file:**

```c
#include <linux/i2c.h>
```

**Available functions:**

```c
/* Quick command (no data) */
s32 i2c_smbus_write_quick(struct i2c_client *client, u8 value);

/* Byte operations */
s32 i2c_smbus_read_byte(struct i2c_client *client);
s32 i2c_smbus_write_byte(struct i2c_client *client, u8 value);

/* Byte data operations (register + data) */
s32 i2c_smbus_read_byte_data(struct i2c_client *client, u8 command);
s32 i2c_smbus_write_byte_data(struct i2c_client *client,
                              u8 command, u8 value);

/* Word data operations (16-bit) */
s32 i2c_smbus_read_word_data(struct i2c_client *client, u8 command);
s32 i2c_smbus_write_word_data(struct i2c_client *client,
                              u8 command, u16 value);

/* Block data operations */
s32 i2c_smbus_read_block_data(struct i2c_client *client,
                              u8 command, u8 *values);
s32 i2c_smbus_write_block_data(struct i2c_client *client,
                               u8 command, u8 length,
                               const u8 *values);

/* I2C block operations */
s32 i2c_smbus_read_i2c_block_data(struct i2c_client *client,
                                 u8 command, u8 length, u8 *values);
s32 i2c_smbus_write_i2c_block_data(struct i2c_client *client,
                                  u8 command, u8 length,
                                  const u8 *values);
```

### 7.3.3 SMBus Transaction Types

**Visual representation:**

```
┌─────────────────────────────────────────────────┐
│  SMBus Transaction Types                        │
├─────────────────────────────────────────────────┤
│                                                 │
│  1. Quick Command                               │
│     i2c_smbus_write_quick(client, value)        │
│     [S][Addr|R/W][A][P]                         │
│                                                 │
│  2. Receive Byte                                │
│     i2c_smbus_read_byte(client)                 │
│     [S][Addr|R][A][Data][N][P]                  │
│                                                 │
│  3. Send Byte                                   │
│     i2c_smbus_write_byte(client, value)         │
│     [S][Addr|W][A][Data][A][P]                  │
│                                                 │
│  4. Read Byte Data                              │
│     i2c_smbus_read_byte_data(client, cmd)       │
│     [S][Addr|W][A][Cmd][A]                      │
│     [S][Addr|R][A][Data][N][P]                  │
│                                                 │
│  5. Write Byte Data                             │
│     i2c_smbus_write_byte_data(client, cmd, val) │
│     [S][Addr|W][A][Cmd][A][Data][A][P]          │
│                                                 │
│  6. Read Word Data                              │
│     i2c_smbus_read_word_data(client, cmd)       │
│     [S][Addr|W][A][Cmd][A]                      │
│     [S][Addr|R][A][DataLow][A][DataHigh][N][P]  │
│                                                 │
│  Legend:                                        │
│  S=START, P=STOP, A=ACK, N=NACK, W=Write, R=Read│
└─────────────────────────────────────────────────┘
```

### 7.3.4 SMBus Example: GPIO Expander MCP23016

**Complete GPIO expander driver using SMBus:**

```c
#define GP0  0x00  /* GPIO Port 0 register */
#define GP1  0x01  /* GPIO Port 1 register */

struct mcp23016 {
    struct i2c_client *client;
    struct gpio_chip chip;
    struct mutex lock;
};

/* Set GPIO value using SMBus */
static int mcp23016_set(struct mcp23016 *mcp, unsigned offset, int val)
{
    s32 value;
    unsigned bank = offset / 8;        /* Which register (GP0 or GP1) */
    u8 reg_gpio = (bank == 0) ? GP0 : GP1;
    unsigned bit = offset % 8;         /* Which bit in register */

    /* Read current register value */
    value = i2c_smbus_read_byte_data(mcp->client, reg_gpio);
    if (value < 0) {
        dev_err(&mcp->client->dev, "Failed to read GPIO\n");
        return value;
    }

    /* Modify bit */
    if (val)
        value |= (1 << bit);   /* Set bit */
    else
        value &= ~(1 << bit);  /* Clear bit */

    /* Write back */
    return i2c_smbus_write_byte_data(mcp->client, reg_gpio, value);
}

/* Get GPIO value using SMBus */
static int mcp23016_get(struct mcp23016 *mcp, unsigned offset)
{
    s32 value;
    unsigned bank = offset / 8;
    u8 reg_gpio = (bank == 0) ? GP0 : GP1;
    unsigned bit = offset % 8;

    value = i2c_smbus_read_byte_data(mcp->client, reg_gpio);
    if (value < 0)
        return value;

    return !!(value & (1 << bit));
}

/* GPIO chip callbacks */
static int mcp23016_gpio_get(struct gpio_chip *chip, unsigned offset)
{
    struct mcp23016 *mcp = gpiochip_get_data(chip);
    int ret;

    mutex_lock(&mcp->lock);
    ret = mcp23016_get(mcp, offset);
    mutex_unlock(&mcp->lock);

    return ret;
}

static void mcp23016_gpio_set(struct gpio_chip *chip,
                             unsigned offset, int value)
{
    struct mcp23016 *mcp = gpiochip_get_data(chip);

    mutex_lock(&mcp->lock);
    mcp23016_set(mcp, offset, value);
    mutex_unlock(&mcp->lock);
}

static int mcp23016_direction_output(struct gpio_chip *chip,
                                    unsigned offset, int value)
{
    /* Set direction register, then set value */
    /* Implementation omitted for brevity */
    return 0;
}

static int mcp23016_direction_input(struct gpio_chip *chip, unsigned offset)
{
    /* Set direction register */
    /* Implementation omitted for brevity */
    return 0;
}
```

### 7.3.5 SMBus Block Operations

**Reading multiple bytes:**

```c
static int read_sensor_data(struct i2c_client *client)
{
    u8 data[32];
    s32 ret;

    /* Read up to 32 bytes from register 0x10 */
    ret = i2c_smbus_read_i2c_block_data(client, 0x10, 32, data);
    if (ret < 0) {
        dev_err(&client->dev, "Block read failed: %d\n", ret);
        return ret;
    }

    dev_info(&client->dev, "Read %d bytes\n", ret);

    /* Process data */
    return 0;
}
```

**Writing multiple bytes:**

```c
static int write_configuration(struct i2c_client *client)
{
    u8 config[4] = {0x01, 0x02, 0x03, 0x04};
    s32 ret;

    /* Write 4 bytes to register 0x20 */
    ret = i2c_smbus_write_i2c_block_data(client, 0x20, 4, config);
    if (ret < 0) {
        dev_err(&client->dev, "Block write failed: %d\n", ret);
        return ret;
    }

    return 0;
}
```

### 7.3.6 Error Handling with SMBus

**All SMBus functions return:**

- **Positive value or 0**: Success (for reads, this is the data)
- **Negative value**: Error code

**Proper error checking:**

```c
static int read_device_id(struct i2c_client *client)
{
    s32 id;

    id = i2c_smbus_read_byte_data(client, REG_DEVICE_ID);
    if (id < 0) {
        /* Error occurred */
        dev_err(&client->dev, "Failed to read device ID: %d\n", id);
        return id;
    }

    /* Success - id contains the value */
    if (id != EXPECTED_ID) {
        dev_err(&client->dev, "Wrong device ID: 0x%02x\n", id);
        return -ENODEV;
    }

    dev_info(&client->dev, "Device ID: 0x%02x\n", id);
    return 0;
}
```

---

## 7.4 Complete Real-World Driver Example

### 7.4.1 Temperature Sensor Driver (LM75)

**Complete driver demonstrating all concepts:**

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

/* LM75 Registers */
#define LM75_REG_TEMP       0x00
#define LM75_REG_CONFIG     0x01
#define LM75_REG_THYST      0x02
#define LM75_REG_TOS        0x03

/* Private data structure */
struct lm75_data {
    struct i2c_client *client;
    struct device *hwmon_dev;
    struct mutex lock;
    int temp;  /* Temperature in millidegrees */
};

/* Read temperature from sensor */
static int lm75_read_temp(struct i2c_client *client)
{
    s32 raw;
    int temp;

    /* Read 16-bit temperature value */
    raw = i2c_smbus_read_word_data(client, LM75_REG_TEMP);
    if (raw < 0)
        return raw;

    /* LM75 temperature format:
     * - 16-bit value, MSB first
     * - Resolution: 0.5°C per LSB
     * - Convert to millidegrees Celsius
     */
    raw = swab16(raw);  /* Swap bytes */
    temp = (raw >> 5) * 500;  /* Each bit = 0.5°C = 500 millidegrees */

    return temp;
}

/* Sysfs attribute: read temperature */
static ssize_t temp_show(struct device *dev,
                        struct device_attribute *attr, char *buf)
{
    struct lm75_data *data = dev_get_drvdata(dev);
    int temp;

    mutex_lock(&data->lock);
    temp = lm75_read_temp(data->client);
    mutex_unlock(&data->lock);

    if (temp < 0)
        return temp;

    /* Return temperature in millidegrees */
    return sprintf(buf, "%d\n", temp);
}

/* Sysfs attribute: update interval */
static ssize_t update_interval_show(struct device *dev,
                                   struct device_attribute *attr,
                                   char *buf)
{
    return sprintf(buf, "%d\n", 200);  /* 200ms */
}

/* Define sysfs attributes */
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, temp_show, NULL, 0);
static SENSOR_DEVICE_ATTR(update_interval, S_IRUGO,
                         update_interval_show, NULL, 0);

static struct attribute *lm75_attrs[] = {
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_update_interval.dev_attr.attr,
    NULL
};

ATTRIBUTE_GROUPS(lm75);

/* Configure sensor */
static int lm75_configure(struct i2c_client *client)
{
    s32 ret;
    u8 config;

    /* Read current configuration */
    ret = i2c_smbus_read_byte_data(client, LM75_REG_CONFIG);
    if (ret < 0)
        return ret;

    config = ret;

    /* Clear shutdown bit (bit 0) to enable sensor */
    config &= ~0x01;

    /* Write configuration back */
    ret = i2c_smbus_write_byte_data(client, LM75_REG_CONFIG, config);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to write config\n");
        return ret;
    }

    dev_info(&client->dev, "Sensor configured\n");
    return 0;
}

/* Probe function */
static int lm75_probe(struct i2c_client *client,
                     const struct i2c_device_id *id)
{
    struct lm75_data *data;
    int ret;

    /* Check adapter functionality */
    if (!i2c_check_functionality(client->adapter,
                                I2C_FUNC_SMBUS_BYTE_DATA |
                                I2C_FUNC_SMBUS_WORD_DATA)) {
        dev_err(&client->dev, "Adapter doesn't support required ops\n");
        return -ENODEV;
    }

    /* Allocate private data */
    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    /* Initialize private data */
    data->client = client;
    mutex_init(&data->lock);

    /* Store private data */
    i2c_set_clientdata(client, data);

    /* Configure sensor */
    ret = lm75_configure(client);
    if (ret < 0)
        return ret;

    /* Register with hwmon subsystem */
    data->hwmon_dev = devm_hwmon_device_register_with_groups(
                        &client->dev, client->name, data, lm75_groups);
    if (IS_ERR(data->hwmon_dev)) {
        ret = PTR_ERR(data->hwmon_dev);
        dev_err(&client->dev, "Failed to register hwmon device\n");
        return ret;
    }

    dev_info(&client->dev, "LM75 temperature sensor initialized\n");
    return 0;
}

/* Remove function */
static int lm75_remove(struct i2c_client *client)
{
    struct lm75_data *data = i2c_get_clientdata(client);

    /* hwmon device is unregistered automatically (devm_*) */
    mutex_destroy(&data->lock);

    dev_info(&client->dev, "LM75 driver removed\n");
    return 0;
}

/* Device ID table */
static const struct i2c_device_id lm75_id[] = {
    { "lm75", 0 },
    { "lm75a", 1 },
    { }
};
MODULE_DEVICE_TABLE(i2c, lm75_id);

/* OF match table */
static const struct of_device_id lm75_of_match[] = {
    { .compatible = "national,lm75", },
    { .compatible = "national,lm75a", },
    { }
};
MODULE_DEVICE_TABLE(of, lm75_of_match);

/* I2C driver structure */
static struct i2c_driver lm75_driver = {
    .driver = {
        .name = "lm75",
        .of_match_table = lm75_of_match,
    },
    .probe = lm75_probe,
    .remove = lm75_remove,
    .id_table = lm75_id,
};

module_i2c_driver(lm75_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("LM75 Temperature Sensor Driver");
```

**Usage from userspace:**

```bash
# Temperature is accessible via sysfs
cat /sys/class/hwmon/hwmon0/temp1_input
25500   # 25.5°C (in millidegrees)

cat /sys/class/hwmon/hwmon0/update_interval
200     # Update every 200ms
```

### 7.4.2 Advanced Example: Multi-Register Read

```c
/* Read accelerometer data (3 axes) */
static int read_accel_data(struct i2c_client *client,
                          s16 *x, s16 *y, s16 *z)
{
    u8 data[6];
    s32 ret;

    /* Read 6 bytes starting from X-axis register */
    ret = i2c_smbus_read_i2c_block_data(client, REG_ACCEL_X, 6, data);
    if (ret < 0)
        return ret;

    if (ret != 6) {
        dev_err(&client->dev, "Expected 6 bytes, got %d\n", ret);
        return -EIO;
    }

    /* Parse data (assuming little-endian) */
    *x = (s16)((data[1] << 8) | data[0]);
    *y = (s16)((data[3] << 8) | data[2]);
    *z = (s16)((data[5] << 8) | data[4]);

    return 0;
}
```

---

## 7.5 Choosing the Right API

### 7.5.1 Decision Tree

```
┌────────────────────────────────────────────┐
│  Which I2C API should I use?               │
├────────────────────────────────────────────┤
│                                            │
│  Does device follow SMBus protocol?        │
│    ├─ YES → Use SMBus API                  │
│    │         (i2c_smbus_read_byte_data,    │
│    │          i2c_smbus_write_word_data)   │
│    │                                       │
│    └─ NO/UNSURE → Ask yourself:            │
│         │                                  │
│         ├─ Simple byte/word access?        │
│         │   YES → Try SMBus API first      │
│         │                                  │
│         ├─ Need atomic multi-message?      │
│         │   YES → Use i2c_transfer()       │
│         │                                  │
│         └─ Complex protocol?               │
│             YES → Use i2c_transfer()       │
│                                            │
└────────────────────────────────────────────┘
```

### 7.5.2 API Comparison Table

```
┌──────────────────┬─────────────────┬────────────────────┐
│ Operation        │ Plain I2C       │ SMBus              │
├──────────────────┼─────────────────┼────────────────────┤
│ Read 1 byte      │ i2c_master_recv │ i2c_smbus_read_    │
│ (no register)    │                 │   byte             │
│                  │                 │                    │
│ Write 1 byte     │ i2c_master_send │ i2c_smbus_write_   │
│ (no register)    │                 │   byte             │
│                  │                 │                    │
│ Read register    │ i2c_transfer    │ i2c_smbus_read_    │
│ (1 byte)         │ (2 messages)    │   byte_data        │
│                  │                 │                    │
│ Write register   │ i2c_master_send │ i2c_smbus_write_   │
│ (1 byte)         │                 │   byte_data        │
│                  │                 │                    │
│ Read register    │ i2c_transfer    │ i2c_smbus_read_    │
│ (2 bytes)        │ (2 messages)    │   word_data        │
│                  │                 │                    │
│ Write register   │ i2c_master_send │ i2c_smbus_write_   │
│ (2 bytes)        │                 │   word_data        │
│                  │                 │                    │
│ Block read       │ i2c_transfer    │ i2c_smbus_read_    │
│                  │                 │   i2c_block_data   │
│                  │                 │                    │
│ Block write      │ i2c_master_send │ i2c_smbus_write_   │
│                  │                 │   i2c_block_data   │
└──────────────────┴─────────────────┴────────────────────┘
```

### 7.5.3 Best Practices

**1. Always check adapter functionality:**

```c
if (!i2c_check_functionality(client->adapter,
                            I2C_FUNC_SMBUS_BYTE_DATA)) {
    dev_err(&client->dev, "Adapter lacks required functionality\n");
    return -EIO;
}
```

*2. Use devm_ functions for cleanup:**

```c
/* Automatic cleanup on error/remove */
data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
```

**3. Protect I2C transactions with mutex:**

```c
struct my_device {
    struct mutex lock;
    /* ... */
};

/* In functions that access I2C */
mutex_lock(&dev->lock);
ret = i2c_smbus_read_byte_data(client, reg);
mutex_unlock(&dev->lock);
```

**4. Check return values:**

```c
ret = i2c_smbus_write_byte_data(client, reg, value);
if (ret < 0) {
    dev_err(&client->dev, "Write failed: %d\n", ret);
    return ret;
}
```

**5. Use dev_err/dev_info for logging:**

```c
dev_err(&client->dev, "Error message\n");
dev_info(&client->dev, "Info message\n");
```

---

## Summary

In this part, we covered:

**Plain I2C API:**

- `i2c_master_send()` / `i2c_master_recv()`
- `i2c_transfer()` for atomic multi-message transactions
- `struct i2c_msg` for building messages
- Buffer size limitation (64 KB)

**SMBus API:**

- Simpler, safer API
- Better hardware compatibility
- Byte/Word/Block operations
- Return value: data or negative error

**Real-World Examples:**

- EEPROM 24LC512 driver
- GPIO expander MCP23016
- Temperature sensor LM75
- Accelerometer data reading

**Best Practices:**

- Check adapter functionality
- Use devm_* for automatic cleanup
- Protect with mutex
- Always check return values
- Proper error handling