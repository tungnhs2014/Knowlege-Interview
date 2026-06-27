# Part 1. Pin Control Framework

This part introduces pin multiplexing (pinmux), pin configuration (pinconf), the Linux pin control subsystem, and Device Tree integration with real hardware examples.

---

## 14.1 Pin Control Fundamentals

### 14.1.1 What is a Pin?

**A pin is an outgoing line of a component (SoC):**

```
┌────────────────────────────────────────────────┐
│  SoC Pin - Multiple Functions                  │
├────────────────────────────────────────────────┤
│                                                │
│  Physical Pin (e.g., GPIO1_21 on AM335x)       │
│                                                │
│  Can operate in multiple MODES:                │
│                                                │
│  Mode 0: GPIO (General Purpose I/O)            │
│  Mode 1: UART_TXD (UART transmit)              │
│  Mode 2: I2C_SDA (I2C data line)               │
│  Mode 3: MMC_DATA (SD card data)               │
│  Mode 4: TIMER_OUT (Timer output)              │
│  Mode 5: CAN_TX (CAN bus transmit)             │
│  Mode 6: SPI_MOSI (SPI data out)               │
│  Mode 7: PWM_OUT (PWM signal)                  │
│                                                │
│  Only ONE mode can be active at a time!        │
│                                                │
└────────────────────────────────────────────────┘
```

**Real example from i.MX6:**

```
Pin: MX6QDL_PAD_SD3_DAT1

Can be:
  - SD3 Data Line 1 (SD card interface)
  - UART1 CTS/RTS (flow control)
  - FlexCAN2 RX (CAN bus receive)
  - GPIO (general purpose input/output)
```

### 14.1.2 Why Pin Multiplexing?

**Problem:** Modern SoCs have more functions than physical pins!

```
Example: AM335x SoC
├── Total functions: 200+ peripherals
├── Available pins: 324 physical pins
└── Solution: Each pin has 8 possible modes

Without multiplexing:
  Need 200+ × average 4 pins = 800+ pins!
  → Impossible to manufacture
  → Too expensive
  → Package too large

With multiplexing:
  324 pins × 8 modes = 2,592 possible functions
  → Flexible configuration
  → Smaller package
  → Lower cost
```

### 14.1.3 Pin Multiplexing (Pinmux)

**Pin multiplexing** is the mechanism to choose which function a pin operates in.

```
┌────────────────────────────────────────────────┐
│  Pin Multiplexing Hardware                     │
├────────────────────────────────────────────────┤
│                                                │
│         Pad Configuration Register             │
│         ┌────────────────────┐                 │
│         │ Mode[2:0] = 3'b010 │                 │
│         └────────┬───────────┘                 │
│                  │                             │
│                  ▼                             │
│         8-to-1 Multiplexer                     │
│         ┌──────────────────┐                   │
│  Mode 0 │                  │                   │
│  Mode 1 │                  │                   │
│  Mode 2 │→→→→→→→→→→→→→→→→→→│──→ Physical Pin   │
│  Mode 3 │                  │                   │
│  Mode 4 │                  │                   │
│  Mode 5 │                  │                   │
│  Mode 6 │                  │                   │
│  Mode 7 │                  │                   │
│         └──────────────────┘                   │
│                                                │
└────────────────────────────────────────────────┘
```

**Hardware control:**

- Each pin has a **pad configuration register**
- Register contains mode selection bits
- Hardware multiplexer routes signal based on mode
- Software writes to register to change mode

### 14.1.4 Pin Configuration (Pinconf)

**Beyond mode selection, pins need electrical configuration:**

```
┌────────────────────────────────────────────────┐
│  Pin Configuration Parameters                  │
├────────────────────────────────────────────────┤
│                                                │
│  Pull-up/Pull-down:                            │
│  ┌─────────┐                                   │
│  │  VCC    │  Pull-up resistor                 │
│  │   │     │  → Pulls pin HIGH when floating   │
│  │  ┌┴┐    │                                   │
│  │  └┬┘    │                                   │
│  │   │─────┼──→ Pin                            │
│  │  ┌┴┐    │                                   │
│  │  └┬┘    │  Pull-down resistor               │
│  │   │     │  → Pulls pin LOW when floating    │
│  │  GND    │                                   │
│  └─────────┘                                   │
│                                                │
│  Drive Strength (Current capability):          │
│  - 2mA, 4mA, 8mA, 12mA, etc.                   │
│  - Higher current = stronger signal            │
│  - Trade-off: power consumption vs speed       │
│                                                │
│  Slew Rate (Edge speed):                       │
│  - Fast: Quick transitions (EMI risk)          │
│  - Slow: Gradual transitions (cleaner)         │
│                                                │
│  Output Type:                                  │
│  - Push-pull: Can drive HIGH and LOW           │
│  - Open-drain: Can only drive LOW              │
│                                                │
│  Debounce Period (for inputs):                 │
│  - Filter noise on input signals               │
│  - Typical: 10ms - 100ms                       │
│                                                │
│  Schmitt Trigger:                              │
│  - Hysteresis for noisy signals                │
│  - Different thresholds for rising/falling     │
│                                                │
└────────────────────────────────────────────────┘
```

**Example configuration needs:**

```
I2C Bus:
  ✓ Mode: I2C_SDA / I2C_SCL
  ✓ Pull-up: ENABLED (required by I2C spec)
  ✓ Drive: Open-drain
  ✓ Slew rate: Slow (reduce EMI)

UART:
  ✓ Mode: UART_TX / UART_RX
  ✓ Pull-up: Disabled on TX, Enabled on RX
  ✓ Drive: Push-pull
  ✓ Slew rate: Fast (high baud rates)

GPIO Button:
  ✓ Mode: GPIO Input
  ✓ Pull-up: ENABLED (button connects to GND)
  ✓ Debounce: 50ms (filter mechanical bounce)
```

### 14.1.5 Pin Controller

**The Pin Controller is the hardware block that manages pin configuration:**

```
┌────────────────────────────────────────────────┐
│  Pin Controller Hardware Architecture          │
├────────────────────────────────────────────────┤
│                                                │
│  CPU/Software                                  │
│       │                                        │
│       │ Writes to registers                    │
│       ▼                                        │
│  ┌──────────────────────────────────┐          │
│  │  Pin Controller Registers        │          │
│  │  (Memory-mapped)                 │          │
│  │                                  │          │
│  │  Pad Config Reg 0: 0x44E10800    │          │
│  │  Pad Config Reg 1: 0x44E10804    │          │
│  │  Pad Config Reg 2: 0x44E10808    │          │
│  │  ...                             │          │
│  │  Pad Config Reg N: 0x44E10xxx    │          │
│  └──────────┬───────────────────────┘          │
│             │                                  │
│             │ Controls                         │
│             ▼                                  │
│  ┌─────────────────────────────────┐           │
│  │  Pin Multiplexing Logic         │           │
│  │  - Mode selection               │           │
│  │  - Pull-up/down control         │           │
│  │  - Drive strength               │           │
│  │  - Slew rate                    │           │
│  └──────────┬──────────────────────┘           │
│             │                                  │
│             │ Configures                       │
│             ▼                                  │
│  ┌─────────────────────────────────┐           │
│  │  Physical Pins (Pads)           │           │
│  │  GPIO1_21, UART0_TX, I2C0_SDA... │          │
│  └─────────────────────────────────┘           │
│                                                │
└────────────────────────────────────────────────┘
```

**Examples:**

- **AM335x**: Pad configuration registers at 0x44E10800
- **i.MX6**: IOMUXC registers
- **STM32MP1**: Pin controller at 0x50002000

---

## 14.2 Linux Pin Control Subsystem

### 14.2.1 Pinctrl Subsystem Overview

**The Linux pinctrl subsystem provides:**

```
┌────────────────────────────────────────────────┐
│  Linux Pin Control Subsystem                   │
├────────────────────────────────────────────────┤
│                                                │
│  1. Pin Multiplexing (Pinmux):                 │
│     - Select which function a pin operates in  │
│     - Group multiple pins for a function       │
│     - Example: UART needs TX + RX pins         │
│                                                │
│  2. Pin Configuration (Pinconf):               │
│     - Set pull-up/pull-down                    │
│     - Configure drive strength                 │
│     - Set slew rate                            │
│     - Enable/disable Schmitt trigger           │
│                                                │
│  3. State Management:                          │
│     - Different states: default, sleep, idle   │
│     - Dynamic reconfiguration at runtime       │
│     - Power management support                 │
│                                                │
└────────────────────────────────────────────────┘
```

### 14.2.2 Pinctrl Architecture

```
┌────────────────────────────────────────────────┐
│  Pinctrl Architecture                          │
├────────────────────────────────────────────────┤
│                                                │
│  Consumer Drivers (I2C, UART, SPI...)          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │ I2C1     │  │ UART3    │  │ SPI1     │      │
│  │ Driver   │  │ Driver   │  │ Driver   │      │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘      │
│       │             │             │            │
│       │ pinctrl_get/select_state  │            │
│       ▼             ▼             ▼            │
│  ┌─────────────────────────────────────┐       │
│  │  Pinctrl Core                       │       │
│  │  (drivers/pinctrl/core.c)           │       │
│  │                                     │       │
│  │  - Device/driver matching           │       │
│  │  - State management                 │       │
│  │  - Consumer API                     │       │
│  └─────────────┬───────────────────────┘       │
│                │                               │
│                │ Calls                         │
│                ▼                               │
│  ┌─────────────────────────────────────┐       │
│  │  Pin Controller Driver              │       │
│  │  (vendor-specific)                  │       │
│  │                                     │       │
│  │  - pinctrl-single.c (generic)       │       │
│  │  - pinctrl-imx.c (i.MX6)            │       │
│  │  - pinctrl-stm32.c (STM32)          │       │
│  │  - pinctrl-am33xx.c (AM335x)        │       │
│  └─────────────┬───────────────────────┘       │
│                │                               │
│                │ Writes to                     │
│                ▼                               │
│  ┌─────────────────────────────────────┐       │
│  │  Pin Controller Hardware            │       │
│  │  (Pad configuration registers)      │       │
│  └─────────────────────────────────────┘       │
│                                                │
└────────────────────────────────────────────────┘
```

**Key components:**

1. **Pinctrl Core** (`drivers/pinctrl/core.c`)
    - Generic infrastructure
    - Consumer API
    - State management
2. **Pin Controller Driver** (vendor-specific)
    - Implements hardware-specific operations
    - Knows register layout
    - Example: `pinctrl-single.c` for generic DT-based controller
3. **Consumer Drivers**
    - Use pinctrl consumer API
    - Don't know hardware details
    - Just request pin states

---

## 14.3 Pin Control and Device Tree

### 14.3.1 DT Node Structure

**Pin control requires TWO types of nodes:**

```
┌────────────────────────────────────────────────┐
│  Pin Control Device Tree Structure             │
├────────────────────────────────────────────────┤
│                                                │
│  1. Pin Controller Node (in SoC .dtsi)         │
│     - Describes the pin controller hardware    │
│     - Contains configuration subnodes          │
│                                                │
│  2. Pin Configuration Nodes (nested)           │
│     - Describe pin groups and settings         │
│     - Multiple configurations possible         │
│                                                │
│  3. Consumer Device Node (in board .dts)       │
│     - References pin configurations            │
│     - Claims pins for use                      │
│                                                │
└────────────────────────────────────────────────┘
```

### 14.3.2 Pin Controller Node

**Example 1: AM335x (Texas Instruments BeagleBone Black)**

```
/* From arch/arm/boot/dts/am33xx-l4.dtsi */

am33xx_pinmux: pinmux@44e10800 {
    compatible = "pinctrl-single";
    reg = <0x44e10800 0x0238>;
    #address-cells = <1>;
    #size-cells = <0>;
    #pinctrl-cells = <1>;
    pinctrl-single,register-width = <32>;
    pinctrl-single,function-mask = <0x7f>;
};
```

**Field explanations:**

- `compatible = "pinctrl-single"`: Uses generic pinctrl driver
- `reg = <0x44e10800 0x0238>`: Register base address and size
- `#pinctrl-cells = <1>`: Each pin config uses 1 cell
- `pinctrl-single,register-width = <32>`: Each register is 32-bit
- `pinctrl-single,function-mask = <0x7f>`: Lower 7 bits for pin config

**Example 2: i.MX6 (Freescale/NXP)**

```
/* From arch/arm/boot/dts/imx6qdl.dtsi */

iomuxc: iomuxc@020e0000 {
    compatible = "fsl,imx6q-iomuxc";
    reg = <0x020e0000 0x4000>;
};
```

**Example 3: STM32MP1 (ST Microelectronics)**

```
/* From arch/arm/boot/dts/stm32mp151.dtsi */

pinctrl: pin-controller@50002000 {
    #address-cells = <1>;
    #size-cells = <1>;
    compatible = "st,stm32mp157-pinctrl";
    ranges = <0 0x50002000 0xa400>;
    pins-are-numbered;

    gpioa: gpio@50002000 {
        gpio-controller;
        #gpio-cells = <2>;
        interrupt-controller;
        #interrupt-cells = <2>;
        reg = <0x0 0x400>;
    };

    gpiob: gpio@50003000 {
        gpio-controller;
        #gpio-cells = <2>;
        interrupt-controller;
        #interrupt-cells = <2>;
        reg = <0x1000 0x400>;
    };

    /* More GPIO banks... */
};
```

### 14.3.3 Pin Configuration Nodes

**These nodes describe HOW to configure pins:**

**Structure (typically nested in controller node):**

```
&pinctrl_controller {

    /* Function group */
    function_name {

        /* Configuration node */
        config_name: config_label {
            /* Pin list with settings */
            pinctrl-single,pins = <
                OFFSET1 VALUE1
                OFFSET2 VALUE2
                ...
            >;
        };
    };
};
```

**Example 1: AM335x I2C pins**

```
&am33xx_pinmux {

    i2c0_pins: pinmux_i2c0_pins {
        pinctrl-single,pins = <
            /* Offset    Value */
            0x188 (PIN_INPUT_PULLUP | MUX_MODE0)  /* I2C0_SDA */
            0x18c (PIN_INPUT_PULLUP | MUX_MODE0)  /* I2C0_SCL */
        >;
    };

    i2c1_pins: pinmux_i2c1_pins {
        pinctrl-single,pins = <
            0x158 (PIN_INPUT_PULLUP | MUX_MODE2)  /* I2C1_SDA */
            0x15c (PIN_INPUT_PULLUP | MUX_MODE2)  /* I2C1_SCL */
        >;
    };
};
```

**Understanding the values:**

```c
/* From include/dt-bindings/pinctrl/am33xx.h */

#define PIN_OUTPUT              0
#define PIN_OUTPUT_PULLUP       (1 << 4)
#define PIN_INPUT               (1 << 5)
#define PIN_INPUT_PULLUP        (1 << 5 | 1 << 4)
#define PIN_INPUT_PULLDOWN      (1 << 5)

#define MUX_MODE0               0
#define MUX_MODE1               1
#define MUX_MODE2               2
/* ... up to MODE7 */

/* Example: PIN_INPUT_PULLUP | MUX_MODE2
 * = (1<<5 | 1<<4) | 2
 * = 0x32
 * Bit 5: Input enable
 * Bit 4: Pull-up enable
 * Bits 2-0: Mode selection (mode 2)
 */
```

**Example 2: i.MX6 UART pins**

```
/* From arch/arm/boot/dts/imx6qdl.dtsi */

&iomuxc {

    uart3 {
        pinctrl_uart3_1: uart3grp-1 {
            fsl,pins = <
                MX6QDL_PAD_EIM_D24__UART3_TX_DATA 0x1b0b1
                MX6QDL_PAD_EIM_D25__UART3_RX_DATA 0x1b0b1
            >;
        };
    };

    usdhc4 {
        pinctrl_usdhc4_1: usdhc4grp-1 {
            fsl,pins = <
                MX6QDL_PAD_SD4_CMD__SD4_CMD     0x17059
                MX6QDL_PAD_SD4_CLK__SD4_CLK     0x10059
                MX6QDL_PAD_SD4_DAT0__SD4_DATA0  0x17059
                MX6QDL_PAD_SD4_DAT1__SD4_DATA1  0x17059
                MX6QDL_PAD_SD4_DAT2__SD4_DATA2  0x17059
                MX6QDL_PAD_SD4_DAT3__SD4_DATA3  0x17059
            >;
        };
    };
};
```

**i.MX6 macro format:**

```c
/* From arch/arm/boot/dts/imx6q-pinfunc.h */

#define MX6QDL_PAD_EIM_D25__UART3_RX_DATA  0x19c 0x4b0 0x000 0x5 0x0
/*      ^                                   ^     ^     ^     ^   ^
 *      Pin name                            |     |     |     |   |
 *                                     mux_reg  |     |     |   |
 *                                         conf_reg    |     |   |
 *                                             input_reg    |   |
 *                                                    mux_mode  |
 *                                                      input_val
 */
```

**Example 3: STM32MP1 I2C and CAN pins**

```
/* From arch/arm/boot/dts/stm32mp15-pinctrl.dtsi */

&pinctrl {

    i2c1_pins_a: i2c1-0 {
        pins {
            pinmux = <STM32_PINMUX('D', 12, AF5)>,  /* I2C1_SCL */
                     <STM32_PINMUX('F', 15, AF5)>;  /* I2C1_SDA */
            bias-disable;
            drive-open-drain;
            slew-rate = <0>;
        };
    };

    m_can1_pins_a: m-can1-0 {
        pins1 {
            pinmux = <STM32_PINMUX('H', 13, AF9)>;  /* CAN1_TX */
            slew-rate = <1>;
            drive-push-pull;
            bias-disable;
        };
        pins2 {
            pinmux = <STM32_PINMUX('I', 9, AF9)>;   /* CAN1_RX */
            bias-disable;
        };
    };
};
```

**STM32 macro:**

```c
#define STM32_PINMUX(port, pin, af) \
    ((((port) - 'A') << 12) | ((pin) << 8) | (af))

/* STM32_PINMUX('D', 12, AF5)
 * Port D, Pin 12, Alternate Function 5
 */
```

### 14.3.4 Consumer Device Nodes

**Devices claim pins using two properties:**

```
device_node {
    compatible = "...";

    /* Property 1: State names */
    pinctrl-names = "default", "sleep", "idle";

    /* Property 2: Pin configurations for each state */
    pinctrl-0 = <&config_default>;      /* State 0: default */
    pinctrl-1 = <&config_sleep>;        /* State 1: sleep */
    pinctrl-2 = <&config_idle>;         /* State 2: idle */
};
```

**Properties explained:**

- `pinctrl-names`: List of state names
    - Entry 0 → State ID 0
    - Entry 1 → State ID 1
    - "default" state (ID 0) is mandatory
- `pinctrl-N`: Pin configuration for state N
    - Phandle pointing to configuration node
    - Can have multiple phandles (multiple pin groups)

**Example 1: I2C device (AM335x)**

```
&i2c1 {
    pinctrl-names = "default", "sleep";
    pinctrl-0 = <&i2c1_pins>;
    pinctrl-1 = <&i2c1_sleep_pins>;
    status = "okay";

    eeprom@50 {
        compatible = "atmel,24c256";
        reg = <0x50>;
    };
};
```

**Example 2: UART device (i.MX6)**

```
&uart3 {
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_uart3_1>;
    status = "okay";
};
```

**Example 3: SD card (i.MX6)**

```
usdhc@0219c000 {
    compatible = "fsl,imx6q-usdhc";
    reg = <0x0219c000 0x4000>;
    non-removable;
    vmmc-supply = <&reg_3p3v>;

    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_usdhc4_1>;

    status = "okay";
};
```

**Example 4: GPIO device with multiple states**

```
gpio-keys {
    compatible = "gpio-keys";

    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_io_foo &pinctrl_io_bar>;

    button1 {
        label = "User Button";
        gpios = <&gpio5 9 GPIO_ACTIVE_LOW>;
        linux,code = <KEY_ENTER>;
    };
};
```

### 14.3.5 Complete Real Example: BeagleBone Black LCD

**Hardware:** 16x2 character LCD connected to BBB

**Pin connections:**

```
LCD Pin  →  BBB Pin   →  GPIO
RS       →  P8.45     →  GPIO2_6
RW       →  P8.46     →  GPIO2_7
EN       →  P8.43     →  GPIO2_8
D4       →  P8.44     →  GPIO2_9
D5       →  P8.41     →  GPIO2_10
D6       →  P8.42     →  GPIO2_11
D7       →  P8.39     →  GPIO2_12
```

**Device Tree configuration:**

```
/* Step 1: Define pin configurations in SoC file */
&am33xx_pinmux {

    lcd_pins: lcd_pins {
        pinctrl-single,pins = <
            /* RS - GPIO2_6 */
            0x0a0 (PIN_OUTPUT | MUX_MODE7)

            /* RW - GPIO2_7 */
            0x0a4 (PIN_OUTPUT | MUX_MODE7)

            /* EN - GPIO2_8 */
            0x0a8 (PIN_OUTPUT | MUX_MODE7)

            /* D4 - GPIO2_9 */
            0x0ac (PIN_OUTPUT | MUX_MODE7)

            /* D5 - GPIO2_10 */
            0x0b0 (PIN_OUTPUT | MUX_MODE7)

            /* D6 - GPIO2_11 */
            0x0b4 (PIN_OUTPUT | MUX_MODE7)

            /* D7 - GPIO2_12 */
            0x0b8 (PIN_OUTPUT | MUX_MODE7)
        >;
    };
};

/* Step 2: Create device node using these pins */
/ {
    lcd_device {
        compatible = "bone,lcd16x2";

        pinctrl-names = "default";
        pinctrl-0 = <&lcd_pins>;

        lcd-gpios =
            <&gpio2 6 GPIO_ACTIVE_HIGH>,   /* RS */
            <&gpio2 7 GPIO_ACTIVE_HIGH>,   /* RW */
            <&gpio2 8 GPIO_ACTIVE_HIGH>,   /* EN */
            <&gpio2 9 GPIO_ACTIVE_HIGH>,   /* D4 */
            <&gpio2 10 GPIO_ACTIVE_HIGH>,  /* D5 */
            <&gpio2 11 GPIO_ACTIVE_HIGH>,  /* D6 */
            <&gpio2 12 GPIO_ACTIVE_HIGH>;  /* D7 */

        status = "okay";
    };
};
```

---

## 14.4 Pin Control Consumer API

### 14.4.1 Required Header

```c
#include <linux/pinctrl/consumer.h>
```

### 14.4.2 Basic Pin Control Functions

**Get pin control:**

```c
struct pinctrl *pinctrl_get(struct device *dev);
```

**Lookup a state:**

```c
struct pinctrl_state *pinctrl_lookup_state(struct pinctrl *p,
                                            const char *name);
```

**Select a state:**

```c
int pinctrl_select_state(struct pinctrl *p, struct pinctrl_state *s);
```

**Release pin control:**

```c
void pinctrl_put(struct pinctrl *p);
```

### 14.4.3 Complete Example

```c
#include <linux/pinctrl/consumer.h>

static int my_probe(struct platform_device *pdev)
{
    struct pinctrl *p;
    struct pinctrl_state *s;
    int ret;

    /* Step 1: Get pin control */
    p = pinctrl_get(&pdev->dev);
    if (IS_ERR(p)) {
        dev_err(&pdev->dev, "Failed to get pinctrl\n");
        return PTR_ERR(p);
    }

    /* Step 2: Lookup "default" state */
    s = pinctrl_lookup_state(p, "default");
    if (IS_ERR(s)) {
        dev_err(&pdev->dev, "Failed to lookup default state\n");
        pinctrl_put(p);
        return PTR_ERR(s);
    }

    /* Step 3: Apply the state */
    ret = pinctrl_select_state(p, s);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to select state\n");
        pinctrl_put(p);
        return ret;
    }

    dev_info(&pdev->dev, "Pins configured successfully\n");

    /* Continue with device initialization... */

    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    /* Cleanup */
    pinctrl_put(p);
    return 0;
}
```

### 14.4.4 Convenience Functions

**Get and select in one call:**

```c
struct pinctrl *pinctrl_get_select(struct device *dev,
                                    const char *name);
```

**Get and select "default" state:**

```c
static inline struct pinctrl *pinctrl_get_select_default(
                                        struct device *dev)
{
    return pinctrl_get_select(dev, PINCTRL_STATE_DEFAULT);
}
```

**Usage example:**

```c
static int my_probe(struct platform_device *pdev)
{
    struct pinctrl *pinctrl;

    /* One-line pin configuration! */
    pinctrl = pinctrl_get_select_default(&pdev->dev);
    if (IS_ERR(pinctrl)) {
        dev_warn(&pdev->dev,
                 "Pins not configured from driver\n");
    }

    /* Device initialization... */

    return 0;
}
```

### 14.4.5 Resource-Managed Versions

*Using devm_ for automatic cleanup:**

```c
struct pinctrl *devm_pinctrl_get(struct device *dev);
struct pinctrl_state *devm_pinctrl_get_select(struct device *dev,
                                               const char *name);
struct pinctrl_state *devm_pinctrl_get_select_default(
                                        struct device *dev);
```

**Advantage:** Automatic cleanup on driver detach!

```c
static int my_probe(struct platform_device *pdev)
{
    struct pinctrl *pinctrl;

    /* Automatically freed on remove/error! */
    pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
    if (IS_ERR(pinctrl))
        return PTR_ERR(pinctrl);

    /* No need to call pinctrl_put() in remove! */

    return 0;
}
```

### 14.4.6 Real Driver Example: CAN

```c
/* From board .dts */
&dcan1 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&d_can1_pins>;
};

/* Driver code */
static int dcan_probe(struct platform_device *pdev)
{
    struct pinctrl *pinctrl;

    /* Pin configuration (automatic with default state) */
    pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
    if (IS_ERR(pinctrl))
        dev_warn(&pdev->dev,
                 "Pins not configured from driver\n");

    /* Rest of initialization... */

    return 0;
}
```

### 14.4.7 Automatic Default State Selection

**Important note:**

> The pin control core automatically claims the "default" state when a device is probed!
> 

**This means:**

```c
/* You usually DON'T need to do this: */
pinctrl_get_select_default(&pdev->dev);

/* Because the core already did it! */
```

**When you DO need explicit control:**

1. Multiple states (default, sleep, idle)
2. Dynamic state switching at runtime
3. Debugging/verification

**Example with multiple states:**

```c
struct my_device {
    struct pinctrl *pinctrl;
    struct pinctrl_state *pins_default;
    struct pinctrl_state *pins_sleep;
};

static int my_probe(struct platform_device *pdev)
{
    struct my_device *priv;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);

    /* Get pinctrl */
    priv->pinctrl = devm_pinctrl_get(&pdev->dev);
    if (IS_ERR(priv->pinctrl))
        return PTR_ERR(priv->pinctrl);

    /* Lookup states */
    priv->pins_default = pinctrl_lookup_state(priv->pinctrl,
                                               "default");
    priv->pins_sleep = pinctrl_lookup_state(priv->pinctrl,
                                             "sleep");

    /* Start in default state */
    pinctrl_select_state(priv->pinctrl, priv->pins_default);

    return 0;
}

static int my_suspend(struct device *dev)
{
    struct my_device *priv = dev_get_drvdata(dev);

    /* Switch to sleep state to save power */
    pinctrl_select_state(priv->pinctrl, priv->pins_sleep);

    return 0;
}

static int my_resume(struct device *dev)
{
    struct my_device *priv = dev_get_drvdata(dev);

    /* Restore default state */
    pinctrl_select_state(priv->pinctrl, priv->pins_default);

    return 0;
}
```

---

## Summary

In this part, we covered:

**Key Concepts:**

1. **Pin Multiplexing**
    - Multiple functions per physical pin
    - Hardware multiplexer controlled by registers
    - Reduces pin count and cost
2. **Pin Configuration**
    - Pull-up/pull-down resistors
    - Drive strength and slew rate
    - Output type (push-pull/open-drain)
    - Debounce for inputs
3. **Pin Controller**
    - Hardware block managing pins
    - Memory-mapped registers
    - Vendor-specific implementations
4. **Linux Pinctrl Subsystem**
    - Generic infrastructure (core)
    - Vendor-specific drivers
    - Consumer API for device drivers
5. **Device Tree Integration**
    - Pin controller nodes (SoC .dtsi)
    - Pin configuration nodes (nested)
    - Consumer device nodes (board .dts)
    - Real examples: AM335x, i.MX6, STM32MP1
6. **Consumer API**
    - pinctrl_get/put functions
    - State lookup and selection
    - Convenience functions
    - Resource-managed versions
7. **Real Hardware Examples**
    - BeagleBone Black LCD project
    - I2C, UART, CAN configurations
    - Multi-state power management