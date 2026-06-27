# Part 1. Environment Setup and Getting Source Code

This section covers the fundamentals of setting up your Linux kernel development environment, understanding the kernel source tree, and downloading kernel sources.

---

## 1.1 Introduction to Linux Kernel

### What is the Linux Kernel?

The **Linux kernel** is the core component of the operating system that:

- Manages system resources (CPU, memory, I/O devices)
- Provides interface between hardware and software
- Controls device drivers and system services
- Handles process scheduling and memory management

**Why Linux for Embedded Systems?**

- **Free and open source** - No licensing fees
- **Portable** - Runs on ARM, x86, RISC-V, MIPS, etc.
- **Large community** - Extensive documentation and support
- **Rich driver ecosystem** - Thousands of device drivers available
- **Long-term support (LTS)** - Stable releases with extended maintenance

### Linux Kernel Architecture

```
┌─────────────────────────────────────┐
│     User Space Applications         │
├─────────────────────────────────────┤
│      System Libraries (glibc)       │
├─────────────────────────────────────┤
│   System Call Interface (syscall)   │
├─────────────────────────────────────┤
│                                     │
│         Linux Kernel                │
│  ┌──────────────────────────────┐  │
│  │   Process Management         │  │
│  │   Memory Management          │  │
│  │   File System                │  │
│  │   Network Stack              │  │
│  │   Device Drivers   ←───────  │  │ (Our Focus)
│  └──────────────────────────────┘  │
├─────────────────────────────────────┤
│         Hardware Layer              │
└─────────────────────────────────────┘
```

---

## 1.2 Development Environment Setup

### Host and Target Concepts

**Host Machine**: Your development computer (typically x86/x86_64 running Linux)

- Powerful CPU (multi-core recommended)
- Sufficient RAM (minimum 8GB, 16GB recommended)
- Storage space (50GB+ for kernel sources)

**Target Machine**: Embedded device where code will run (e.g., ARM-based board)

- Limited resources
- Different CPU architecture
- May not have development tools installed

**Why can't we compile directly on target?**

- Target has limited CPU power (compilation is slow)
- Limited storage (kernel source tree is 1.3GB+)
- May lack development tools (gcc, make, etc.)

### Required Development Tools

### Essential Packages (Ubuntu/Debian)

```bash
# Update package list
sudo apt-get update

# Install development essentials
sudo apt-get install \
    build-essential \      # gcc, make, and basic tools
    git \                  # Version control
    libncurses-dev \       # For menuconfig interface
    flex \                 # Lexical analyzer
    bison \                # Parser generator
    libssl-dev \           # Crypto support
    libelf-dev \           # ELF format support
    bc \                   # Basic calculator (for kernel build)
    wget \                 # Download tool
    device-tree-compiler   # Device Tree compilation
```

**Why each tool?**

- **build-essential**: Provides GCC compiler, make, and basic compilation tools
- **git**: To download and manage kernel source code
- **libncurses-dev**: Required for `make menuconfig` text-based configuration interface
- **flex/bison**: Used to generate parsers during kernel build process
- **libssl-dev**: Needed for cryptographic functions and module signing
- **bc**: Used in kernel build scripts for calculations
- **device-tree-compiler**: Compiles Device Tree Source (.dts) to Binary (.dtb)

### Cross-Compilation Toolchain

**What is cross-compilation?**
Cross-compilation means compiling on one architecture (host) to produce binaries for a different architecture (target).

```
Host (x86_64) ──compile──> Binary for ARM
```

**Installing ARM Toolchain:**

```bash
# For ARM 32-bit (ARMv7)
sudo apt-get install gcc-arm-linux-gnueabihf

# For ARM 64-bit (ARMv8/Aarch64)
sudo apt-get install gcc-aarch64-linux-gnu
```

**Understanding Toolchain Naming:**`arm-linux-gnueabihf-gcc` breakdown:

- **arm**: Target architecture
- **linux**: Target operating system
- **gnueabihf**: ABI (Application Binary Interface)
    - **gnu**: GNU tools
    - **eabi**: Embedded Application Binary Interface
    - **hf**: Hard Float (hardware floating-point support)

**When to use which ABI?**

- **gnueabihf**: For ARM cores with hardware FPU (ARMv7-A Cortex-A series)
- **gnueabi**: For ARM cores without hardware FPU (older ARM cores)

### Workspace Organization

Create a structured workspace to keep your development organized:

```bash
# Create workspace directory
mkdir -p ~/embedded-workspace
cd ~/embedded-workspace

# Create subdirectories
mkdir -p kernel          # Linux kernel source
mkdir -p toolchain       # Cross-compilation tools
mkdir -p drivers         # Your driver development
mkdir -p devicetree      # Device Tree files
mkdir -p rootfs          # Root filesystem
mkdir -p build           # Build output
```

**Workspace structure:**

```
~/embedded-workspace/
├── kernel/              # Kernel sources (git clone here)
├── toolchain/           # Toolchain if not using system-wide
├── drivers/             # Your custom driver code
│   ├── char_driver/
│   ├── gpio_driver/
│   └── spi_driver/
├── devicetree/          # Custom .dts files
├── rootfs/              # Target root filesystem
└── build/               # Build outputs
```

---

## 1.3 Getting the Kernel Sources

### Kernel Versioning History

Understanding kernel versioning helps you choose the right version for your project.

**Evolution of Versioning:**

1. **Odd-Even System (until 2003)**
    - Odd numbers (2.1, 2.3, 2.5): Development/unstable
    - Even numbers (2.0, 2.2, 2.4): Stable/production
2. **Semantic Versioning (2003-2011): X.Y.Z**
    - X: Major version (backward incompatible changes)
    - Y: Minor version (new features, backward compatible)
    - Z: Patch version (bug fixes)
    - Example: 2.6.32 → 2.6.39
3. **X.Y Scheme (2011-2015): 3.Y**
    - Dropped the Z (patch) level
    - Example: 3.0, 3.1, ... 3.20
4. **Arbitrary Versioning (2015-present): X.Y**
    - Major number incremented when minor gets "too large"
    - Example: 3.20 → 4.0, 4.20 → 5.0, 5.19 → 6.0
    - Current: 6.x series

**Why did versioning change?**
Linus Torvalds decided semantic versioning wasn't meaningful for the kernel development model. The major number now changes arbitrarily when the minor number gets large enough (around 20).

### Kernel Version Types

**1. Mainline**

- Latest development version
- Released every 9-10 weeks
- Use for: Learning, experimentation, cutting-edge features
- Example: 6.7-rc1 (release candidate)

**2. Stable**

- Released shortly after mainline
- Receives bug fixes for several months
- Use for: Active development, recent hardware support
- Example: 6.6.1, 6.5.13

**3. Longterm (LTS)**

- Extended support: 2-6 years
- Critical bug fixes and security updates only
- Use for: Production products, long-term projects
- Example: 6.1 LTS, 5.15 LTS, 5.10 LTS, 5.4 LTS

**4. Vendor/BSP**

- Modified by SoC/board manufacturers
- Includes platform-specific drivers and patches
- Use for: Quick start with vendor hardware
- Example: NXP i.MX kernel, Raspberry Pi kernel

**Which version to choose?**

```
┌────────────────────────────────────────────┐
│ Requirement          │ Recommended Version │
├────────────────────────────────────────────┤
│ Learning/Practice    │ Mainline or Stable │
│ Product Development  │ LTS                │
│ Vendor Board        │ Vendor BSP first,  │
│                     │ then migrate to LTS│
│ Legacy Product       │ Matching LTS       │
└────────────────────────────────────────────┘
```

### Downloading Kernel Source

### Method 1: Git Clone (Recommended)

**Advantages:**

- Full version history
- Easy to switch between versions
- Can create patches
- Track your changes

```bash
cd ~/embedded-workspace/kernel

# Clone mainline kernel (large download ~3.5GB)
git clone git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
cd linux

# Clone stable kernel (recommended for beginners)
git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
cd linux

# List available tags (versions)
git tag | tail -20

# Checkout specific version
git checkout v6.1     # Specific stable version
git checkout v6.7-rc1 # Release candidate
```

**Understanding Git URLs:**

- **git.kernel.org**: Official kernel repository
- **torvalds/linux**: Linus Torvalds' mainline tree
- **stable/linux**: Stable release tree

### Method 2: Download Tarball

**Advantages:**

- Smaller download size
- No git history (saves space)
- Faster for single-version use

```bash
cd ~/embedded-workspace/kernel

# Download specific version
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.1.tar.xz

# Extract
tar xvf linux-6.1.tar.xz
cd linux-6.1
```

**When to use tarball?**

- Limited bandwidth
- Only need one specific version
- Production builds (reproducible)
- Automated build systems

### Method 3: Shallow Clone (Best of Both)

Combines benefits of git with smaller size:

```bash
# Clone only latest commit (much smaller)
git clone --depth=1 --branch v6.1 \
    git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git

# Later, if you need full history
cd linux
git fetch --unshallow
```

### Verifying Download

Always verify your kernel source authenticity:

```bash
# Check kernel version
cd linux
head Makefile

# You should see:
# VERSION = 6
# PATCHLEVEL = 1
# SUBLEVEL = 0
```

---

## 1.4 Understanding Kernel Source Tree

### Top-Level Directory Structure

```bash
cd linux
ls -F

arch/         # Architecture-specific code
block/        # Block layer code
certs/        # Certificates for module signing
crypto/       # Cryptographic API
Documentation/# Kernel documentation (IMPORTANT!)
drivers/      # Device drivers (LARGEST!)
fs/           # Filesystem implementations
include/      # Kernel headers
init/         # Initialization code
io_uring/     # Async I/O subsystem
ipc/          # Inter-Process Communication
kernel/       # Core kernel code
lib/          # Library routines
mm/           # Memory management
net/          # Networking stack
rust/         # Rust infrastructure (from 6.1+)
samples/      # Sample code
scripts/      # Build scripts and tools
security/     # Security modules
sound/        # Sound subsystem
tools/        # User-space tools
usr/          # initramfs support
virt/         # Virtualization support

COPYING       # GPL v2 license
CREDITS       # Contributor credits
Kconfig       # Top-level configuration
MAINTAINERS   # Subsystem maintainers
Makefile      # Top-level makefile
README        # Basic readme
```

### Key Directories Explained

### 1. arch/ - Architecture-Specific Code

Contains code specific to each CPU architecture:

```bash
arch/
├── arm/          # ARM 32-bit (ARMv5, ARMv6, ARMv7)
│   ├── boot/     # Boot code
│   │   ├── dts/  # Device Tree Source files
│   │   └── compressed/ # Kernel decompressor
│   ├── configs/  # Default configurations
│   ├── include/  # ARM-specific headers
│   ├── kernel/   # Core ARM kernel code
│   ├── lib/      # ARM-specific library functions
│   ├── mach-*/   # Machine/SoC-specific code
│   └── mm/       # ARM memory management
├── arm64/        # ARM 64-bit (ARMv8, Cortex-A53/57/72)
├── x86/          # x86 and x86_64
├── riscv/        # RISC-V
├── mips/         # MIPS
└── ...
```

**Why separation?**

- **Portability**: Core kernel code works across all architectures
- **Maintainability**: Each architecture team maintains their code
- **Clean API**: Architecture code must use standard kernel APIs

**Example: Boot process differs by architecture**

- **x86**: BIOS/UEFI → Bootloader → Kernel
- **ARM**: ROM Code → U-Boot → Kernel
- **RISC-V**: OpenSBI → U-Boot → Kernel

### 2. drivers/ - Device Drivers (Your Focus!)

**Largest directory** - Contains all hardware device drivers:

```bash
drivers/
├── char/         # Character device drivers
├── block/        # Block device drivers
├── net/          # Network device drivers
├── gpio/         # GPIO controllers
├── i2c/          # I2C bus and devices
│   ├── busses/   # I2C controller drivers
│   └── i2c-dev.c # User-space I2C access
├── spi/          # SPI bus and devices
├── usb/          # USB host and device
├── mmc/          # SD/MMC cards
├── input/        # Input devices (keyboard, mouse, touchscreen)
├── rtc/          # Real-Time Clock
├── pwm/          # PWM controllers
├── iio/          # Industrial I/O (ADC, DAC, sensors)
├── dma/          # DMA controllers
├── clk/          # Clock framework
├── pinctrl/      # Pin control
├── watchdog/     # Watchdog timers
└── ...           # 70+ subsystems
```

**Driver statistics (Linux 6.1):**

- Total drivers: ~61% of kernel code
- Most active subsystem
- Continuously growing

### 3. include/ - Header Files

```bash
include/
├── linux/        # Core kernel headers
│   ├── module.h  # Module infrastructure
│   ├── kernel.h  # Core kernel definitions
│   ├── device.h  # Device model
│   ├── slab.h    # Memory allocation
│   ├── gpio.h    # GPIO APIs
│   ├── i2c.h     # I2C APIs
│   └── ...
├── uapi/         # User-space API headers
│   └── linux/    # System call interface
├── asm-generic/  # Generic architecture headers
└── dt-bindings/  # Device Tree binding headers
```

**Important distinction:**

- **include/linux/**: Kernel internal use only
- **include/uapi/**: Exposed to user space
- **include/asm/**: Architecture-specific (auto-generated link)

### 4. Documentation/ - Your Best Friend!

```bash
Documentation/
├── driver-api/           # Driver development APIs
│   ├── gpio/             # GPIO subsystem documentation
│   ├── i2c.rst           # I2C driver guide
│   └── spi.rst           # SPI driver guide
├── devicetree/           # Device Tree documentation
│   └── bindings/         # DT binding specs
│       ├── gpio/         # GPIO controller bindings
│       ├── i2c/          # I2C device bindings
│       └── ...
├── process/              # Development process
│   ├── coding-style.rst  # Coding standards
│   └── submitting-patches.rst
├── translations/         # Non-English docs
└── ...
```

**How to use Documentation:**

1. Always check here first before asking on forums
2. Read subsystem documentation before writing drivers
3. Follow device tree binding specifications
4. Understand coding style requirements

### Source Code Size and Statistics

**Linux 6.1 Statistics:**

```
Total Files:     ~75,000 files
Total Lines:     ~30 million lines of code
Total Size:      ~1.3 GB (uncompressed)
Compressed Size: ~200 MB (tar.xz)
Compressed Binary: ~5-10 MB (typical embedded kernel)
```

**Size Breakdown (by lines of code):**

```
drivers/         60%  ← Most code here!
arch/            12%
fs/              4%
sound/           4%
net/             4%
include/         4%
tools/           3%
Documentation/   3%
kernel/          1%
mm/              1%
Others           4%
```

**Key Insight:** The kernel core is actually small! Most code is device drivers, which means as an embedded developer, you'll work primarily in the `drivers/` directory.

### Navigating the Source Tree

**Using command line:**

```bash
# Find a file
find . -name "*i2c*.h" | grep include

# Search for a function definition
grep -r "i2c_register_driver" --include="*.c"

# Count lines in a subsystem
find drivers/gpio -name "*.c" | xargs wc -l

# List files modified recently
git log --since="1 week ago" --name-only --pretty=format: | sort | uniq
```

**Using web browser - Elixir (Highly Recommended!):**

Visit: https://elixir.bootlin.com/linux/latest/source

Features:

- Browse any kernel version
- Fast symbol lookup (functions, structures, macros)
- See where symbols are defined and used
- Cross-reference functionality
- No need to download source locally

**Example: Looking up `gpio_chip` structure:**

1. Go to https://elixir.bootlin.com
2. Search "gpio_chip"
3. Click on definition
4. See all usages across kernel