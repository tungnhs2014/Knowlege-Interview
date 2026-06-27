# Part 2. Device Addressing and Resources

This part explains how devices are addressed in the Device Tree and how various resources (memory, interrupts, clocks, GPIOs, DMA) are described and accessed.

---

## 6.2 Device Addressing and Resources

### 6.2.1 Understanding Address Cells

Each addressable device in a Device Tree gets a `reg` property that describes its address and size. But before we can interpret `reg`, we need to understand **address cells** and **size cells**.

**The key properties:**

```
#address-cells = <n>  →  Number of u32 cells for address
#size-cells = <m>     →  Number of u32 cells for size
```

**How they work together:**

```
If #address-cells = <1> and #size-cells = <1>:
    reg = <address size>
    reg = <0x02020000 0x4000>
          └─ 1 cell ─┘ └─ 1 cell ─┘

If #address-cells = <2> and #size-cells = <2>:
    reg = <address_high address_low size_high size_low>
    reg = <0x0 0x80000000 0x0 0x10000000>
          └─── 2 cells ───┘ └──── 2 cells ────┘
          64-bit address     64-bit size
```

**Important rules:**

1. **Inherited from parent**: Child nodes inherit `#address-cells` and `#size-cells` from their **parent** node
2. **Property of parent**: These properties in a node affect how to interpret `reg` in its **children**, not itself
3. **Bus-specific**: Different bus types use different values

**Memory layout explanation:**

```
┌─────────────────────────────────────────────────┐
│  32-bit System (ARM, older x86)                 │
│  #address-cells = <1>    (32-bit addresses)     │
│  #size-cells = <1>       (32-bit sizes)         │
├─────────────────────────────────────────────────┤
│  64-bit System (ARM64, x86_64)                  │
│  #address-cells = <2>    (64-bit addresses)     │
│  #size-cells = <2>       (64-bit sizes)         │
├─────────────────────────────────────────────────┤
│  I2C/SPI Devices (non-memory-mapped)            │
│  #address-cells = <1>    (device address)       │
│  #size-cells = <0>       (size not relevant)    │
└─────────────────────────────────────────────────┘
```

**Example with 32-bit addressing:**

```
soc {
    #address-cells = <1>;  /* Children use 32-bit addresses */
    #size-cells = <1>;     /* Children use 32-bit sizes */
    compatible = "simple-bus";
    ranges;

    uart1: serial@02020000 {
        compatible = "fsl,imx6q-uart";
        /* address=0x02020000, size=0x4000 (16 KB) */
        reg = <0x02020000 0x4000>;
        /*      └─ 1 u32 ─┘ └─ 1 u32 ─┘  */
    };

    ethernet@02188000 {
        compatible = "fsl,imx6q-fec";
        /* address=0x02188000, size=0x4000 */
        reg = <0x02188000 0x4000>;
    };
};
```

**Example with 64-bit addressing:**

```
/ {
    #address-cells = <2>;  /* 64-bit addresses */
    #size-cells = <2>;     /* 64-bit sizes */

    memory@0 {
        device_type = "memory";
        /* address=0x0000000000000000, size=0x0000000020000000 (512 MB) */
        reg = <0x0 0x00000000 0x0 0x20000000>;
        /*      └─ high ─┘└─ low ──┘ └─ high ─┘└─ low ──┘  */
    };
};
```

---

## 6.3 The reg Property

The `reg` property is the **main addressing property** and its meaning depends on the bus the device sits on.

### 6.3.1 Memory-Mapped Devices (MMIO)

For memory-mapped devices, `reg` specifies the **base address** and **size** of memory regions.

**Format:**

```
reg = <base_addr size>;

/* Multiple regions */
reg = <base_addr1 size1>, <base_addr2 size2>;
```

**Complete example:**

```
soc {
    #address-cells = <1>;
    #size-cells = <1>;
    compatible = "simple-bus";
    ranges;

    /* Single memory region */
    uart1: serial@02020000 {
        compatible = "fsl,imx6q-uart";
        reg = <0x02020000 0x4000>;
        /* Base: 0x02020000
         * Size: 0x4000 (16 KB)
         * End:  0x02023fff
         */
    };

    /* Multiple memory regions */
    sai4: audio@50027000 {
        compatible = "fsl,imx8mm-sai";
        reg = <0x50027000 0x4>,      /* Control registers */
              <0x500273f0 0x10>;     /* FIFO registers */
        reg-names = "control", "fifo";
    };
};
```

**Extracting in driver:**

```c
static int uart_probe(struct platform_device *pdev)
{
    struct resource *res;
    void __iomem *base;
    resource_size_t size;

    /* Get memory resource from reg property */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
        return -ENODEV;

    /* res->start = 0x02020000 (from first cell of reg) */
    /* res->end   = 0x02023fff (calculated from size) */
    size = resource_size(res);  /* 0x4000 */

    /* Map the memory region */
    base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(base))
        return PTR_ERR(base);

    /* Now can access registers at 'base' */
    writel(value, base + UART_CONTROL_REG);

    return 0;
}
```

### 6.3.2 I2C Devices Addressing

For I2C devices, `reg` contains the **I2C slave address** on the bus.

**I2C addressing rules:**

```
#address-cells = <1>  →  reg is I2C address
#size-cells = <0>     →  size not relevant
```

**Example:**

```
&i2c1 {
    #address-cells = <1>;
    #size-cells = <0>;
    clock-frequency = <100000>;
    status = "okay";

    /* Temperature sensor at address 0x49 */
    temperature-sensor@49 {
        compatible = "national,lm73";
        reg = <0x49>;  /* I2C slave address */
    };

    /* RTC at address 0x68 */
    pcf8523: rtc@68 {
        compatible = "nxp,pcf8523";
        reg = <0x68>;  /* I2C slave address */
    };

    /* EEPROM at address 0x50 */
    eeprom@50 {
        compatible = "atmel,24c02";
        reg = <0x50>;
        pagesize = <16>;
    };

    /* Audio codec at address 0x0a */
    codec: sgtl5000@0a {
        compatible = "fsl,sgtl5000";
        reg = <0x0a>;
        clocks = <&clks IMX6QDL_CLK_CKO>;
        VDDA-supply = <&reg_2p5v>;
    };
};
```

**Driver access:**

```c
static int lm73_probe(struct i2c_client *client,
                     const struct i2c_device_id *id)
{
    /* client->addr contains the I2C address (0x49) */
    dev_info(&client->dev, "I2C address: 0x%02x\n", client->addr);

    /* Read from I2C device */
    int temperature = i2c_smbus_read_word_data(client, LM73_REG_TEMP);

    return 0;
}
```

### 6.3.3 SPI Devices Addressing

For SPI devices, `reg` contains the **chip select (CS) line number**.

**SPI addressing rules:**

```
#address-cells = <1>  →  reg is CS index
#size-cells = <0>     →  size not relevant
```

**Complete SPI example:**

```
&ecspi1 {
    #address-cells = <1>;
    #size-cells = <0>;
    fsl,spi-num-chipselects = <3>;
    cs-gpios = <&gpio5 17 GPIO_ACTIVE_LOW>,   /* CS0 */
               <&gpio5 18 GPIO_ACTIVE_LOW>,   /* CS1 */
               <&gpio5 19 GPIO_ACTIVE_LOW>;   /* CS2 */
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_ecspi1>;
    status = "okay";

    /* ADC on CS0 (first chip select) */
    ad7606r8@0 {
        compatible = "adi,ad7606-8";
        reg = <0>;  /* Uses cs-gpios[0] = gpio5 17 */
        spi-max-frequency = <1000000>;
        interrupt-parent = <&gpio4>;
        interrupts = <30 IRQ_TYPE_EDGE_FALLING>;
    };

    /* Flash on CS1 (second chip select) */
    flash@1 {
        compatible = "jedec,spi-nor";
        reg = <1>;  /* Uses cs-gpios[1] = gpio5 18 */
        spi-max-frequency = <20000000>;
    };

    /* CAN controller on CS2 (third chip select) */
    can@2 {
        compatible = "microchip,mcp2515";
        reg = <2>;  /* Uses cs-gpios[2] = gpio5 19 */
        spi-max-frequency = <10000000>;
        interrupt-parent = <&gpio4>;
        interrupts = <29 IRQ_TYPE_LEVEL_LOW>;
        clocks = <&clk8m>;
    };
};
```

**How it works:**

```
reg = <0>  →  cs-gpios[0]  →  &gpio5 17
reg = <1>  →  cs-gpios[1]  →  &gpio5 18
reg = <2>  →  cs-gpios[2]  →  &gpio5 19
```

**Driver access:**

```c
static int spi_device_probe(struct spi_device *spi)
{
    /* spi->chip_select contains the CS index (0, 1, or 2) */
    dev_info(&spi->dev, "Chip select: %u\n", spi->chip_select);
    dev_info(&spi->dev, "Max speed: %u Hz\n", spi->max_speed_hz);

    /* Perform SPI transfer */
    struct spi_transfer xfer = {
        .tx_buf = tx_data,
        .rx_buf = rx_data,
        .len = 4,
    };

    spi_sync_transfer(spi, &xfer, 1);

    return 0;
}
```

### 6.3.4 Platform Device Addressing (simple-bus)

Devices under `simple-bus` are memory-mapped platform devices.

**Complete example:**

```
soc {
    compatible = "simple-bus";
    #address-cells = <1>;
    #size-cells = <1>;
    ranges;  /* 1:1 mapping with parent address space */

    aips1: aips-bus@02000000 {
        compatible = "fsl,aips-bus", "simple-bus";
        #address-cells = <1>;
        #size-cells = <1>;
        reg = <0x02000000 0x100000>;
        ranges;

        ecspi1: spi@02008000 {
            #address-cells = <1>;
            #size-cells = <0>;
            compatible = "fsl,imx6q-ecspi";
            reg = <0x02008000 0x4000>;
            interrupts = <0 31 IRQ_TYPE_LEVEL_HIGH>;
            clocks = <&clks IMX6QDL_CLK_ECSPI1>,
                     <&clks IMX6QDL_CLK_ECSPI1>;
            clock-names = "ipg", "per";
        };

        i2c1: i2c@021a0000 {
            #address-cells = <1>;
            #size-cells = <0>;
            compatible = "fsl,imx6q-i2c", "fsl,imx21-i2c";
            reg = <0x021a0000 0x4000>;
            interrupts = <0 36 IRQ_TYPE_LEVEL_HIGH>;
            clocks = <&clks IMX6QDL_CLK_I2C1>;
        };
    };
};
```

**Address translation:**

```
Device: i2c@021a0000
├─ Node address: 0x021a0000
├─ Parent (aips1): ranges (1:1 mapping)
├─ Parent (soc): ranges (1:1 mapping)
└─ Final CPU address: 0x021a0000
```

---

## 6.4 Interrupt Handling in Device Tree

Interrupts are described with four key properties:

```
Controller side:              Consumer side:
- interrupt-controller        - interrupt-parent
- #interrupt-cells            - interrupts
```

### 6.4.1 Interrupt Controller Properties

**Interrupt controller example:**

```
intc: interrupt-controller@a0021000 {
    compatible = "arm,cortex-a7-gic";

    /* Mark this as interrupt controller */
    interrupt-controller;

    /* Number of cells needed to specify an interrupt */
    #interrupt-cells = <3>;

    /* Controller's own registers */
    reg = <0xa0021000 0x1000>,   /* Distributor */
          <0xa0022000 0x2000>;   /* CPU interface */
};
```

**The #interrupt-cells property:**

This specifies how many cells are needed to describe an interrupt. The meaning of each cell is **controller-specific**.

**For ARM GIC (Generic Interrupt Controller):**

```
#interrupt-cells = <3>

interrupts = <type number flags>;

Cell 0 (type):
  0 = SPI (Shared Peripheral Interrupt)
  1 = PPI (Private Peripheral Interrupt)

Cell 1 (number):
  Interrupt number within the type

Cell 2 (flags):
  Trigger type and polarity
  IRQ_TYPE_EDGE_RISING   = 1
  IRQ_TYPE_EDGE_FALLING  = 2
  IRQ_TYPE_LEVEL_HIGH    = 4
  IRQ_TYPE_LEVEL_LOW     = 8
```

**Visual representation:**

```
┌──────────────────────────────────────────────┐
│  ARM GIC Interrupt Specifier                 │
│                                              │
│  interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;    │
│                │ │  └─ Trigger: Level High   │
│                │ └──── IRQ number: 26        │
│                └────── Type: SPI (0)         │
└──────────────────────────────────────────────┘
```

### 6.4.2 Interrupt Consumer Properties

**Device using interrupts:**

```
uart1: serial@02020000 {
    compatible = "fsl,imx6q-uart";
    reg = <0x02020000 0x4000>;

    /* Which interrupt controller? */
    interrupt-parent = <&intc>;

    /* Interrupt specifier (format defined by parent's #interrupt-cells) */
    interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
    /*            │ │  └─ Level-sensitive, active high
     *            │ └──── Interrupt number 26
     *            └────── SPI interrupt
     */

    clocks = <&clks IMX6QDL_CLK_UART_IPG>,
             <&clks IMX6QDL_CLK_UART_SERIAL>;
    clock-names = "ipg", "per";
};
```

**If interrupt-parent is omitted:**

The device inherits `interrupt-parent` from its parent node:

```
soc {
    compatible = "simple-bus";
    /* All children inherit this interrupt-parent */
    interrupt-parent = <&intc>;

    uart1: serial@02020000 {
        /* No need to specify interrupt-parent */
        interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
    };

    uart2: serial@021e8000 {
        /* Also inherits from soc node */
        interrupts = <0 27 IRQ_TYPE_LEVEL_HIGH>;
    };
};
```

### 6.4.3 Multiple Interrupts

Devices can have multiple interrupt lines:

```
ethernet@02188000 {
    compatible = "fsl,imx6q-fec";
    reg = <0x02188000 0x4000>;

    interrupt-parent = <&intc>;
    interrupts = <0 118 IRQ_TYPE_LEVEL_HIGH>,  /* TX interrupt */
                 <0 119 IRQ_TYPE_LEVEL_HIGH>;  /* RX interrupt */

    /* Optional: name them for clarity */
    interrupt-names = "tx", "rx";
};
```

**Extracting in driver:**

```c
static int fec_probe(struct platform_device *pdev)
{
    int tx_irq, rx_irq;
    int ret;

    /* Get interrupts by index */
    tx_irq = platform_get_irq(pdev, 0);  /* First interrupt */
    rx_irq = platform_get_irq(pdev, 1);  /* Second interrupt */

    if (tx_irq < 0 || rx_irq < 0)
        return -EINVAL;

    /* Or get by name (if interrupt-names provided) */
    tx_irq = platform_get_irq_byname(pdev, "tx");
    rx_irq = platform_get_irq_byname(pdev, "rx");

    /* Request IRQ handlers */
    ret = devm_request_irq(&pdev->dev, tx_irq, fec_tx_interrupt,
                          0, "fec-tx", priv);
    if (ret)
        return ret;

    ret = devm_request_irq(&pdev->dev, rx_irq, fec_rx_interrupt,
                          0, "fec-rx", priv);

    return ret;
}

static irqreturn_t fec_tx_interrupt(int irq, void *dev_id)
{
    /* Handle TX interrupt */
    pr_info("TX interrupt triggered\n");
    return IRQ_HANDLED;
}

static irqreturn_t fec_rx_interrupt(int irq, void *dev_id)
{
    /* Handle RX interrupt */
    pr_info("RX interrupt triggered\n");
    return IRQ_HANDLED;
}
```

### 6.4.4 Extended Interrupts (interrupts-extended)

For devices with interrupts from **multiple controllers**:

```
device {
    /* Standard way - single controller */
    interrupt-parent = <&intc>;
    interrupts = <0 56 IRQ_TYPE_LEVEL_HIGH>;

    /* Extended way - multiple controllers */
    interrupts-extended = <&intc 0 56 IRQ_TYPE_LEVEL_HIGH>,
                         <&gpio_intc 5 IRQ_TYPE_EDGE_RISING>;
    /*                    └─ controller 1 ─┘ └── controller 2 ──┘ */
};
```

---

## 6.5 Clock References

Clocks are described using phandles and clock specifiers.

### 6.5.1 Clock Provider

**Clock controller:**

```
clk: clock-controller@020c4000 {
    compatible = "fsl,imx6q-ccm";
    reg = <0x020c4000 0x4000>;

    /* This device provides clocks */
    #clock-cells = <1>;
    /*             └─ Number of cells needed to identify a clock */
};
```

**The #clock-cells property:**

```
#clock-cells = <1>  →  Need 1 cell (clock ID) to identify clock
#clock-cells = <0>  →  Device provides only one clock (no ID needed)
```

**Clock IDs are defined in header files:**

```c
/* include/dt-bindings/clock/imx6qdl-clock.h */
#define IMX6QDL_CLK_UART_IPG        15
#define IMX6QDL_CLK_UART_SERIAL     16
#define IMX6QDL_CLK_ECSPI1          19
#define IMX6QDL_CLK_I2C1            22
```

### 6.5.2 Clock Consumer

**Device consuming clocks:**

```
uart1: serial@02020000 {
    compatible = "fsl,imx6q-uart";
    reg = <0x02020000 0x4000>;

    /* Clock references */
    clocks = <&clk IMX6QDL_CLK_UART_IPG>,
             <&clk IMX6QDL_CLK_UART_SERIAL>;
    /*        └─controller─┘ └─ clock ID ─┘ */

    /* Clock names for driver */
    clock-names = "ipg", "per";
};
```

**Clock specifier format:**

```
clocks = <&clk_controller clock_id>;

Example:
clocks = <&clk 15>, <&clk 16>;
         └─ phandle to controller
              └─ clock ID (from #clock-cells)
```

**Extracting clocks in driver:**

```c
static int uart_probe(struct platform_device *pdev)
{
    struct clk *ipg_clk, *per_clk;
    int ret;

    /* Get clocks by name */
    ipg_clk = devm_clk_get(&pdev->dev, "ipg");
    if (IS_ERR(ipg_clk))
        return PTR_ERR(ipg_clk);

    per_clk = devm_clk_get(&pdev->dev, "per");
    if (IS_ERR(per_clk))
        return PTR_ERR(per_clk);

    /* Prepare and enable clocks */
    ret = clk_prepare_enable(ipg_clk);
    if (ret)
        return ret;

    ret = clk_prepare_enable(per_clk);
    if (ret) {
        clk_disable_unprepare(ipg_clk);
        return ret;
    }

    /* Get clock rate */
    unsigned long rate = clk_get_rate(per_clk);
    dev_info(&pdev->dev, "UART clock rate: %lu Hz\n", rate);

    return 0;
}
```

### 6.5.3 Multiple Clock Providers

Device can get clocks from different providers:

```
device {
    clocks = <&clk_controller1 5>,
             <&clk_controller2 10>,
             <&fixed_clock>;
    clock-names = "core", "bus", "ref";
};
```

---

## 6.6 GPIO References

GPIOs work similarly to clocks but with GPIO-specific properties.

### 6.6.1 GPIO Controller

**GPIO controller:**

```
gpio1: gpio@0209c000 {
    compatible = "fsl,imx6q-gpio", "fsl,imx35-gpio";
    reg = <0x0209c000 0x4000>;
    interrupts = <0 66 IRQ_TYPE_LEVEL_HIGH>,
                 <0 67 IRQ_TYPE_LEVEL_HIGH>;

    /* This is a GPIO controller */
    gpio-controller;

    /* GPIO specifier needs 2 cells */
    #gpio-cells = <2>;
    /*             └─ pin number (cell 0)
     *                └─ flags (cell 1)
     */

    /* Can also be interrupt controller */
    interrupt-controller;
    #interrupt-cells = <2>;
};
```

**The #gpio-cells property:**

```
#gpio-cells = <2>

Format: <pin_number flags>

Cell 0: GPIO pin number (0-31 typically)
Cell 1: Flags
  GPIO_ACTIVE_HIGH  = 0
  GPIO_ACTIVE_LOW   = 1
  GPIO_OPEN_DRAIN   = 2
  GPIO_OPEN_SOURCE  = 4
```

### 6.6.2 GPIO Consumer

**Device using GPIOs:**

```
device {
    compatible = "vendor,device";

    /* Single GPIO */
    reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
    /*             └─controller
     *                    └─ pin 7
     *                       └─ active low
     */

    /* Multiple GPIOs */
    led-gpios = <&gpio2 15 GPIO_ACTIVE_HIGH>,
                <&gpio2 16 GPIO_ACTIVE_HIGH>;

    /* Different controllers */
    control-gpios = <&gpio1 5 GPIO_ACTIVE_HIGH>,
                    <&gpio3 10 GPIO_ACTIVE_LOW>;
};
```

**Common GPIO properties:**

```
reset-gpios      /* Reset signal */
enable-gpios     /* Enable signal */
power-gpios      /* Power control */
cd-gpios         /* Card detect */
wp-gpios         /* Write protect */
led-gpios        /* LED control */
```

**Extracting GPIOs in driver:**

```c
#include <linux/gpio/consumer.h>
#include <linux/of_gpio.h>

static int device_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct gpio_desc *reset_gpio, *enable_gpio;
    int reset_gpio_num;
    int ret;

    /* Method 1: Using GPIO descriptor API (preferred) */
    reset_gpio = devm_gpiod_get(&pdev->dev, "reset", GPIOD_OUT_LOW);
    if (IS_ERR(reset_gpio))
        return PTR_ERR(reset_gpio);

    enable_gpio = devm_gpiod_get(&pdev->dev, "enable", GPIOD_OUT_HIGH);
    if (IS_ERR(enable_gpio))
        return PTR_ERR(enable_gpio);

    /* Use GPIOs */
    gpiod_set_value(reset_gpio, 1);   /* Assert reset */
    msleep(10);
    gpiod_set_value(reset_gpio, 0);   /* Deassert reset */

    gpiod_set_value(enable_gpio, 1);  /* Enable device */

    /* Method 2: Using OF GPIO API (older) */
    reset_gpio_num = of_get_named_gpio(np, "reset-gpios", 0);
    if (gpio_is_valid(reset_gpio_num)) {
        ret = devm_gpio_request_one(&pdev->dev, reset_gpio_num,
                                   GPIOF_OUT_INIT_LOW, "reset");
        if (ret)
            return ret;

        /* Toggle GPIO */
        gpio_set_value(reset_gpio_num, 1);
        msleep(10);
        gpio_set_value(reset_gpio_num, 0);
    }

    /* Get multiple GPIOs (LED example) */
    int led_count = of_gpio_named_count(np, "led-gpios");
    for (int i = 0; i < led_count; i++) {
        int gpio = of_get_named_gpio(np, "led-gpios", i);
        if (gpio_is_valid(gpio)) {
            devm_gpio_request_one(&pdev->dev, gpio,
                                 GPIOF_OUT_INIT_LOW,
                                 "led");
        }
    }

    return 0;
}
```

### 6.6.3 GPIO as Interrupt Source

GPIOs can also generate interrupts:

```
gpio1: gpio@0209c000 {
    gpio-controller;
    #gpio-cells = <2>;

    /* Also interrupt controller */
    interrupt-controller;
    #interrupt-cells = <2>;
};

device {
    interrupt-parent = <&gpio1>;
    interrupts = <7 IRQ_TYPE_EDGE_FALLING>;
    /* GPIO 7, falling edge */
};
```

---

## 6.7 DMA References

DMA channels are described similarly to clocks and GPIOs.

### 6.7.1 DMA Controller

**DMA controller:**

```
sdma: dma-controller@020ec000 {
    compatible = "fsl,imx6q-sdma";
    reg = <0x020ec000 0x4000>;
    interrupts = <0 2 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&clks IMX6QDL_CLK_SDMA>,
             <&clks IMX6QDL_CLK_SDMA>;
    clock-names = "ipg", "ahb";

    /* DMA controller properties */
    #dma-cells = <3>;
    /*             └─ channel, priority, direction */

    dma-channels = <32>;
};
```

**The #dma-cells property:**

```
#dma-cells = <3>

Format: <channel priority direction>

Cell 0: DMA request/channel number
Cell 1: Priority (0-7, 0 = highest)
Cell 2: Direction flags
```

### 6.7.2 DMA Consumer

**Device using DMA:**

```
uart1: serial@02020000 {
    compatible = "fsl,imx6q-uart";
    reg = <0x02020000 0x4000>;

    /* DMA channels */
    dmas = <&sdma 25 4 0>,   /* RX: channel 25, priority 4, flags 0 */
           <&sdma 26 4 0>;   /* TX: channel 26, priority 4, flags 0 */
    dma-names = "rx", "tx";
};
```

**Extracting DMA channels in driver:**

```c
static int uart_probe(struct platform_device *pdev)
{
    struct dma_chan *dma_rx, *dma_tx;

    /* Request DMA channels by name */
    dma_rx = dma_request_slave_channel(&pdev->dev, "rx");
    if (!dma_rx) {
        dev_warn(&pdev->dev, "Failed to get RX DMA channel\n");
        /* Fall back to PIO mode */
    }

    dma_tx = dma_request_slave_channel(&pdev->dev, "tx");
    if (!dma_tx) {
        dev_warn(&pdev->dev, "Failed to get TX DMA channel\n");
        /* Fall back to PIO mode */
    }

    return 0;
}
```

---

## 6.8 Pin Control (Pinctrl)

Pin multiplexing and configuration are handled by the pinctrl subsystem.

### 6.8.1 Pinctrl Provider

**Pin controller:**

```
iomuxc: iomuxc@020e0000 {
    compatible = "fsl,imx6q-iomuxc";
    reg = <0x020e0000 0x4000>;

    /* Pin configurations */
    pinctrl_uart1: uart1grp {
        fsl,pins = <
            MX6QDL_PAD_CSI0_DAT10__UART1_TX_DATA  0x1b0b1
            MX6QDL_PAD_CSI0_DAT11__UART1_RX_DATA  0x1b0b1
        >;
    };

    pinctrl_i2c1: i2c1grp {
        fsl,pins = <
            MX6QDL_PAD_CSI0_DAT8__I2C1_SDA  0x4001b8b1
            MX6QDL_PAD_CSI0_DAT9__I2C1_SCL  0x4001b8b1
        >;
    };
};
```

### 6.8.2 Pinctrl Consumer

**Device using pinctrl:**

```
uart1: serial@02020000 {
    compatible = "fsl,imx6q-uart";
    reg = <0x02020000 0x4000>;

    /* Pin configuration */
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_uart1>;
    /*           └─ Reference to pin configuration */
};
```

**Multiple pin states:**

```
device {
    pinctrl-names = "default", "sleep";
    pinctrl-0 = <&pinctrl_device_active>;
    pinctrl-1 = <&pinctrl_device_sleep>;
};
```

---

## 6.9 Named Resources

Named resources make code more readable and order-independent.

### 6.9.1 Named Memory Regions

```
device {
    reg = <0x02020000 0x4000>,   /* Control registers */
          <0x02024000 0x1000>;   /* Data FIFO */
    reg-names = "control", "fifo";
};
```

**In driver:**

```c
struct resource *ctrl_res, *fifo_res;

ctrl_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "control");
fifo_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "fifo");
```

### 6.9.2 Named Interrupts

```
device {
    interrupts = <0 118 IRQ_TYPE_LEVEL_HIGH>,
                 <0 119 IRQ_TYPE_LEVEL_HIGH>;
    interrupt-names = "tx", "rx";
};
```

**In driver:**

```c
int tx_irq = platform_get_irq_byname(pdev, "tx");
int rx_irq = platform_get_irq_byname(pdev, "rx");
```

### 6.9.3 Named Clocks

```
device {
    clocks = <&clk1>, <&clk2>, <&clk3>;
    clock-names = "core", "bus", "ref";
};
```

**In driver:**

```c
struct clk *core_clk = devm_clk_get(&pdev->dev, "core");
struct clk *bus_clk = devm_clk_get(&pdev->dev, "bus");
struct clk *ref_clk = devm_clk_get(&pdev->dev, "ref");
```

### 6.9.4 Complete Named Resources Example

```
mydevice: device@80000000 {
    compatible = "vendor,mydevice";

    /* Named memory regions */
    reg = <0x80000000 0x1000>,
          <0x80001000 0x400>,
          <0x80002000 0x100>;
    reg-names = "control", "data", "status";

    /* Named interrupts */
    interrupts = <0 56 IRQ_TYPE_LEVEL_HIGH>,
                 <0 57 IRQ_TYPE_LEVEL_HIGH>,
                 <0 58 IRQ_TYPE_LEVEL_HIGH>;
    interrupt-names = "tx", "rx", "error";

    /* Named clocks */
    clocks = <&clk IMX6QDL_CLK_IPG>,
             <&clk IMX6QDL_CLK_PER>,
             <&clk IMX6QDL_CLK_REF>;
    clock-names = "ipg", "per", "ref";

    /* Named DMA channels */
    dmas = <&sdma 25 4 0>, <&sdma 26 4 0>;
    dma-names = "rx", "tx";

    /* Named GPIOs */
    reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
    enable-gpios = <&gpio1 8 GPIO_ACTIVE_HIGH>;
};
```

**Driver extracting all named resources:**

```c
static int mydevice_probe(struct platform_device *pdev)
{
    struct resource *ctrl_res, *data_res, *status_res;
    void __iomem *ctrl_base, *data_base, *status_base;
    int tx_irq, rx_irq, error_irq;
    struct clk *ipg_clk, *per_clk, *ref_clk;
    struct dma_chan *dma_rx, *dma_tx;
    struct gpio_desc *reset_gpio, *enable_gpio;

    /* Get named memory regions */
    ctrl_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "control");
    data_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "data");
    status_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "status");

    ctrl_base = devm_ioremap_resource(&pdev->dev, ctrl_res);
    data_base = devm_ioremap_resource(&pdev->dev, data_res);
    status_base = devm_ioremap_resource(&pdev->dev, status_res);

    /* Get named interrupts */
    tx_irq = platform_get_irq_byname(pdev, "tx");
    rx_irq = platform_get_irq_byname(pdev, "rx");
    error_irq = platform_get_irq_byname(pdev, "error");

    /* Get named clocks */
    ipg_clk = devm_clk_get(&pdev->dev, "ipg");
    per_clk = devm_clk_get(&pdev->dev, "per");
    ref_clk = devm_clk_get(&pdev->dev, "ref");

    /* Get named DMA channels */
    dma_rx = dma_request_slave_channel(&pdev->dev, "rx");
    dma_tx = dma_request_slave_channel(&pdev->dev, "tx");

    /* Get named GPIOs */
    reset_gpio = devm_gpiod_get(&pdev->dev, "reset", GPIOD_OUT_HIGH);
    enable_gpio = devm_gpiod_get(&pdev->dev, "enable", GPIOD_OUT_LOW);

    return 0;
}
```

---

## 6.10 Status Property

The `status` property controls whether a device is enabled.

**Valid values:**

```
status = "okay";      /* Device is operational */
status = "disabled";  /* Device is not operational */
status = "fail";      /* Device failed (error detected) */
status = "fail-sss";  /* Device failed with specific error */
```

**Common pattern in .dtsi and .dts:**

```
/* SoC .dtsi - all peripherals disabled by default */
uart1: serial@02020000 {
    compatible = "fsl,imx6q-uart";
    reg = <0x02020000 0x4000>;
    interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
    status = "disabled";  /* Disabled by default */
};

/* Board .dts - enable what's actually used */
&uart1 {
    status = "okay";  /* Enable on this board */
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_uart1>;
};
```

**If status is omitted:**

The default is `"okay"` (device is enabled).

---

## Summary

In this part, we covered device addressing and resource handling:

**Key Concepts:**

1. **Address Cells**
    - `#address-cells`: Number of cells for address
    - `#size-cells`: Number of cells for size
    - Inherited from parent node
2. **reg Property**
    - MMIO: `<base_addr size>`
    - I2C: `<i2c_address>`
    - SPI: `<chip_select_index>`
3. **Interrupts**
    - Controller: `interrupt-controller`, `#interrupt-cells`
    - Consumer: `interrupt-parent`, `interrupts`
4. **Clocks**
    - Provider: `#clock-cells`
    - Consumer: `clocks`, `clock-names`
5. **GPIOs**
    - Controller: `gpio-controller`, `#gpio-cells`
    - Consumer: `<name>-gpios`
6. **Named Resources**
    - More readable code
    - Order-independent
    - Self-documenting