# Part 3. Userspace GPIO Access

This part covers userspace GPIO access through the legacy sysfs interface and the modern libgpiod library, with complete working examples.

---

## 14.1 GPIO Sysfs Interface (Legacy - Deprecated)

### 14.1.1 Overview

**The sysfs GPIO interface allows userspace GPIO control:**

```
┌────────────────────────────────────────────────┐
│  GPIO Sysfs Interface                          │
├────────────────────────────────────────────────┤
│                                                │
│  Location: /sys/class/gpio/                    │
│                                                │
│  Status: DEPRECATED                            │
│  - Still widely used in existing systems       │
│  - Being replaced by libgpiod                  │
│  - Has known limitations                       │
│                                                │
│  Advantages:                                   │
│  ✓ No additional tools needed                  │
│  ✓ Works with standard shell commands          │
│  ✓ Simple interface                            │
│                                                │
│  Disadvantages:                                │
│  ✗ GPIOs remain exported if process crashes    │
│  ✗ Need to compute GPIO numbers                │
│  ✗ GPIO numbers not stable                     │
│  ✗ Race conditions possible                    │
│  ✗ No support for bulk operations              │
│                                                │
└────────────────────────────────────────────────┘
```

### 14.1.2 Sysfs Directory Structure

**Three types of entries:**

```
/sys/class/gpio/
│
├── export              (write-only)
│   Write GPIO number to export it
│
├── unexport            (write-only)
│   Write GPIO number to unexport it
│
├── gpiochip0/          (directory for GPIO controller 0)
│   ├── base            (read-only: first GPIO number)
│   ├── label           (read-only: controller name)
│   └── ngpio           (read-only: number of GPIOs)
│
├── gpiochip32/         (directory for GPIO controller 1)
│   ├── base
│   ├── label
│   └── ngpio
│
├── gpio21/             (directory for exported GPIO 21)
│   ├── direction       (read/write: "in" or "out")
│   ├── value           (read/write: 0 or 1)
│   ├── edge            (read/write: "none", "rising", "falling", "both")
│   └── active_low      (read/write: 0 or 1)
│
└── gpio53/             (directory for exported GPIO 53)
    ├── direction
    ├── value
    ├── edge
    └── active_low
```

### 14.1.3 Control Files

**1. /sys/class/gpio/export**

```bash
# Export GPIO 21 for userspace control
echo 21 > /sys/class/gpio/export

# This creates /sys/class/gpio/gpio21/ directory
```

**2. /sys/class/gpio/unexport**

```bash
# Remove GPIO 21 from userspace
echo 21 > /sys/class/gpio/unexport

# This removes /sys/class/gpio/gpio21/ directory
```

**3. /sys/class/gpio/gpioN/direction**

```bash
# Set as input
echo "in" > /sys/class/gpio/gpio21/direction

# Set as output (default LOW)
echo "out" > /sys/class/gpio/gpio21/direction

# Set as output with initial HIGH
echo "high" > /sys/class/gpio/gpio21/direction

# Set as output with initial LOW
echo "low" > /sys/class/gpio/gpio21/direction
```

**4. /sys/class/gpio/gpioN/value**

```bash
# Read value (for input or output)
cat /sys/class/gpio/gpio21/value
# Returns: 0 or 1

# Write value (for output only)
echo 1 > /sys/class/gpio/gpio21/value  # Set HIGH
echo 0 > /sys/class/gpio/gpio21/value  # Set LOW
```

**5. /sys/class/gpio/gpioN/edge**

```bash
# Disable edge detection
echo "none" > /sys/class/gpio/gpio21/edge

# Trigger on rising edge
echo "rising" > /sys/class/gpio/gpio21/edge

# Trigger on falling edge
echo "falling" > /sys/class/gpio/gpio21/edge

# Trigger on both edges
echo "both" > /sys/class/gpio/gpio21/edge
```

**6. /sys/class/gpio/gpioN/active_low**

```bash
# Normal polarity (0 = LOW, 1 = HIGH)
echo 0 > /sys/class/gpio/gpio21/active_low

# Inverted polarity (0 = HIGH, 1 = LOW)
echo 1 > /sys/class/gpio/gpio21/active_low
```

### 14.1.4 Complete Shell Script Example - LED Blink

```bash
#!/bin/bash

# BeagleBone Black LED Blink Example
# Hardware: LED connected to GPIO1_21 (P8.13)

# Calculate GPIO number: (bank * 32) + offset
# GPIO1_21 = (1 * 32) + 21 = 53
GPIO=53

# Export GPIO
echo "Exporting GPIO $GPIO"
echo $GPIO > /sys/class/gpio/export

# Wait for directory to be created
sleep 0.1

# Set as output
echo "Setting GPIO $GPIO as output"
echo "out" > /sys/class/gpio/gpio$GPIO/direction

# Blink 10 times
echo "Blinking LED 10 times"
for i in {1..10}; do
    echo "Blink $i"
    echo 1 > /sys/class/gpio/gpio$GPIO/value
    sleep 0.5
    echo 0 > /sys/class/gpio/gpio$GPIO/value
    sleep 0.5
done

# Cleanup
echo "Cleaning up"
echo $GPIO > /sys/class/gpio/unexport

echo "Done!"
```

### 14.1.5 C Program Example - Button with poll()

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define GPIO_BTN 17  /* GPIO number for button */

static int gpio_export(int gpio)
{
    int fd;
    char buf[64];

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        perror("Failed to open export");
        return -1;
    }

    snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, strlen(buf));
    close(fd);

    return 0;
}

static int gpio_unexport(int gpio)
{
    int fd;
    char buf[64];

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        perror("Failed to open unexport");
        return -1;
    }

    snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, strlen(buf));
    close(fd);

    return 0;
}

static int gpio_set_direction(int gpio, const char *direction)
{
    int fd;
    char path[64];

    snprintf(path, sizeof(path),
             "/sys/class/gpio/gpio%d/direction", gpio);

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open direction");
        return -1;
    }

    write(fd, direction, strlen(direction));
    close(fd);

    return 0;
}

static int gpio_set_edge(int gpio, const char *edge)
{
    int fd;
    char path[64];

    snprintf(path, sizeof(path),
             "/sys/class/gpio/gpio%d/edge", gpio);

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open edge");
        return -1;
    }

    write(fd, edge, strlen(edge));
    close(fd);

    return 0;
}

static int gpio_get_value(int gpio)
{
    int fd;
    char path[64];
    char value;

    snprintf(path, sizeof(path),
             "/sys/class/gpio/gpio%d/value", gpio);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open value");
        return -1;
    }

    read(fd, &value, 1);
    close(fd);

    return (value == '0') ? 0 : 1;
}

int main(int argc, char **argv)
{
    int fd;
    char path[64];
    char value;
    struct pollfd pfd;
    int ret;

    printf("GPIO Button Monitor (sysfs)\n");
    printf("Press Ctrl+C to exit\n\n");

    /* Export GPIO */
    if (gpio_export(GPIO_BTN) < 0) {
        fprintf(stderr, "Failed to export GPIO %d\n", GPIO_BTN);
        return 1;
    }

    /* Wait for sysfs files to be created */
    usleep(100000);

    /* Set as input */
    if (gpio_set_direction(GPIO_BTN, "in") < 0) {
        fprintf(stderr, "Failed to set direction\n");
        gpio_unexport(GPIO_BTN);
        return 1;
    }

    /* Enable edge detection on both edges */
    if (gpio_set_edge(GPIO_BTN, "both") < 0) {
        fprintf(stderr, "Failed to set edge\n");
        gpio_unexport(GPIO_BTN);
        return 1;
    }

    /* Open value file for polling */
    snprintf(path, sizeof(path),
             "/sys/class/gpio/gpio%d/value", GPIO_BTN);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open value for polling");
        gpio_unexport(GPIO_BTN);
        return 1;
    }

    /* Setup poll structure */
    pfd.fd = fd;
    pfd.events = POLLPRI | POLLERR;

    /* Initial read to clear any pending interrupt */
    lseek(fd, 0, SEEK_SET);
    read(fd, &value, 1);

    printf("Waiting for button events...\n\n");

    /* Main loop */
    while (1) {
        /* Wait for event */
        ret = poll(&pfd, 1, -1);

        if (ret < 0) {
            perror("poll() failed");
            break;
        }

        if (ret == 0) {
            /* Timeout (shouldn't happen with -1) */
            continue;
        }

        /* Check if event occurred */
        if (pfd.revents & POLLPRI) {
            /* Read value */
            lseek(fd, 0, SEEK_SET);
            read(fd, &value, 1);

            printf("Button event detected! Value: %c\n", value);

            if (value == '1')
                printf("  → Button PRESSED\n");
            else
                printf("  → Button RELEASED\n");
        }
    }

    /* Cleanup */
    close(fd);
    gpio_unexport(GPIO_BTN);

    return 0;
}
```

**Compilation:**

```bash
gcc -o gpio_button gpio_button.c
sudo ./gpio_button
```

### 14.1.6 Complete LCD Driver Example (16x2)

**Hardware connections (BeagleBone Black):**

```
LCD Pin → BBB Pin → GPIO Number
RS  → P8.45 → GPIO2_6  = 70
RW  → P8.46 → GPIO2_7  = 71
EN  → P8.43 → GPIO2_8  = 72
D4  → P8.44 → GPIO2_9  = 73
D5  → P8.41 → GPIO2_10 = 74
D6  → P8.42 → GPIO2_11 = 75
D7  → P8.39 → GPIO2_12 = 76
```

**C program:**

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* GPIO definitions */
#define GPIO_RS  70
#define GPIO_RW  71
#define GPIO_EN  72
#define GPIO_D4  73
#define GPIO_D5  74
#define GPIO_D6  75
#define GPIO_D7  76

/* LCD commands */
#define LCD_CLEAR       0x01
#define LCD_HOME        0x02
#define LCD_ENTRY_MODE  0x06
#define LCD_DISPLAY_ON  0x0C
#define LCD_FUNCTION_4BIT 0x28

static int gpio_fds[7];  /* File descriptors for value files */

static void gpio_export(int gpio)
{
    int fd;
    char buf[64];

    fd = open("/sys/class/gpio/export", O_WRONLY);
    snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, strlen(buf));
    close(fd);
}

static void gpio_set_output(int gpio)
{
    int fd;
    char path[64];

    snprintf(path, sizeof(path),
             "/sys/class/gpio/gpio%d/direction", gpio);

    fd = open(path, O_WRONLY);
    write(fd, "out", 3);
    close(fd);
}

static int gpio_open_value(int gpio)
{
    char path[64];
    snprintf(path, sizeof(path),
             "/sys/class/gpio/gpio%d/value", gpio);
    return open(path, O_WRONLY);
}

static void gpio_write(int fd, int value)
{
    write(fd, value ? "1" : "0", 1);
}

static void lcd_pulse_enable(void)
{
    gpio_write(gpio_fds[2], 1);  /* EN high */
    usleep(1);
    gpio_write(gpio_fds[2], 0);  /* EN low */
    usleep(50);
}

static void lcd_write_nibble(unsigned char nibble)
{
    gpio_write(gpio_fds[3], (nibble >> 0) & 0x01);  /* D4 */
    gpio_write(gpio_fds[4], (nibble >> 1) & 0x01);  /* D5 */
    gpio_write(gpio_fds[5], (nibble >> 2) & 0x01);  /* D6 */
    gpio_write(gpio_fds[6], (nibble >> 3) & 0x01);  /* D7 */
    lcd_pulse_enable();
}

static void lcd_write_byte(unsigned char byte, int rs)
{
    gpio_write(gpio_fds[0], rs);   /* RS */
    gpio_write(gpio_fds[1], 0);    /* RW = 0 (write) */

    lcd_write_nibble(byte >> 4);   /* Upper nibble */
    lcd_write_nibble(byte & 0x0F); /* Lower nibble */

    usleep(2000);  /* Command execution time */
}

static void lcd_command(unsigned char cmd)
{
    lcd_write_byte(cmd, 0);  /* RS = 0 for command */
}

static void lcd_data(unsigned char data)
{
    lcd_write_byte(data, 1);  /* RS = 1 for data */
}

static void lcd_init(void)
{
    int gpios[] = {GPIO_RS, GPIO_RW, GPIO_EN,
                   GPIO_D4, GPIO_D5, GPIO_D6, GPIO_D7};
    int i;

    printf("Initializing LCD...\n");

    /* Export and configure all GPIOs */
    for (i = 0; i < 7; i++) {
        gpio_export(gpios[i]);
        usleep(100000);
        gpio_set_output(gpios[i]);
        gpio_fds[i] = gpio_open_value(gpios[i]);
    }

    usleep(50000);  /* Wait for LCD power-on */

    /* Initialize in 4-bit mode */
    lcd_write_nibble(0x03);
    usleep(5000);
    lcd_write_nibble(0x03);
    usleep(150);
    lcd_write_nibble(0x03);
    usleep(150);
    lcd_write_nibble(0x02);  /* Set 4-bit mode */
    usleep(150);

    /* Configure LCD */
    lcd_command(LCD_FUNCTION_4BIT);  /* 4-bit, 2 lines, 5x8 font */
    lcd_command(LCD_DISPLAY_ON);     /* Display on, cursor off */
    lcd_command(LCD_CLEAR);          /* Clear display */
    usleep(2000);
    lcd_command(LCD_ENTRY_MODE);     /* Entry mode */

    printf("LCD initialized!\n");
}

static void lcd_print(const char *str)
{
    while (*str) {
        lcd_data(*str++);
    }
}

static void lcd_set_cursor(int row, int col)
{
    unsigned char address = (row == 0) ? 0x00 : 0x40;
    address += col;
    lcd_command(0x80 | address);
}

int main(int argc, char **argv)
{
    printf("LCD Driver Example (sysfs)\n\n");

    /* Initialize LCD */
    lcd_init();

    /* Display message */
    lcd_command(LCD_CLEAR);
    lcd_set_cursor(0, 0);
    lcd_print("Hello, World!");
    lcd_set_cursor(1, 0);
    lcd_print("Embedded Linux!");

    printf("\nMessage displayed on LCD\n");
    printf("Press Enter to exit...\n");
    getchar();

    /* Cleanup */
    lcd_command(LCD_CLEAR);

    int i;
    for (i = 0; i < 7; i++) {
        close(gpio_fds[i]);
    }

    printf("Done!\n");

    return 0;
}
```

**Compilation and usage:**

```bash
gcc -o lcd_driver lcd_driver.c
sudo ./lcd_driver
```

### 14.1.7 Kernel Export Functions

**Drivers can export GPIOs to sysfs programmatically:**

**Integer-based interface:**

```c
int gpio_export(unsigned gpio, bool direction_may_change);
void gpio_unexport(unsigned gpio);
int gpio_export_link(struct device *dev, const char *name,
                     unsigned gpio);
```

**Descriptor-based interface:**

```c
int gpiod_export(struct gpio_desc *desc, bool direction_may_change);
void gpiod_unexport(struct gpio_desc *desc);
int gpiod_export_link(struct device *dev, const char *name,
                      struct gpio_desc *desc);
```

**Example:**

```c
static int my_probe(struct platform_device *pdev)
{
    struct gpio_desc *red, *green;

    /* Get GPIOs */
    red = gpiod_get_index(&pdev->dev, "led", 0, GPIOD_OUT_LOW);
    green = gpiod_get_index(&pdev->dev, "led", 1, GPIOD_OUT_LOW);

    /* Export to sysfs (direction can be changed) */
    gpiod_export(red, true);
    gpiod_export(green, true);

    /* Create symbolic links under device */
    gpiod_export_link(&pdev->dev, "Red_LED", red);
    gpiod_export_link(&pdev->dev, "Green_LED", green);

    /*
     * Now accessible at:
     * /sys/class/gpio/gpioN/  (exported GPIO)
     * /sys/devices/.../Red_LED  (symbolic link)
     * /sys/devices/.../Green_LED
     */

    return 0;
}
```

---

## 14.2 Modern libgpiod Interface

### 14.2.1 Overview

**libgpiod is the modern userspace GPIO interface:**

```
┌────────────────────────────────────────────────┐
│  libgpiod - Modern GPIO Interface              │
├────────────────────────────────────────────────┤
│                                                │
│  Status: RECOMMENDED                           │
│                                                │
│  Based on: /dev/gpiochipX character devices    │
│                                                │
│  Advantages:                                   │
│  ✓ Proper resource management                  │
│  ✓ No need to compute GPIO numbers             │
│  ✓ Stable interface                            │
│  ✓ Bulk operations support                     │
│  ✓ Line name lookup                            │
│  ✓ Modern C API                                │
│  ✓ Command-line tools included                 │
│                                                │
│  Components:                                   │
│  • libgpiod - C library                        │
│  • gpiodetect - List GPIO chips                │
│  • gpioinfo - Show GPIO info                   │
│  • gpioget - Read GPIO values                  │
│  • gpioset - Write GPIO values                 │
│  • gpiomon - Monitor GPIO events               │
│                                                │
└────────────────────────────────────────────────┘
```

### 14.2.2 Character Device Interface

**Each GPIO controller appears as /dev/gpiochipN:**

```bash
$ ls -l /dev/gpiochip*
crw-rw---- 1 root gpio 254, 0 Dec 15 10:00 /dev/gpiochip0
crw-rw---- 1 root gpio 254, 1 Dec 15 10:00 /dev/gpiochip1
crw-rw---- 1 root gpio 254, 2 Dec 15 10:00 /dev/gpiochip2
crw-rw---- 1 root gpio 254, 3 Dec 15 10:00 /dev/gpiochip3
```

### 14.2.3 Command-Line Tools

**1. gpiodetect - List GPIO chips:**

```bash
$ gpiodetect
gpiochip0 [gpio-0-31] (32 lines)
gpiochip1 [gpio-32-63] (32 lines)
gpiochip2 [gpio-64-95] (32 lines)
gpiochip3 [gpio-96-127] (32 lines)
```

**2. gpioinfo - Show detailed info:**

```bash
$ gpioinfo gpiochip1
gpiochip1 - 32 lines:
        line   0:      unnamed       unused   input  active-high
        line   1:      unnamed       unused   input  active-high
        line   2:      unnamed       unused   input  active-high
        ...
        line  21:   "USER_LED"       unused   input  active-high
        line  22:   "USER_BTN"       unused   input  active-high
        ...
```

**3. gpioget - Read GPIO values:**

```bash
# Read single GPIO (chip 1, line 21)
$ gpioget gpiochip1 21
0

# Read multiple GPIOs
$ gpioget gpiochip1 21 22 23
0 1 0

# Read by line name
$ gpioget gpiochip1 USER_LED
0
```

**4. gpioset - Write GPIO values:**

```bash
# Set single GPIO to HIGH
$ gpioset gpiochip1 21=1

# Set multiple GPIOs
$ gpioset gpiochip1 21=1 22=0 23=1

# Set and keep running (default: exits immediately)
$ gpioset --mode=signal --background gpiochip1 21=1

# Set for specific duration
$ gpioset --mode=time --sec=5 gpiochip1 21=1

# Set with active-low
$ gpioset --active-low gpiochip1 21=1
```

**5. gpiomon - Monitor GPIO events:**

```bash
# Monitor single GPIO for events
$ gpiomon gpiochip1 21

# Monitor with rising edge only
$ gpiomon --rising-edge gpiochip1 21

# Monitor with falling edge only
$ gpiomon --falling-edge gpiochip1 21

# Monitor both edges
$ gpiomon --both-edges gpiochip1 21

# Example output:
event: RISING EDGE offset: 21 timestamp: [1234567890.123456789]
event: FALLING EDGE offset: 21 timestamp: [1234567890.234567890]
```

### 14.2.4 Installing libgpiod

**On Debian/Ubuntu:**

```bash
sudo apt-get install libgpiod-dev gpiod
```

**Cross-compilation for embedded systems:**

```bash
# Download source
git clone https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git
cd libgpiod

# Configure for cross-compilation
./autogen.sh --enable-tools=yes --prefix=/usr \
    --host=arm-linux-gnueabihf \
    CC=arm-linux-gnueabihf-gcc

# Build
make

# Install to staging directory
make DESTDIR=/path/to/staging install
```

### 14.2.5 C Library API

**Basic data types:**

```c
struct gpiod_chip;      /* GPIO chip */
struct gpiod_line;      /* Single GPIO line */
struct gpiod_line_bulk; /* Multiple GPIO lines */
```

**Core functions:**

```c
/* Open GPIO chip */
struct gpiod_chip *gpiod_chip_open(const char *path);
struct gpiod_chip *gpiod_chip_open_by_name(const char *name);
struct gpiod_chip *gpiod_chip_open_by_number(unsigned int num);

/* Close GPIO chip */
void gpiod_chip_close(struct gpiod_chip *chip);

/* Get GPIO line */
struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip,
                                        unsigned int offset);
struct gpiod_line *gpiod_chip_find_line(struct gpiod_chip *chip,
                                         const char *name);

/* Request line for use */
int gpiod_line_request_input(struct gpiod_line *line,
                              const char *consumer);
int gpiod_line_request_output(struct gpiod_line *line,
                               const char *consumer,
                               int default_val);
int gpiod_line_request_rising_edge_events(struct gpiod_line *line,
                                           const char *consumer);
int gpiod_line_request_falling_edge_events(struct gpiod_line *line,
                                            const char *consumer);
int gpiod_line_request_both_edges_events(struct gpiod_line *line,
                                          const char *consumer);

/* Release line */
void gpiod_line_release(struct gpiod_line *line);

/* Read/write value */
int gpiod_line_get_value(struct gpiod_line *line);
int gpiod_line_set_value(struct gpiod_line *line, int value);

/* Event handling */
int gpiod_line_event_wait(struct gpiod_line *line,
                           const struct timespec *timeout);
int gpiod_line_event_read(struct gpiod_line *line,
                           struct gpiod_line_event *event);
```

### 14.2.6 Complete C Example - LED Blink

```c
#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define LED_CHIP    "gpiochip1"
#define LED_LINE    21
#define CONSUMER    "led-blink"

int main(int argc, char **argv)
{
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    int ret;
    int i;

    printf("LED Blink Example (libgpiod)\n\n");

    /* Open GPIO chip */
    chip = gpiod_chip_open_by_name(LED_CHIP);
    if (!chip) {
        perror("Failed to open chip");
        return 1;
    }

    /* Get GPIO line */
    line = gpiod_chip_get_line(chip, LED_LINE);
    if (!line) {
        fprintf(stderr, "Failed to get line\n");
        gpiod_chip_close(chip);
        return 1;
    }

    /* Request line as output, initial value LOW */
    ret = gpiod_line_request_output(line, CONSUMER, 0);
    if (ret < 0) {
        perror("Failed to request line");
        gpiod_chip_close(chip);
        return 1;
    }

    printf("Blinking LED on %s line %d\n", LED_CHIP, LED_LINE);
    printf("Press Ctrl+C to stop\n\n");

    /* Blink LED 20 times */
    for (i = 0; i < 20; i++) {
        printf("Blink %d\n", i + 1);

        /* LED ON */
        gpiod_line_set_value(line, 1);
        usleep(500000);  /* 500ms */

        /* LED OFF */
        gpiod_line_set_value(line, 0);
        usleep(500000);
    }

    /* Cleanup */
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    printf("\nDone!\n");

    return 0;
}
```

**Compilation:**

```bash
gcc -o led_blink led_blink.c -lgpiod
sudo ./led_blink
```

### 14.2.7 Complete C Example - Button with Events

```c
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BTN_CHIP    "gpiochip1"
#define BTN_LINE    17
#define CONSUMER    "button-monitor"

int main(int argc, char **argv)
{
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    struct gpiod_line_event event;
    struct timespec timeout = { 1, 0 };  /* 1 second */
    int ret;

    printf("Button Monitor (libgpiod)\n");
    printf("Press button to see events\n");
    printf("Press Ctrl+C to exit\n\n");

    /* Open chip */
    chip = gpiod_chip_open_by_name(BTN_CHIP);
    if (!chip) {
        perror("Failed to open chip");
        return 1;
    }

    /* Get line */
    line = gpiod_chip_get_line(chip, BTN_LINE);
    if (!line) {
        fprintf(stderr, "Failed to get line\n");
        gpiod_chip_close(chip);
        return 1;
    }

    /* Request line for event monitoring (both edges) */
    ret = gpiod_line_request_both_edges_events(line, CONSUMER);
    if (ret < 0) {
        perror("Failed to request events");
        gpiod_chip_close(chip);
        return 1;
    }

    printf("Monitoring button on %s line %d\n\n", BTN_CHIP, BTN_LINE);

    /* Main event loop */
    while (1) {
        /* Wait for event */
        ret = gpiod_line_event_wait(line, &timeout);

        if (ret < 0) {
            perror("Error waiting for event");
            break;
        }

        if (ret == 0) {
            /* Timeout - no event */
            continue;
        }

        /* Read event */
        ret = gpiod_line_event_read(line, &event);
        if (ret < 0) {
            perror("Error reading event");
            break;
        }

        /* Process event */
        printf("Event: ");

        if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
            printf("RISING EDGE  (Button PRESSED)\n");
        } else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
            printf("FALLING EDGE (Button RELEASED)\n");
        }

        printf("  Timestamp: %lld.%09ld\n",
               (long long)event.ts.tv_sec,
               event.ts.tv_nsec);
        printf("\n");
    }

    /* Cleanup */
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
```

### 14.2.8 Bulk Operations Example

```c
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>

#define CHIP_NAME   "gpiochip1"
#define NUM_LEDS    4
#define CONSUMER    "led-array"

int main(void)
{
    struct gpiod_chip *chip;
    struct gpiod_line_bulk bulk;
    unsigned int offsets[NUM_LEDS] = {21, 22, 23, 24};
    int values[NUM_LEDS];
    int ret;
    int pattern;

    printf("LED Array Example (Bulk Operations)\n\n");

    /* Open chip */
    chip = gpiod_chip_open_by_name(CHIP_NAME);
    if (!chip) {
        perror("Failed to open chip");
        return 1;
    }

    /* Get multiple lines */
    ret = gpiod_chip_get_lines(chip, offsets, NUM_LEDS, &bulk);
    if (ret < 0) {
        perror("Failed to get lines");
        gpiod_chip_close(chip);
        return 1;
    }

    /* Request all lines as outputs */
    ret = gpiod_line_request_bulk_output(&bulk, CONSUMER, NULL);
    if (ret < 0) {
        perror("Failed to request lines");
        gpiod_chip_close(chip);
        return 1;
    }

    printf("Running LED patterns...\n");

    /* Pattern 1: All on */
    printf("Pattern 1: All ON\n");
    values[0] = 1; values[1] = 1; values[2] = 1; values[3] = 1;
    gpiod_line_set_value_bulk(&bulk, values);
    sleep(1);

    /* Pattern 2: All off */
    printf("Pattern 2: All OFF\n");
    values[0] = 0; values[1] = 0; values[2] = 0; values[3] = 0;
    gpiod_line_set_value_bulk(&bulk, values);
    sleep(1);

    /* Pattern 3: Alternating */
    printf("Pattern 3: Alternating\n");
    values[0] = 1; values[1] = 0; values[2] = 1; values[3] = 0;
    gpiod_line_set_value_bulk(&bulk, values);
    sleep(1);

    /* Pattern 4: Running light */
    printf("Pattern 4: Running light\n");
    for (pattern = 0; pattern < NUM_LEDS; pattern++) {
        values[0] = (pattern == 0);
        values[1] = (pattern == 1);
        values[2] = (pattern == 2);
        values[3] = (pattern == 3);
        gpiod_line_set_value_bulk(&bulk, values);
        usleep(250000);
    }

    /* All off */
    values[0] = 0; values[1] = 0; values[2] = 0; values[3] = 0;
    gpiod_line_set_value_bulk(&bulk, values);

    /* Cleanup */
    gpiod_line_release_bulk(&bulk);
    gpiod_chip_close(chip);

    printf("Done!\n");

    return 0;
}
```

---

## Summary

In this part, we covered:

**Key Concepts:**

1. **Legacy Sysfs Interface (Deprecated)**
    - /sys/class/gpio/ structure
    - export/unexport mechanism
    - GPIO attributes (direction, value, edge, active_low)
    - poll() support for events
    - Kernel export functions
    - Complete examples (LED, button, LCD)
2. **Modern libgpiod Interface (Recommended)**
    - Character device /dev/gpiochipX
    - Command-line tools (gpiodetect, gpioinfo, gpioget, gpioset, gpiomon)
    - C library API
    - Resource management
    - Bulk operations
    - Event handling
3. **Migration Path**
    - Why libgpiod replaces sysfs
    - Advantages of modern interface
    - Installation and cross-compilation
    - Complete working examples
4. **Best Practices**
    - Use libgpiod for new projects
    - Proper error handling
    - Resource cleanup
    - Event-driven programming

**Chapter 14 Complete!** You now understand:

- Pin multiplexing and configuration
- Linux pinctrl subsystem
- GPIO consumer interfaces (legacy and modern)
- Kernel GPIO APIs (integer-based and descriptor-based)
- Userspace GPIO access (sysfs and libgpiod)
- Complete working examples for real hardware