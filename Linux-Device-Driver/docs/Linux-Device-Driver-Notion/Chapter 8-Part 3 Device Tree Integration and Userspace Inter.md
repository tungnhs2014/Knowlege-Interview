# Part 3. Device Tree Integration and Userspace Interface

This part covers SPI device declaration in Device Tree, OF-style driver matching, and the spidev userspace interface with complete working examples.

---

## 8.1 SPI Device Tree Fundamentals

### 8.1.1 SPI Device Addressing

**SPI devices are non-memory-mapped but addressable:**

```
┌────────────────────────────────────────────────┐
│  SPI Device Addressing in Device Tree          │
├────────────────────────────────────────────────┤
│                                                │
│  Address = Chip Select (CS) index              │
│                                                │
│  #address-cells = <1>;  ← 1 cell for address   │
│  #size-cells = <0>;     ← No size needed       │
│                                                │
│  reg = <N>;  ← CS index (0, 1, 2, ...)         │
│                                                │
│  Example:                                      │
│  device@0 { reg = <0>; }  ← Uses CS0           │
│  device@1 { reg = <1>; }  ← Uses CS1           │
│  device@2 { reg = <2>; }  ← Uses CS2           │
│                                                │
└────────────────────────────────────────────────┘
```

**Visual representation:**

```
SPI Master (ecspi1)
   │
   ├── cs-gpios = <&gpio5 17>, <&gpio5 18>, <&gpio5 19>;
   │               CS0=GPIO5_17  CS1=GPIO5_18  CS2=GPIO5_19
   │
   ├── flash@0 { reg = <0>; }  ← Uses CS0 (GPIO5_17)
   ├── adc@1   { reg = <1>; }  ← Uses CS1 (GPIO5_18)
   └── dac@2   { reg = <2>; }  ← Uses CS2 (GPIO5_19)
```

### 8.1.2 Complete SPI Controller Node

**Example for i.MX6 SoC (`imx6qdl.dtsi`):**

```
/* SPI controller definition in SoC .dtsi */
ecspi1: spi@02008000 {
    #address-cells = <1>;
    #size-cells = <0>;
    compatible = "fsl,imx6q-ecspi", "fsl,imx51-ecspi";
    reg = <0x02008000 0x4000>;
    interrupts = <0 31 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&clks IMX6QDL_CLK_ECSPI1>,
             <&clks IMX6QDL_CLK_ECSPI1>;
    clock-names = "ipg", "per";
    dmas = <&sdma 3 7 1>, <&sdma 4 7 2>;
    dma-names = "rx", "tx";
    status = "disabled";
};
```

**Field explanations:**

- `#address-cells = <1>`: Child nodes use 1 cell for address (CS index)
- `#size-cells = <0>`: No size needed for SPI devices
- `compatible`: Driver matching string
- `reg`: Controller register base and size
- `status = "disabled"`: Disabled by default, enabled in board DTS

---

## 8.2 SPI Device Declaration in Device Tree

### 8.2.1 Board-Level Device Tree

**Enable controller and add devices (`imx6-myboard.dts`):**

```
&ecspi1 {
    fsl,spi-num-chipselects = <3>;
    cs-gpios = <&gpio5 17 GPIO_ACTIVE_LOW>,
               <&gpio5 18 GPIO_ACTIVE_LOW>,
               <&gpio5 19 GPIO_ACTIVE_LOW>;
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_ecspi1>;
    status = "okay";

    /* Device 1: SPI Flash */
    flash@0 {
        compatible = "jedec,spi-nor";
        reg = <0>;  /* CS0 */
        spi-max-frequency = <20000000>;  /* 20 MHz */
        spi-cpol;
        spi-cpha;
    };

    /* Device 2: ADC */
    adc@1 {
        compatible = "microchip,mcp3008";
        reg = <1>;  /* CS1 */
        spi-max-frequency = <1000000>;  /* 1 MHz */
        vref-supply = <&vref_3v3>;
    };

    /* Device 3: CAN Controller */
    can@2 {
        compatible = "microchip,mcp2515";
        reg = <2>;  /* CS2 */
        spi-max-frequency = <10000000>;  /* 10 MHz */
        clocks = <&can_clock>;
        interrupt-parent = <&gpio4>;
        interrupts = <29 IRQ_TYPE_LEVEL_LOW>;
    };
};
```

### 8.2.2 SPI Device Properties

**Common SPI-specific properties:**

```
device@N {
    compatible = "vendor,device-name";
    reg = <N>;  /* CS index - REQUIRED */

    /* Speed (REQUIRED) */
    spi-max-frequency = <frequency_in_hz>;

    /* Mode flags (optional) */
    spi-cpol;        /* Clock polarity: idle high */
    spi-cpha;        /* Clock phase: sample on second edge */
    spi-cs-high;     /* CS active high instead of low */
    spi-lsb-first;   /* LSB first instead of MSB */
    spi-3wire;       /* 3-wire mode (MOSI=MISO) */

    /* Transfer settings (optional) */
    spi-tx-bus-width = <1>;  /* 1, 2, or 4 for dual/quad SPI */
    spi-rx-bus-width = <1>;

    /* Other device-specific properties */
    interrupt-parent = <&gpio>;
    interrupts = <pin IRQ_TYPE>;
    /* ... */
};
```

### 8.2.3 Real-World Examples

**Example 1: W25Q128 SPI Flash**

```
&spi1 {
    status = "okay";

    flash: w25q128@0 {
        compatible = "winbond,w25q128", "jedec,spi-nor";
        reg = <0>;
        spi-max-frequency = <50000000>;  /* 50 MHz */

        partitions {
            compatible = "fixed-partitions";
            #address-cells = <1>;
            #size-cells = <1>;

            partition@0 {
                label = "bootloader";
                reg = <0x0 0x40000>;  /* 256KB */
                read-only;
            };

            partition@40000 {
                label = "kernel";
                reg = <0x40000 0x3c0000>;  /* 3.75MB */
            };

            partition@400000 {
                label = "rootfs";
                reg = <0x400000 0xc00000>;  /* 12MB */
            };
        };
    };
};
```

**Example 2: AD7606 ADC (8-channel)**

```
&ecspi1 {
    status = "okay";

    adc: ad7606@0 {
        compatible = "adi,ad7606-8";
        reg = <0>;
        spi-max-frequency = <1000000>;
        spi-cpol;
        spi-cpha;

        interrupt-parent = <&gpio4>;
        interrupts = <30 IRQ_TYPE_EDGE_FALLING>;

        adi,conversion-start-gpios = <&gpio1 5 GPIO_ACTIVE_HIGH>;
        reset-gpios = <&gpio1 6 GPIO_ACTIVE_LOW>;
        standby-gpios = <&gpio1 7 GPIO_ACTIVE_HIGH>;

        vcc-supply = <&reg_5v0>;
    };
};
```

**Example 3: MCP2515 CAN Controller**

```
&spi2 {
    status = "okay";

    can0: mcp2515@0 {
        compatible = "microchip,mcp2515";
        reg = <0>;
        spi-max-frequency = <10000000>;

        clocks = <&can_osc>;
        interrupt-parent = <&gpio4>;
        interrupts = <29 IRQ_TYPE_LEVEL_LOW>;

        vdd-supply = <&reg_3v3>;
        xceiver-supply = <&reg_5v0>;
    };
};

/* External oscillator for CAN */
can_osc: can-clock {
    compatible = "fixed-clock";
    #clock-cells = <0>;
    clock-frequency = <16000000>;  /* 16 MHz */
};
```

---

## 8.3 OF-Style Driver Matching

### 8.3.1 OF Device ID Table

**Define match table:**

```c
#include <linux/of.h>

static const struct of_device_id my_spi_of_match[] = {
    {
        .compatible = "vendor,device-v1",
        .data = (void *)DEVICE_V1,  /* Optional driver data */
    },
    {
        .compatible = "vendor,device-v2",
        .data = (void *)DEVICE_V2,
    },
    { }  /* Sentinel */
};
MODULE_DEVICE_TABLE(of, my_spi_of_match);
```

### 8.3.2 Complete Driver with OF Support

```c
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_device.h>

/* Device variants */
enum device_type {
    DEVICE_V1,
    DEVICE_V2,
};

struct my_device {
    struct spi_device *spi;
    enum device_type type;
    u32 custom_param;
};

/* OF match table */
static const struct of_device_id my_spi_of_match[] = {
    {
        .compatible = "acme,spi-sensor-v1",
        .data = (void *)DEVICE_V1,
    },
    {
        .compatible = "acme,spi-sensor-v2",
        .data = (void *)DEVICE_V2,
    },
    { }
};
MODULE_DEVICE_TABLE(of, my_spi_of_match);

/* Legacy ID table (for non-DT systems) */
static const struct spi_device_id my_spi_id[] = {
    { "spi-sensor-v1", DEVICE_V1 },
    { "spi-sensor-v2", DEVICE_V2 },
    { }
};
MODULE_DEVICE_TABLE(spi, my_spi_id);

/* Parse Device Tree properties */
static int my_parse_dt(struct device *dev, struct my_device *priv)
{
    struct device_node *np = dev->of_node;
    int ret;

    /* Read custom property */
    ret = of_property_read_u32(np, "acme,custom-param",
                                &priv->custom_param);
    if (ret < 0) {
        /* Use default if not specified */
        priv->custom_param = 100;
    }

    dev_info(dev, "Custom param: %u\n", priv->custom_param);

    return 0;
}

static int my_spi_probe(struct spi_device *spi)
{
    const struct of_device_id *of_id;
    const struct spi_device_id *id;
    struct my_device *priv;
    int ret;

    dev_info(&spi->dev, "Probing device\n");

    /* Allocate private data */
    priv = devm_kzalloc(&spi->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->spi = spi;

    /* Check if probed via Device Tree */
    of_id = of_match_device(my_spi_of_match, &spi->dev);
    if (of_id) {
        dev_info(&spi->dev, "Matched via DT: %s\n", of_id->compatible);

        /* Get device type from .data */
        priv->type = (enum device_type)of_id->data;

        /* Parse DT properties */
        ret = my_parse_dt(&spi->dev, priv);
        if (ret < 0)
            return ret;

    } else {
        dev_info(&spi->dev, "Matched via legacy ID table\n");

        /* Get device ID */
        id = spi_get_device_id(spi);
        priv->type = id->driver_data;

        /* Use platform data if available */
        /* struct platform_data *pdata = spi->dev.platform_data; */
    }

    /* Configure SPI based on device type */
    switch (priv->type) {
    case DEVICE_V1:
        spi->mode = SPI_MODE_0;
        spi->max_speed_hz = 1000000;
        break;
    case DEVICE_V2:
        spi->mode = SPI_MODE_3;
        spi->max_speed_hz = 10000000;
        break;
    }

    spi->bits_per_word = 8;
    ret = spi_setup(spi);
    if (ret < 0)
        return ret;

    spi_set_drvdata(spi, priv);

    /* Device-specific initialization */
    /* ... */

    dev_info(&spi->dev, "Probe successful (type=%d)\n", priv->type);
    return 0;
}

static int my_spi_remove(struct spi_device *spi)
{
    struct my_device *priv = spi_get_drvdata(spi);

    dev_info(&spi->dev, "Removing device\n");

    /* Cleanup */

    return 0;
}

static struct spi_driver my_spi_driver = {
    .driver = {
        .name = "my-spi-driver",
        .of_match_table = of_match_ptr(my_spi_of_match),
    },
    .probe = my_spi_probe,
    .remove = my_spi_remove,
    .id_table = my_spi_id,
};

module_spi_driver(my_spi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("SPI Driver with OF Support");
```

### 8.3.3 Corresponding Device Tree Entry

```
&spi1 {
    status = "okay";

    sensor@0 {
        compatible = "acme,spi-sensor-v2";
        reg = <0>;
        spi-max-frequency = <10000000>;

        /* Custom property */
        acme,custom-param = <250>;
    };
};
```

---

## 8.4 Reading Device Tree Properties

### 8.4.1 Common OF APIs

```c
#include <linux/of.h>

/* Read u32 property */
int of_property_read_u32(const struct device_node *np,
                         const char *propname,
                         u32 *out_value);

/* Read string property */
int of_property_read_string(const struct device_node *np,
                             const char *propname,
                             const char **out_string);

/* Check if property exists */
bool of_property_read_bool(const struct device_node *np,
                            const char *propname);

/* Read array of u32 */
int of_property_read_u32_array(const struct device_node *np,
                                const char *propname,
                                u32 *out_values,
                                size_t sz);

/* Read u64 */
int of_property_read_u64(const struct device_node *np,
                         const char *propname,
                         u64 *out_value);
```

### 8.4.2 Complete Property Parsing Example

```c
struct sensor_config {
    u32 sample_rate;
    u32 resolution;
    const char *mode;
    bool enable_filter;
    u32 channels[8];
    u32 num_channels;
};

static int parse_sensor_config(struct device *dev,
                                struct sensor_config *cfg)
{
    struct device_node *np = dev->of_node;
    int ret;

    /* Read sample rate (required) */
    ret = of_property_read_u32(np, "sample-rate", &cfg->sample_rate);
    if (ret < 0) {
        dev_err(dev, "Missing 'sample-rate' property\n");
        return ret;
    }

    /* Read resolution (optional, default to 12) */
    ret = of_property_read_u32(np, "resolution", &cfg->resolution);
    if (ret < 0)
        cfg->resolution = 12;

    /* Read mode string */
    ret = of_property_read_string(np, "mode", &cfg->mode);
    if (ret < 0)
        cfg->mode = "normal";

    /* Check boolean flag */
    cfg->enable_filter = of_property_read_bool(np, "enable-filter");

    /* Read array of channels */
    cfg->num_channels = of_property_count_u32_elems(np, "channels");
    if (cfg->num_channels > 0 && cfg->num_channels <= 8) {
        ret = of_property_read_u32_array(np, "channels",
                                          cfg->channels,
                                          cfg->num_channels);
        if (ret < 0)
            cfg->num_channels = 0;
    }

    dev_info(dev, "Config: rate=%u, res=%u, mode=%s, filter=%d\n",
             cfg->sample_rate, cfg->resolution,
             cfg->mode, cfg->enable_filter);

    return 0;
}
```

**Corresponding DT:**

```
sensor@0 {
    compatible = "vendor,sensor";
    reg = <0>;
    spi-max-frequency = <5000000>;

    sample-rate = <1000>;     /* 1 kHz */
    resolution = <16>;        /* 16-bit */
    mode = "high-speed";
    enable-filter;            /* Boolean flag */
    channels = <0 1 2 3>;     /* Use channels 0-3 */
};
```

---

## 8.5 Backward Compatibility with Platform Data

### 8.5.1 Supporting Both DT and Platform Data

```c
/* Platform data structure (legacy) */
struct my_platform_data {
    u32 param1;
    u32 param2;
    bool flag;
};

static int my_probe(struct spi_device *spi)
{
    struct my_device *priv;
    u32 param1, param2;
    bool flag;

    priv = devm_kzalloc(&spi->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /* Try Device Tree first */
    if (spi->dev.of_node) {
        /* DT path */
        of_property_read_u32(spi->dev.of_node, "param1", &param1);
        of_property_read_u32(spi->dev.of_node, "param2", &param2);
        flag = of_property_read_bool(spi->dev.of_node, "flag");

    } else {
        /* Platform data path (legacy) */
        struct my_platform_data *pdata = dev_get_platdata(&spi->dev);

        if (!pdata) {
            dev_err(&spi->dev, "No platform data\n");
            return -EINVAL;
        }

        param1 = pdata->param1;
        param2 = pdata->param2;
        flag = pdata->flag;
    }

    /* Use parsed parameters */
    priv->param1 = param1;
    priv->param2 = param2;
    priv->flag = flag;

    /* Rest of probe... */
    return 0;
}
```

---

## 8.6 Userspace SPI - spidev

### 8.6.1 What is spidev?

**spidev is a generic SPI character device driver:**

```
┌────────────────────────────────────────────────┐
│  spidev - Userspace SPI Access                 │
├────────────────────────────────────────────────┤
│                                                │
│  Userspace Application                         │
│         ↕ (read/write/ioctl)                  │
│  /dev/spidevX.Y                                │
│         ↕                                     │
│  spidev.c (kernel driver)                      │
│         ↕                                     │
│  SPI Core                                      │
│         ↕                                     │
│  SPI Controller Driver                         │
│         ↕                                     │
│  Hardware SPI Bus                              │
│                                                │
└────────────────────────────────────────────────┘
```

**Device naming:**

- `/dev/spidev0.0` = Bus 0, CS 0
- `/dev/spidev0.1` = Bus 0, CS 1
- `/dev/spidev1.0` = Bus 1, CS 0

### 8.6.2 Enabling spidev in Device Tree

```
&spi1 {
    status = "okay";

    spidev@0 {
        compatible = "spidev";  /* Generic spidev */
        reg = <0>;              /* CS0 */
        spi-max-frequency = <800000>;  /* 800 kHz */
    };

    spidev@1 {
        compatible = "spidev";
        reg = <1>;              /* CS1 */
        spi-max-frequency = <1000000>;  /* 1 MHz */
    };
};
```

**Alternative compatible strings:**

```
compatible = "rohm,dh2228fv";  /* Allowed spidev compatible */
compatible = "lineartechnology,ltc2488";
compatible = "ge,achc";
```

### 8.6.3 Userspace API

**Include header:**

```c
#include <linux/spi/spidev.h>
```

**IOCTL commands:**

```c
/* Read/Write mode */
#define SPI_IOC_RD_MODE          _IOR(SPI_IOC_MAGIC, 1, __u8)
#define SPI_IOC_WR_MODE          _IOW(SPI_IOC_MAGIC, 1, __u8)

/* Read/Write bits per word */
#define SPI_IOC_RD_BITS_PER_WORD _IOR(SPI_IOC_MAGIC, 3, __u8)
#define SPI_IOC_WR_BITS_PER_WORD _IOW(SPI_IOC_MAGIC, 3, __u8)

/* Read/Write max speed */
#define SPI_IOC_RD_MAX_SPEED_HZ  _IOR(SPI_IOC_MAGIC, 4, __u32)
#define SPI_IOC_WR_MAX_SPEED_HZ  _IOW(SPI_IOC_MAGIC, 4, __u32)

/* Read/Write LSB first */
#define SPI_IOC_RD_LSB_FIRST     _IOR(SPI_IOC_MAGIC, 2, __u8)
#define SPI_IOC_WR_LSB_FIRST     _IOW(SPI_IOC_MAGIC, 2, __u8)

/* Full-duplex transfer */
#define SPI_IOC_MESSAGE(N) \
    _IOW(SPI_IOC_MAGIC, 0, char[SPI_MSGSIZE(N)])
```

---

## 8.7 Userspace Examples

### 8.7.1 Simple Read/Write Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>

int main(int argc, char **argv)
{
    int fd;
    int ret;
    uint8_t tx_buf[] = {0xFF, 0x00, 0x1F, 0x0F};
    uint8_t rx_buf[10];

    /* Open SPI device */
    fd = open("/dev/spidev0.0", O_RDWR);
    if (fd < 0) {
        perror("Failed to open SPI device");
        return 1;
    }

    /* Write data */
    ret = write(fd, tx_buf, sizeof(tx_buf));
    if (ret != sizeof(tx_buf)) {
        perror("Write error");
        close(fd);
        return 1;
    }

    printf("Wrote %d bytes\n", ret);

    /* Read data */
    ret = read(fd, rx_buf, sizeof(rx_buf));
    if (ret != sizeof(rx_buf)) {
        perror("Read error");
        close(fd);
        return 1;
    }

    printf("Read %d bytes: ", ret);
    for (int i = 0; i < ret; i++)
        printf("0x%02X ", rx_buf[i]);
    printf("\n");

    close(fd);
    return 0;
}
```

### 8.7.2 Configure SPI Settings

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>

static int spi_device_setup(int fd)
{
    int ret;
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 8000000;  /* 8 MHz */
    uint8_t lsb = 0;           /* MSB first */

    /* Set SPI mode */
    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (ret < 0) {
        perror("Failed to set SPI mode");
        return -1;
    }

    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    if (ret < 0) {
        perror("Failed to read SPI mode");
        return -1;
    }
    printf("SPI Mode: 0x%02x\n", mode);

    /* Set bits per word */
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret < 0) {
        perror("Failed to set bits per word");
        return -1;
    }

    /* Set max speed */
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret < 0) {
        perror("Failed to set max speed");
        return -1;
    }

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret < 0) {
        perror("Failed to read max speed");
        return -1;
    }
    printf("Max speed: %u Hz (%u MHz)\n", speed, speed / 1000000);

    /* Set MSB first */
    ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &lsb);
    if (ret < 0) {
        perror("Failed to set MSB first");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int fd;

    if (argc < 2) {
        printf("Usage: %s /dev/spidevX.Y\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    if (spi_device_setup(fd) < 0) {
        close(fd);
        return 1;
    }

    printf("SPI device configured successfully\n");

    close(fd);
    return 0;
}
```

### 8.7.3 Full-Duplex Transfer with IOCTL

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>

static void do_transfer(int fd)
{
    int ret;
    uint8_t tx_buf[] = {0x0B, 0x02, 0xB5};
    uint8_t rx_buf[3] = {0};
    uint8_t cmd_buf = 0x9F;

    struct spi_ioc_transfer tr[2] = {
        {
            /* Transfer 1: Send command */
            .tx_buf = (unsigned long)&cmd_buf,
            .rx_buf = 0,
            .len = 1,
            .cs_change = 1,        /* Deassert CS after */
            .delay_usecs = 50,     /* 50µs delay */
            .bits_per_word = 8,
        },
        {
            /* Transfer 2: Send/receive data */
            .tx_buf = (unsigned long)tx_buf,
            .rx_buf = (unsigned long)rx_buf,
            .len = sizeof(tx_buf),
            .bits_per_word = 8,
        },
    };

    /* Execute both transfers atomically */
    ret = ioctl(fd, SPI_IOC_MESSAGE(2), &tr);
    if (ret < 0) {
        perror("SPI transfer failed");
        return;
    }

    printf("Received: ");
    for (int i = 0; i < sizeof(rx_buf); i++)
        printf("0x%02X ", rx_buf[i]);
    printf("\n");
}

int main(int argc, char **argv)
{
    int fd;
    int error;

    if (argc < 2) {
        printf("Usage: %s /dev/spidevX.Y\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    /* Setup (from previous example) */
    error = spi_device_setup(fd);
    if (error) {
        close(fd);
        return 1;
    }

    /* Do transfer */
    do_transfer(fd);

    close(fd);
    return 0;
}
```

### 8.7.4 Complete Flash Read Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>

#define CMD_READ_ID     0x9F
#define CMD_READ_DATA   0x03

static int read_flash_id(int fd, uint8_t *id)
{
    struct spi_ioc_transfer tr[2] = {
        {
            .tx_buf = (unsigned long)(uint8_t[]){CMD_READ_ID},
            .len = 1,
        },
        {
            .rx_buf = (unsigned long)id,
            .len = 3,
        },
    };

    return ioctl(fd, SPI_IOC_MESSAGE(2), tr);
}

static int read_flash_data(int fd, uint32_t addr,
                            uint8_t *data, size_t len)
{
    uint8_t cmd[4] = {
        CMD_READ_DATA,
        (addr >> 16) & 0xFF,
        (addr >> 8) & 0xFF,
        addr & 0xFF,
    };

    struct spi_ioc_transfer tr[2] = {
        {
            .tx_buf = (unsigned long)cmd,
            .len = 4,
        },
        {
            .rx_buf = (unsigned long)data,
            .len = len,
        },
    };

    return ioctl(fd, SPI_IOC_MESSAGE(2), tr);
}

int main(int argc, char **argv)
{
    int fd;
    uint8_t id[3];
    uint8_t data[256];
    int ret;

    fd = open("/dev/spidev0.0", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    /* Setup device */
    uint8_t mode = SPI_MODE_0;
    uint32_t speed = 10000000;  /* 10 MHz */
    uint8_t bits = 8;

    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);

    /* Read flash ID */
    ret = read_flash_id(fd, id);
    if (ret < 0) {
        perror("Failed to read ID");
        close(fd);
        return 1;
    }

    printf("Flash ID: %02X %02X %02X\n", id[0], id[1], id[2]);

    /* Read 256 bytes from address 0x0000 */
    ret = read_flash_data(fd, 0x0000, data, sizeof(data));
    if (ret < 0) {
        perror("Failed to read data");
        close(fd);
        return 1;
    }

    printf("Data at 0x0000:\n");
    for (int i = 0; i < 256; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }

    close(fd);
    return 0;
}
```

### 8.7.5 Compilation

```bash
# Compile
gcc -o spi_test spi_test.c

# Run
sudo ./spi_test /dev/spidev0.0
```

---

## 8.8 Driver Writing Checklist

### 8.8.1 Complete SPI Driver Checklist

**Step 1: Define device IDs**

```c
/* OF match table */
static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,device", },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);

/* Legacy ID table */
static const struct spi_device_id my_id[] = {
    { "device", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, my_id);
```

**Step 2: Write probe function**

```c
static int my_probe(struct spi_device *spi)
{
    /* 1. Configure SPI */
    spi->mode = SPI_MODE_0;
    spi->max_speed_hz = 10000000;
    spi->bits_per_word = 8;
    spi_setup(spi);

    /* 2. Allocate private data */
    /* 3. Parse DT properties */
    /* 4. Initialize device */
    /* 5. Register with framework */

    return 0;
}
```

**Step 3: Write remove function**

```c
static int my_remove(struct spi_device *spi)
{
    /* Undo everything from probe */
    return 0;
}
```

**Step 4: Define driver structure**

```c
static struct spi_driver my_driver = {
    .driver = {
        .name = "my-driver",
        .of_match_table = my_of_match,
    },
    .probe = my_probe,
    .remove = my_remove,
    .id_table = my_id,
};
```

**Step 5: Register driver**

```c
module_spi_driver(my_driver);
```

---

## Summary

In this part, we covered:

**Key Concepts:**

1. **Device Tree for SPI**
    - #address-cells, #size-cells
    - reg property = CS index
    - spi-max-frequency
    - Mode flags (spi-cpol, spi-cpha, etc.)
2. **OF-Style Matching**
    - struct of_device_id
    - MODULE_DEVICE_TABLE(of, ...)
    - of_match_device()
    - Property parsing APIs
3. **Backward Compatibility**
    - Supporting both DT and platform_data
    - Legacy ID table fallback
4. **spidev Userspace Interface**
    - /dev/spidevX.Y character device
    - IOCTL configuration
    - read/write operations
    - Full-duplex transfers
5. **Complete Examples**
    - Real Device Tree nodes
    - Full OF driver implementation
    - Userspace C programs
    - Flash read/write operations

**Chapter 8 Complete!** You now have comprehensive knowledge of:

- SPI protocol and architecture
- Linux SPI subsystem
- Driver development (probe/remove/transfer)
- Device Tree integration
- Userspace access via spidev