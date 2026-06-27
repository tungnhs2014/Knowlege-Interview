# Part 2. Kernel Configuration

This section covers kernel configuration system, configuration tools, and how to properly configure the kernel for your embedded target.

## 1.5 Kernel Configuration

### What is Kernel Configuration?

Kernel configuration is the process of **selecting which features, drivers, and subsystems** to include in your kernel build. The Linux kernel has thousands of options!

**Why do we need configuration?**

- **Customize** kernel for specific hardware
- **Optimize size** - Disable unnecessary features
- **Enable required drivers** - For your target hardware
- **Set parameters** - Tune kernel behavior
- **Security** - Enable/disable security features

**Configuration Result:**
All choices are stored in `.config` file at kernel source root directory.

### Configuration System (Kconfig)

The kernel uses a configuration language called **Kconfig** to define:

- Configuration options
- Dependencies between options
- Help text for each option
- Default values

**Example Kconfig snippet:**

```
config GPIO_MCP23S08
	tristate "Microchip MCP23xxx I/O expander"
	depends on (SPI_MASTER && !I2C) || I2C
	help
	  SPI/I2C driver for Microchip MCP23S08/MCP23S17/MCP23008/MCP23017
	  I/O expanders.
	  This provides a GPIO interface supporting inputs and outputs.
```

### Understanding Build Options

Each kernel feature can be:

**`[*]` Built-in (y)**

- Compiled directly into kernel image
- Always available when kernel boots
- **Use when**: Feature needed early in boot (e.g., rootfs driver, console)
- Cannot be unloaded
- Example: MMC driver for SD card root filesystem

**`[M]` Module (m)**

- Compiled as separate `.ko` file
- Loaded on demand (reduces memory usage)
- **Use when**: Feature not needed at boot, optional hardware
- Can be loaded/unloaded at runtime
- Example: USB WiFi driver
- [ ]  **`[ ]` Disabled (n)**
- Not compiled at all
- **Use when**: Feature not needed, save space
- Example: Bluetooth if your device has no BT hardware

**Example:**

```
Device Drivers --->
  [*] GPIO Support --->
      <*>   /sys/class/gpio/... (sysfs interface)  ← Built-in
      <M>   MCP23S08 I/O expander                  ← Module
      < >   Generic soft-gpio I2C                  ← Disabled
```

---

## 1.6 Kernel Configuration Tools

The kernel provides several configuration interfaces:

### 1. menuconfig (Text-Based UI) - Most Popular

**Command:**

```bash
cd ~/embedded-workspace/kernel/linux
make menuconfig
```

**Interface:**

```
 ┌─────── Linux Kernel Configuration ───────┐
 │  Arrow keys navigate the menu.           │
 │  <Enter> selects submenus --->           │
 │  Highlighted letters are hotkeys.        │
 │  Pressing <Y> includes, <N> excludes,    │
 │  <M> modularizes features.               │
 │  Legend: [*] built-in  [ ] excluded      │
 │          <M> module    < > module capable│
 └──────────────────────────────────────────┘
   General setup --->
   [*] Enable loadable module support --->
   [*] Enable the block layer --->
   Processor type and features --->
   Power management and ACPI options --->
   Bus options --->
   Executable file formats --->
   [*] Networking support --->
   Device Drivers --->
   File systems --->
   Kernel hacking --->
```

**Navigation:**

- **Arrow keys**: Move up/down
- **Enter**: Enter submenu or toggle option
- **Space**: Cycle between [*], [M], [ ]
- **Y**: Force built-in [*]
- **M**: Force module <M>
- **N**: Disable [ ]
- **/**: Search for option
- **?**: Show help for selected option
- **ESC ESC**: Go back or exit

**Advantages:**

- Works over SSH (no GUI needed)
- Fast navigation
- Lightweight (no X server required)
- Most commonly used by developers

### 2. xconfig (Qt-Based GUI)

**Requirements:**

```bash
sudo apt-get install qt5-default
```

**Command:**

```bash
make xconfig
```

**Advantages:**

- Visual interface
- Easy to navigate
- Search functionality
- Multiple view modes (split panes)
- Good for beginners

### 3. gconfig (GTK-Based GUI)

**Requirements:**

```bash
sudo apt-get install libglade2-dev
```

**Command:**

```bash
make gconfig
```

### 4. nconfig (Newer Text UI)

```bash
make nconfig
```

- Similar to menuconfig but with improved UI
- Single-menu display mode

**Comparison:**

```
┌──────────┬────────────┬──────────┬────────────┐
│ Tool     │ Interface  │ SSH OK?  │ Use Case   │
├──────────┼────────────┼──────────┼────────────┤
│menuconfig│ Text (ncurses)│ Yes   │ Production │
│ xconfig  │ GUI (Qt)   │ No       │ Local dev  │
│ gconfig  │ GUI (GTK)  │ No       │ Local dev  │
│ nconfig  │ Text (new) │ Yes      │ Alternative│
└──────────┴────────────┴──────────┴────────────┘
```

---

## 1.7 Setting Architecture and Compiler

Before configuration or building, you **must** specify:

1. **ARCH**: Target architecture
2. **CROSS_COMPILE**: Cross-compiler prefix

### Method 1: Command Line (One-time)

```bash
# For each make command
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- menuconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- zImage
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules
```

**Drawback:** Easy to forget, causing native x86 build accidentally!

### Method 2: Environment Variables (Recommended)

```bash
# Set once per terminal session
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

# Now all make commands use these values
make menuconfig
make zImage
make modules
```

**Drawback:** Only works in current shell

### Method 3: Configuration Script (Best Practice)

Create a setup script to source:

```bash
# Create setup script
cat > ~/embedded-workspace/setup-env.sh << 'EOF'
#!/bin/bash
# Embedded Linux Development Environment

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export KERNEL_DIR=~/embedded-workspace/kernel/linux
export INSTALL_MOD_PATH=~/embedded-workspace/rootfs

# Add toolchain to PATH if needed
export PATH=/path/to/toolchain/bin:$PATH

echo "Environment configured:"
echo "  ARCH=$ARCH"
echo "  CROSS_COMPILE=$CROSS_COMPILE"
echo "  KERNEL_DIR=$KERNEL_DIR"
EOF

chmod +x ~/embedded-workspace/setup-env.sh
```

**Usage:**

```bash
# Every time you open a new terminal
cd ~/embedded-workspace
source setup-env.sh

# Now ready to build
cd $KERNEL_DIR
make menuconfig
```

**Verify settings:**

```bash
echo $ARCH
# Output: arm

echo $CROSS_COMPILE
# Output: arm-linux-gnueabihf-
```

### Understanding ARCH Values

```bash
# Common ARCH values (from arch/ directory)
ls arch/

alpha/      # Alpha processors
arc/        # ARC processors
arm/        # ARM 32-bit (ARMv5/v6/v7)
arm64/      # ARM 64-bit (ARMv8, Cortex-A53/A57/A72)
csky/       # C-Sky processors
hexagon/    # Qualcomm Hexagon
ia64/       # Intel Itanium
loongarch/  # LoongArch
m68k/       # Motorola 68000
microblaze/ # Xilinx MicroBlaze
mips/       # MIPS processors
nios2/      # Altera Nios II
openrisc/   # OpenRISC
parisc/     # HP PA-RISC
powerpc/    # PowerPC (32 & 64-bit)
riscv/      # RISC-V
s390/       # IBM s390
sh/         # SuperH
sparc/      # SPARC processors
um/         # User-Mode Linux
x86/        # x86 and x86_64
xtensa/     # Xtensa processors
```

**Most common for embedded:**

- `ARCH=arm` → ARM 32-bit (Cortex-A7/A9/A15/A17, ARMv7)
- `ARCH=arm64` → ARM 64-bit (Cortex-A53/A55/A57/A72/A73, ARMv8)
- `ARCH=riscv` → RISC-V (emerging open architecture)
- `ARCH=mips` → MIPS (routers, some embedded systems)

### Understanding CROSS_COMPILE Prefix

**Format:** `<architecture>-<vendor>-<os>-<abi>-<tool>`

**Examples:**

```bash
# ARM 32-bit hard float
arm-linux-gnueabihf-gcc
├─ arm:           Architecture
├─ linux:         Target OS
├─ gnueabihf:     ABI with hard float
└─ gcc:           Tool (compiler)

# ARM 64-bit
aarch64-linux-gnu-gcc

# RISC-V 64-bit
riscv64-linux-gnu-gcc
```

**Testing your toolchain:**

```bash
# Check compiler version
${CROSS_COMPILE}gcc --version

# Output example:
# arm-linux-gnueabihf-gcc (Ubuntu 11.3.0-1ubuntu1~22.04) 11.3.0

# Test compilation
echo 'int main() { return 0; }' > test.c
${CROSS_COMPILE}gcc test.c -o test
file test

# Output:
# test: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV)...
```

---

## 1.8 Default Configurations (defconfig)

### What are defconfig Files?

**Default configurations** are minimal configuration files that contain only settings different from kernel defaults.

**Location:**

```bash
arch/<ARCH>/configs/

# ARM 32-bit examples
arch/arm/configs/
├── imx_v6_v7_defconfig    # NXP i.MX6/i.MX7
├── multi_v7_defconfig     # Generic ARMv7 (multiple SoCs)
├── sunxi_defconfig        # Allwinner (A10/A20)
├── omap2plus_defconfig    # TI OMAP
├── exynos_defconfig       # Samsung Exynos
└── ...

# ARM 64-bit example
arch/arm64/configs/
└── defconfig              # One defconfig for all ARM64!

# x86 examples
arch/x86/configs/
├── i386_defconfig         # 32-bit x86
└── x86_64_defconfig       # 64-bit x86
```

### Why Use defconfig?

**Starting from scratch is difficult!**

- Thousands of options
- Complex dependencies
- Easy to miss critical drivers
- May not boot without correct options

**defconfig provides:**

- Known working baseline
- Appropriate drivers for SoC family
- Reasonable defaults
- Community-tested configuration

### Loading a defconfig

```bash
cd ~/embedded-workspace/kernel/linux

# Set ARCH first!
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

# List available defconfigs
make help | grep defconfig
# Or: ls arch/arm/configs/

# Load specific defconfig
make imx_v6_v7_defconfig

# Result: .config file created with default settings
```

**What just happened?**

1. Kernel reads `arch/arm/configs/imx_v6_v7_defconfig`
2. Expands minimal config to full `.config`
3. Sets all dependencies automatically
4. `.config` now contains ~6000+ options

### Customizing After defconfig

**Typical workflow:**

```bash
# 1. Start with defconfig
make imx_v6_v7_defconfig

# 2. Customize for your needs
make menuconfig

# 3. Save your changes (optional)
make savedefconfig
# Creates minimal defconfig in 'defconfig' file

# 4. Rename and save
cp defconfig arch/arm/configs/my_board_defconfig

# 5. Later, you or your team can use it
make my_board_defconfig
```

### Example: Configuring for i.MX6 Board

```bash
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

# Load i.MX6 defaults
make imx_v6_v7_defconfig

# Customize
make menuconfig
```

**In menuconfig, enable your specific drivers:**

```
Device Drivers --->
  [*] GPIO Support --->
      <M> MCP23S08 I/O expander     ← Enable your GPIO expander

  <*> I2C support --->
      <*>   I2C device interface     ← Enable I2C user-space access
      [*]   I2C Hardware Bus support --->
          <*> IMX I2C interface       ← Already enabled in defconfig

  [*] Industrial I/O support --->
      <M> BMA220 accelerometer       ← Add your sensor driver
```

---

## 1.9 Common Configuration Options

### Essential Kernel Options

### 1. Module Support

**Location:** `General setup ---> Enable loadable module support`

```
[*] Enable loadable module support
  [*]   Module unloading
  [*]   Forced module unloading
  [*]   Module versioning support
  [*]   Source checksum for all modules
```

**Why important?**

- Allows drivers as modules (`.ko` files)
- Reduces kernel image size
- Flexible driver loading/unloading

**When to disable?**

- Minimal embedded systems
- Security-critical systems (prevent runtime modification)

### 2. Device Tree Support

**Location:** `Device Drivers ---> Device Tree and Open Firmware support`

```
[*] Device Tree and Open Firmware support
  [*]   Support for device tree overlays
```

**Required for:** ARM, ARM64, RISC-V, PowerPC (most embedded platforms)

### 3. GPIO Support

**Location:** `Device Drivers ---> GPIO Support`

```
[*] GPIO Support
  [*]   /sys/class/gpio/... (sysfs interface)
  [*]   Debug GPIO calls
  <M>   MCP23S08 I/O expander
```

### 4. I2C Support

**Location:** `Device Drivers ---> I2C support`

```
<*> I2C support
  <*>   I2C device interface            ← /dev/i2c-X
  [*]   I2C Hardware Bus support --->
      <*> IMX I2C interface
      <M> GPIO-based bitbanging I2C
```

### 5. SPI Support

**Location:** `Device Drivers ---> SPI support`

```
<*> SPI support
  <*>   User mode SPI device driver support  ← /dev/spidevX.Y
  [*]   SPI controller drivers --->
      <*> IMX SPI controller
```

### Configuration Tips

**Tip 1: Use Search Function**
Press `/` in menuconfig to search:

```
Symbol: GPIO_MCP23S08 [=m]
Type  : tristate
Prompt: Microchip MCP23xxx I/O expander
  Location:
    -> Device Drivers
      -> GPIO Support (GPIOLIB [=y])
  Defined at drivers/gpio/Kconfig:1234
  Depends on: (SPI_MASTER && !I2C) || I2C
```

**Tip 2: Check Dependencies**
Press `?` on an option to see:

- What it depends on
- What depends on it
- Help text
- Where it's defined

**Tip 3: Enable Help Text**
Always read help text (press `?`) before enabling unknown options.

**Tip 4: Save Configurations**

```bash
# Save current config to another name
cp .config config.backup

# Compare with original defconfig
diff .config arch/arm/configs/imx_v6_v7_defconfig

# Save as new minimal defconfig
make savedefconfig
```

---

## 1.10 Configuration for Specific Use Cases

### Embedded System (Size-Optimized)

**Goal:** Minimal kernel size

```
General setup --->
  [ ] Initial RAM filesystem  ← If not needed
  [ ] Support for paging of anonymous memory (swap)

Kernel Features --->
  [ ] Preemptible Kernel  ← Unless you need real-time

File systems --->
  [ ] Second extended fs  ← Disable unneeded filesystems
  [ ] NTFS file system support
```

### Real-Time System

**Goal:** Low latency, deterministic behavior

```
General setup --->
  Preemption Model (Fully Preemptible Kernel (RT)) --->

Kernel Features --->
  Timer frequency (1000 HZ) --->  ← Higher frequency
```

### Development/Debug Build

**Goal:** Maximum debugging capability

```
Kernel hacking --->
  [*] Kernel debugging
  [*] Enable __deprecated logic
  [*] Enable __must_check logic
  [*] Debug Filesystem
  [*] Memory Debugging
  [*] Compile the kernel with debug info
```