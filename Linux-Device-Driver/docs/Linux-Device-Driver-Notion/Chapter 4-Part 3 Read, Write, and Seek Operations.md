# Part 3. Read, Write, and Seek Operations

This part covers the core I/O operations: reading from device, writing to device, and seeking within device memory. These are the most frequently used operations in character drivers.

---

## 4.2.6 The read() Method

### Function Prototype

```c
ssize_t (*read)(struct file *file, char __user *buf,
                size_t count, loff_t *f_pos);

/* Parameters:
 * file  - File structure pointer
 * buf   - User space buffer to write data into
 * count - Number of bytes user wants to read
 * f_pos - Current file position (read/write cursor)
 *
 * Returns:
 *   > 0  - Number of bytes successfully read
 *   = 0  - End of file (EOF)
 *   < 0  - Error code (e.g., -EFAULT, -EINVAL)
 */
```

### What read() Should Do

```
read() Implementation Steps:

1. Check f_pos
   ├─ If at EOF → return 0
   └─ Continue

2. Adjust count
   ├─ Don't read beyond device size
   └─ count = min(count, device_size - f_pos)

3. Read from device
   ├─ Get data from hardware or buffer
   └─ Store in kernel buffer

4. Copy to user
   ├─ copy_to_user(buf, kernel_buf, count)
   └─ Check return value

5. Update f_pos
   ├─ *f_pos += bytes_read
   └─ Return bytes_read
```

### Important Notes

**About f_pos:**

- ⚠️ `f_pos` is **per file descriptor**, not per device
- ⚠️ Multiple opens = multiple independent `f_pos` values
- ⚠️ Driver must update `f_pos` after successful read

**About return value:**

- ✅ Return actual bytes read (may be less than requested)
- ✅ Return 0 for EOF
- ✅ Return negative error code on failure
- ⚠️ Returning less than `count` is **not an error**

### Basic read() Implementation

```c
static ssize_t my_read(struct file *file, char __user *buf,
                      size_t count, loff_t *f_pos)
{
    struct my_device *dev = file->private_data;
    size_t bytes_to_read;
    unsigned long ret;

    /* Step 1: Check for EOF */
    if (*f_pos >= dev->size) {
        pr_info("my_device: EOF at position %lld\n", *f_pos);
        return 0;  /* EOF */
    }

    /* Step 2: Adjust count to not exceed device size */
    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;

    bytes_to_read = count;

    /* Step 3: Device-specific read (example: from memory buffer) */
    /* In real driver, this might read from hardware registers */

    /* Step 4: Copy to user space */
    ret = copy_to_user(buf, dev->buffer + *f_pos, bytes_to_read);
    if (ret) {
        pr_err("my_device: copy_to_user failed, %lu bytes not copied\n", ret);
        return -EFAULT;
    }

    /* Step 5: Update file position */
    *f_pos += bytes_to_read;

    pr_info("my_device: Read %zu bytes, new f_pos=%lld\n",
            bytes_to_read, *f_pos);

    return bytes_to_read;
}
```

### Complete read() Example with Hardware

```c
#define DEVICE_MEM_SIZE 1024

struct uart_device {
    struct cdev cdev;
    void __iomem *reg_base;      /* Hardware registers */
    char *rx_buffer;             /* Receive buffer */
    size_t rx_count;             /* Bytes in RX buffer */
    spinlock_t lock;
};

static ssize_t uart_read(struct file *file, char __user *buf,
                        size_t count, loff_t *f_pos)
{
    struct uart_device *uart = file->private_data;
    size_t bytes_to_read;
    unsigned long flags;
    unsigned long ret;

    /* Check if data available */
    spin_lock_irqsave(&uart->lock, flags);

    if (uart->rx_count == 0) {
        spin_unlock_irqrestore(&uart->lock, flags);

        /* Non-blocking mode: return immediately */
        if (file->f_flags & O_NONBLOCK)
            return -EAGAIN;

        /* Blocking mode: would wait here (covered in later section) */
        return 0;  /* No data for now */
    }

    /* Calculate how much to read */
    bytes_to_read = min(count, uart->rx_count);

    /* Copy from kernel RX buffer to user */
    ret = copy_to_user(buf, uart->rx_buffer, bytes_to_read);
    if (ret) {
        spin_unlock_irqrestore(&uart->lock, flags);
        return -EFAULT;
    }

    /* Remove copied data from buffer (shift remaining data) */
    memmove(uart->rx_buffer,
            uart->rx_buffer + bytes_to_read,
            uart->rx_count - bytes_to_read);
    uart->rx_count -= bytes_to_read;

    spin_unlock_irqrestore(&uart->lock, flags);

    /* Note: f_pos not used for stream devices like UART */

    pr_debug("uart: Read %zu bytes, %zu bytes remaining in buffer\n",
             bytes_to_read, uart->rx_count);

    return bytes_to_read;
}
```

---

## 4.2.7 The write() Method

### Function Prototype

```c
ssize_t (*write)(struct file *file, const char __user *buf,
                 size_t count, loff_t *f_pos);

/* Parameters:
 * file  - File structure pointer
 * buf   - User space buffer containing data to write
 * count - Number of bytes user wants to write
 * f_pos - Current file position
 *
 * Returns:
 *   > 0  - Number of bytes successfully written
 *   < 0  - Error code
 */
```

### What write() Should Do

```
write() Implementation Steps:

1. Check space available
   ├─ If no space → return -ENOSPC
   └─ Continue

2. Adjust count
   ├─ Don't write beyond device size
   └─ count = min(count, device_size - f_pos)

3. Copy from user
   ├─ copy_from_user(kernel_buf, buf, count)
   └─ Check return value

4. Write to device
   ├─ Write to hardware or buffer
   └─ Handle errors

5. Update f_pos
   ├─ *f_pos += bytes_written
   └─ Return bytes_written
```

### Basic write() Implementation

```c
static ssize_t my_write(struct file *file, const char __user *buf,
                       size_t count, loff_t *f_pos)
{
    struct my_device *dev = file->private_data;
    size_t bytes_to_write;
    unsigned long ret;

    /* Step 1: Check if buffer is full */
    if (*f_pos >= dev->size) {
        pr_err("my_device: Buffer full at position %lld\n", *f_pos);
        return -ENOSPC;  /* No space left on device */
    }

    /* Step 2: Adjust count to not exceed device size */
    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;

    bytes_to_write = count;

    /* Step 3: Copy from user space */
    ret = copy_from_user(dev->buffer + *f_pos, buf, bytes_to_write);
    if (ret) {
        pr_err("my_device: copy_from_user failed, %lu bytes not copied\n", ret);
        return -EFAULT;
    }

    /* Step 4: Device-specific write (if needed) */
    /* In real driver, this might write to hardware */

    /* Step 5: Update file position */
    *f_pos += bytes_to_write;

    pr_info("my_device: Wrote %zu bytes, new f_pos=%lld\n",
            bytes_to_write, *f_pos);

    return bytes_to_write;
}
```

### Complete write() Example with Validation

```c
#define MAX_BUFFER_SIZE 4096

struct char_device {
    struct cdev cdev;
    char *buffer;
    size_t buffer_size;
    size_t data_size;      /* Current amount of data */
    struct mutex lock;
};

static ssize_t char_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *f_pos)
{
    struct char_device *dev = file->private_data;
    size_t bytes_to_write;
    size_t available_space;
    unsigned long ret;

    pr_debug("char_dev: write() called: count=%zu, f_pos=%lld\n",
             count, *f_pos);

    /* Check for write permission */
    if ((file->f_mode & FMODE_WRITE) == 0) {
        pr_err("char_dev: File not opened for writing\n");
        return -EBADF;
    }

    /* Lock device */
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    /* Check position validity */
    if (*f_pos < 0 || *f_pos > dev->buffer_size) {
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }

    /* Calculate available space */
    available_space = dev->buffer_size - *f_pos;

    if (available_space == 0) {
        mutex_unlock(&dev->lock);
        pr_info("char_dev: No space available at f_pos=%lld\n", *f_pos);
        return -ENOSPC;
    }

    /* Adjust count */
    bytes_to_write = min(count, available_space);

    /* Copy from user */
    ret = copy_from_user(dev->buffer + *f_pos, buf, bytes_to_write);
    if (ret) {
        mutex_unlock(&dev->lock);
        pr_err("char_dev: Failed to copy %lu bytes from user\n", ret);
        return -EFAULT;
    }

    /* Update position and data size */
    *f_pos += bytes_to_write;
    if (*f_pos > dev->data_size)
        dev->data_size = *f_pos;

    mutex_unlock(&dev->lock);

    pr_debug("char_dev: Wrote %zu bytes, f_pos=%lld, data_size=%zu\n",
             bytes_to_write, *f_pos, dev->data_size);

    return bytes_to_write;
}
```

---

## 4.2.8 The llseek() Method

### Function Prototype

```c
loff_t (*llseek)(struct file *file, loff_t offset, int whence);

/* Parameters:
 * file   - File structure pointer
 * offset - Offset value (meaning depends on whence)
 * whence - Where to seek from
 *
 * Returns:
 *   >= 0 - New file position
 *   < 0  - Error code
 */
```

### Whence Values

```c
#define SEEK_SET    0    /* Seek from beginning of file */
#define SEEK_CUR    1    /* Seek from current position */
#define SEEK_END    2    /* Seek from end of file */

/* Examples:
 * lseek(fd, 0, SEEK_SET)     → Go to beginning
 * lseek(fd, 0, SEEK_END)     → Go to end
 * lseek(fd, 10, SEEK_SET)    → Go to position 10
 * lseek(fd, -5, SEEK_CUR)    → Go back 5 bytes
 * lseek(fd, -10, SEEK_END)   → 10 bytes before end
 */
```

### What llseek() Should Do

```
llseek() Implementation:

Switch on whence:
├─ SEEK_SET: new_pos = offset
├─ SEEK_CUR: new_pos = f_pos + offset
└─ SEEK_END: new_pos = file_size + offset

Validate new_pos:
├─ If new_pos < 0      → return -EINVAL
├─ If new_pos > max    → return -EINVAL (or allow)
└─ Else                → update f_pos, return new_pos
```

### Basic llseek() Implementation

```c
static loff_t my_llseek(struct file *file, loff_t offset, int whence)
{
    struct my_device *dev = file->private_data;
    loff_t new_pos;

    switch (whence) {
    case SEEK_SET:  /* Absolute position */
        new_pos = offset;
        break;

    case SEEK_CUR:  /* Relative to current */
        new_pos = file->f_pos + offset;
        break;

    case SEEK_END:  /* Relative to end */
        new_pos = dev->size + offset;
        break;

    default:
        return -EINVAL;
    }

    /* Validate new position */
    if (new_pos < 0) {
        pr_err("my_device: Invalid seek position %lld\n", new_pos);
        return -EINVAL;
    }

    /* Allow seeking beyond end (like regular files) */
    if (new_pos > dev->size) {
        pr_warn("my_device: Seeking beyond end: %lld > %zu\n",
                new_pos, dev->size);
    }

    /* Update position */
    file->f_pos = new_pos;

    pr_debug("my_device: Seeked to position %lld\n", new_pos);

    return new_pos;
}
```

### Complete llseek() Example

```c
#define DEVICE_SIZE 1024

struct seekable_device {
    struct cdev cdev;
    char buffer[DEVICE_SIZE];
    struct mutex lock;
};

static loff_t seekable_llseek(struct file *file, loff_t offset, int whence)
{
    struct seekable_device *dev = file->private_data;
    loff_t new_pos = 0;

    pr_debug("seekable: llseek(offset=%lld, whence=%d)\n", offset, whence);

    /* Lock to prevent race with concurrent operations */
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    switch (whence) {
    case SEEK_SET:
        new_pos = offset;
        pr_debug("seekable: SEEK_SET to %lld\n", offset);
        break;

    case SEEK_CUR:
        new_pos = file->f_pos + offset;
        pr_debug("seekable: SEEK_CUR: %lld + %lld = %lld\n",
                 file->f_pos, offset, new_pos);
        break;

    case SEEK_END:
        new_pos = DEVICE_SIZE + offset;
        pr_debug("seekable: SEEK_END: %d + %lld = %lld\n",
                 DEVICE_SIZE, offset, new_pos);
        break;

    default:
        mutex_unlock(&dev->lock);
        pr_err("seekable: Invalid whence value %d\n", whence);
        return -EINVAL;
    }

    /* Validate bounds */
    if (new_pos < 0) {
        mutex_unlock(&dev->lock);
        pr_err("seekable: Negative position %lld not allowed\n", new_pos);
        return -EINVAL;
    }

    /* For this device, don't allow seeking beyond end */
    if (new_pos > DEVICE_SIZE) {
        mutex_unlock(&dev->lock);
        pr_err("seekable: Position %lld exceeds device size %d\n",
               new_pos, DEVICE_SIZE);
        return -EINVAL;
    }

    /* Update position */
    file->f_pos = new_pos;

    mutex_unlock(&dev->lock);

    pr_debug("seekable: New position: %lld\n", new_pos);

    return new_pos;
}
```

---

## 4.2.9 Complete Example: EEPROM-like Device

### Full Driver with Read/Write/Seek

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "eeprom"
#define CLASS_NAME  "eeprom_class"
#define EEPROM_SIZE 512

/* EEPROM device structure */
struct eeprom_device {
    struct cdev cdev;
    dev_t dev_num;
    struct class *class;
    struct device *device;

    char memory[EEPROM_SIZE];  /* Simulated EEPROM memory */
    struct mutex lock;         /* Protect memory access */

    unsigned long read_count;
    unsigned long write_count;
};

static struct eeprom_device eeprom_dev;

/*
 * Open - Initialize access
 */
static int eeprom_open(struct inode *inode, struct file *file)
{
    file->private_data = &eeprom_dev;

    pr_info("eeprom: Device opened (flags=0x%x)\n", file->f_flags);

    return 0;
}

/*
 * Release - Cleanup
 */
static int eeprom_release(struct inode *inode, struct file *file)
{
    pr_info("eeprom: Device closed\n");
    return 0;
}

/*
 * Read - Read from EEPROM
 */
static ssize_t eeprom_read(struct file *file, char __user *buf,
                          size_t count, loff_t *f_pos)
{
    struct eeprom_device *dev = file->private_data;
    size_t bytes_to_read;

    pr_debug("eeprom: read(count=%zu, f_pos=%lld)\n", count, *f_pos);

    /* Check for EOF */
    if (*f_pos >= EEPROM_SIZE) {
        pr_debug("eeprom: EOF at position %lld\n", *f_pos);
        return 0;
    }

    /* Lock memory */
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    /* Calculate bytes to read */
    bytes_to_read = min(count, (size_t)(EEPROM_SIZE - *f_pos));

    /* Copy to user */
    if (copy_to_user(buf, dev->memory + *f_pos, bytes_to_read)) {
        mutex_unlock(&dev->lock);
        return -EFAULT;
    }

    /* Update position and statistics */
    *f_pos += bytes_to_read;
    dev->read_count += bytes_to_read;

    mutex_unlock(&dev->lock);

    pr_debug("eeprom: Read %zu bytes, new f_pos=%lld\n",
             bytes_to_read, *f_pos);

    return bytes_to_read;
}

/*
 * Write - Write to EEPROM
 */
static ssize_t eeprom_write(struct file *file, const char __user *buf,
                           size_t count, loff_t *f_pos)
{
    struct eeprom_device *dev = file->private_data;
    size_t bytes_to_write;

    pr_debug("eeprom: write(count=%zu, f_pos=%lld)\n", count, *f_pos);

    /* Check if full */
    if (*f_pos >= EEPROM_SIZE) {
        pr_err("eeprom: Memory full at position %lld\n", *f_pos);
        return -ENOSPC;
    }

    /* Lock memory */
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    /* Calculate bytes to write */
    bytes_to_write = min(count, (size_t)(EEPROM_SIZE - *f_pos));

    /* Copy from user */
    if (copy_from_user(dev->memory + *f_pos, buf, bytes_to_write)) {
        mutex_unlock(&dev->lock);
        return -EFAULT;
    }

    /* Update position and statistics */
    *f_pos += bytes_to_write;
    dev->write_count += bytes_to_write;

    mutex_unlock(&dev->lock);

    pr_debug("eeprom: Wrote %zu bytes, new f_pos=%lld\n",
             bytes_to_write, *f_pos);

    return bytes_to_write;
}

/*
 * lseek - Change file position
 */
static loff_t eeprom_llseek(struct file *file, loff_t offset, int whence)
{
    loff_t new_pos;

    pr_debug("eeprom: llseek(offset=%lld, whence=%d)\n", offset, whence);

    switch (whence) {
    case SEEK_SET:
        new_pos = offset;
        break;

    case SEEK_CUR:
        new_pos = file->f_pos + offset;
        break;

    case SEEK_END:
        new_pos = EEPROM_SIZE + offset;
        break;

    default:
        return -EINVAL;
    }

    /* Validate */
    if (new_pos < 0 || new_pos > EEPROM_SIZE) {
        pr_err("eeprom: Invalid position %lld\n", new_pos);
        return -EINVAL;
    }

    file->f_pos = new_pos;

    pr_debug("eeprom: New position: %lld\n", new_pos);

    return new_pos;
}

/* File operations */
static struct file_operations eeprom_fops = {
    .owner   = THIS_MODULE,
    .open    = eeprom_open,
    .release = eeprom_release,
    .read    = eeprom_read,
    .write   = eeprom_write,
    .llseek  = eeprom_llseek,
};

/*
 * Module init
 */
static int __init eeprom_init(void)
{
    int ret;

    pr_info("eeprom: Initializing EEPROM driver\n");

    /* Initialize */
    mutex_init(&eeprom_dev.lock);
    memset(eeprom_dev.memory, 0xFF, EEPROM_SIZE);  /* Erased state */
    eeprom_dev.read_count = 0;
    eeprom_dev.write_count = 0;

    /* Allocate device number */
    ret = alloc_chrdev_region(&eeprom_dev.dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("eeprom: Failed to allocate device number\n");
        return ret;
    }

    /* Initialize cdev */
    cdev_init(&eeprom_dev.cdev, &eeprom_fops);
    eeprom_dev.cdev.owner = THIS_MODULE;

    /* Add cdev */
    ret = cdev_add(&eeprom_dev.cdev, eeprom_dev.dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(eeprom_dev.dev_num, 1);
        return ret;
    }

    /* Create class */
    eeprom_dev.class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(eeprom_dev.class)) {
        cdev_del(&eeprom_dev.cdev);
        unregister_chrdev_region(eeprom_dev.dev_num, 1);
        return PTR_ERR(eeprom_dev.class);
    }

    /* Create device */
    eeprom_dev.device = device_create(eeprom_dev.class, NULL,
                                     eeprom_dev.dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(eeprom_dev.device)) {
        class_destroy(eeprom_dev.class);
        cdev_del(&eeprom_dev.cdev);
        unregister_chrdev_region(eeprom_dev.dev_num, 1);
        return PTR_ERR(eeprom_dev.device);
    }

    pr_info("eeprom: Device created: /dev/%s (size=%d bytes)\n",
            DEVICE_NAME, EEPROM_SIZE);

    return 0;
}

/*
 * Module exit
 */
static void __exit eeprom_exit(void)
{
    pr_info("eeprom: Statistics - Read: %lu bytes, Write: %lu bytes\n",
            eeprom_dev.read_count, eeprom_dev.write_count);

    device_destroy(eeprom_dev.class, eeprom_dev.dev_num);
    class_destroy(eeprom_dev.class);
    cdev_del(&eeprom_dev.cdev);
    unregister_chrdev_region(eeprom_dev.dev_num, 1);

    pr_info("eeprom: Driver unloaded\n");
}

module_init(eeprom_init);
module_exit(eeprom_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Expert");
MODULE_DESCRIPTION("Simulated EEPROM character device driver");
MODULE_VERSION("1.0");
```

### Comprehensive Test Program

```c
/* test_eeprom.c */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEVICE "/dev/eeprom"

void print_hex(const char *label, const unsigned char *data, size_t len)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n     ");
    }
    printf("\n");
}

int main(void)
{
    int fd;
    ssize_t ret;
    off_t pos;
    unsigned char write_buf[64];
    unsigned char read_buf[64];

    printf("=== EEPROM Driver Test ===\n\n");

    /* Open device */
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }
    printf("✓ Device opened\n\n");

    /* === TEST 1: Write at beginning === */
    printf("TEST 1: Write at beginning\n");
    memset(write_buf, 0xAA, sizeof(write_buf));
    ret = write(fd, write_buf, sizeof(write_buf));
    printf("  Wrote %zd bytes\n", ret);
    pos = lseek(fd, 0, SEEK_CUR);
    printf("  Current position: %ld\n\n", pos);

    /* === TEST 2: Seek to beginning and read === */
    printf("TEST 2: Seek to beginning and read\n");
    lseek(fd, 0, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    ret = read(fd, read_buf, sizeof(read_buf));
    printf("  Read %zd bytes\n", ret);
    print_hex("  Data", read_buf, ret);
    printf("\n");

    /* === TEST 3: Seek to offset and write === */
    printf("TEST 3: Seek to offset 100 and write\n");
    lseek(fd, 100, SEEK_SET);
    memset(write_buf, 0xBB, 32);
    ret = write(fd, write_buf, 32);
    printf("  Wrote %zd bytes at offset 100\n", ret);
    pos = lseek(fd, 0, SEEK_CUR);
    printf("  Current position: %ld\n\n", pos);

    /* === TEST 4: Seek back and read === */
    printf("TEST 4: Seek back to offset 100 and read\n");
    lseek(fd, 100, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    ret = read(fd, read_buf, 32);
    printf("  Read %zd bytes\n", ret);
    print_hex("  Data", read_buf, ret);
    printf("\n");

    /* === TEST 5: Seek from end === */
    printf("TEST 5: Seek from end\n");
    pos = lseek(fd, -20, SEEK_END);
    printf("  Position from SEEK_END(-20): %ld\n", pos);
    memset(write_buf, 0xCC, 20);
    ret = write(fd, write_buf, 20);
    printf("  Wrote %zd bytes\n\n", ret);

    /* === TEST 6: Read from end === */
    printf("TEST 6: Read last 20 bytes\n");
    lseek(fd, -20, SEEK_END);
    memset(read_buf, 0, sizeof(read_buf));
    ret = read(fd, read_buf, 20);
    printf("  Read %zd bytes\n", ret);
    print_hex("  Data", read_buf, ret);
    printf("\n");

    /* === TEST 7: Relative seek === */
    printf("TEST 7: Relative seek\n");
    lseek(fd, 200, SEEK_SET);
    printf("  Position after SEEK_SET(200): %ld\n",
           (long)lseek(fd, 0, SEEK_CUR));
    lseek(fd, 50, SEEK_CUR);
    printf("  Position after SEEK_CUR(50): %ld\n",
           (long)lseek(fd, 0, SEEK_CUR));
    lseek(fd, -30, SEEK_CUR);
    printf("  Position after SEEK_CUR(-30): %ld\n\n",
           (long)lseek(fd, 0, SEEK_CUR));

    /* Close device */
    close(fd);
    printf("✓ Device closed\n");
    printf("\n=== Test Complete ===\n");

    return EXIT_SUCCESS;
}
```

**Compile and run:**

```bash
# Build driver
make

# Load driver
sudo insmod eeprom.ko

# Compile test
gcc -o test_eeprom test_eeprom.c

# Run test
sudo ./test_eeprom
```

**Expected output:**

```
=== EEPROM Driver Test ===

✓ Device opened

TEST 1: Write at beginning
  Wrote 64 bytes
  Current position: 64

TEST 2: Seek to beginning and read
  Read 64 bytes
  Data: AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA
        AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA
        AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA
        AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA

TEST 3: Seek to offset 100 and write
  Wrote 32 bytes at offset 100
  Current position: 132

TEST 4: Seek back to offset 100 and read
  Read 32 bytes
  Data: BB BB BB BB BB BB BB BB BB BB BB BB BB BB BB BB
        BB BB BB BB BB BB BB BB BB BB BB BB BB BB BB BB

TEST 5: Seek from end
  Position from SEEK_END(-20): 492
  Wrote 20 bytes

TEST 6: Read last 20 bytes
  Read 20 bytes
  Data: CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC
        CC CC CC CC

TEST 7: Relative seek
  Position after SEEK_SET(200): 200
  Position after SEEK_CUR(50): 250
  Position after SEEK_CUR(-30): 220

✓ Device closed

=== Test Complete ===
```

---

## Summary

This chapter covered read, write, and seek implementations:

**Key Topics:**

- ✅ read() method - reading from device to user
- ✅ write() method - writing from user to device
- ✅ llseek() method - changing file position
- ✅ f_pos management and validation
- ✅ Error handling patterns
- ✅ Complete EEPROM-like driver example
- ✅ Comprehensive test program