# Part 3. Building the Kernel

This section covers the kernel build process, compilation commands, output files, and module installation for both native and cross-compilation scenarios.

---

## 1.11 Understanding the Build System

### What is Kbuild?

**Kbuild** is the Linux kernel build system based on GNU Make and Kconfig.

**Key components:**

- **Makefile**: Top-level makefile at kernel root
- **Kbuild files**: Distributed throughout source tree
- **scripts/**: Build scripts and helper tools
- **.config**: Configuration file (from menuconfig)

**Build flow:**

```
.config (configuration)
    ↓
Kbuild reads configuration
    ↓
Determines what to compile
    ↓
Invokes compiler (gcc/clang)
    ↓
Links object files
    ↓
Generates output (Image, modules, dtbs)
```

### Build Targets

The kernel Makefile supports many targets:

```bash
# View all available targets
make help

# Common targets
make help | grep -A 30 "Cleaning targets:"
make help | grep -A 30 "Configuration targets:"
make help | grep -A 30 "Architecture specific targets:"
```

**Essential targets:**

- `zImage` - Compressed kernel image (ARM)
- `bzImage` - Compressed kernel image (x86)
- `Image` - Uncompressed kernel image
- `modules` - All loadable modules
- `dtbs` - All Device Tree Binaries
- `all` - Default target (usually Image + modules + dtbs)

---

## 1.12 Native Compilation (x86)

### When to Use Native Compilation?

**Use cases:**

- Compiling kernel for your development PC
- Testing in virtual machine (QEMU, VirtualBox)
- Learning kernel basics
- Kernel debugging on x86

### Native Build Steps

**Step 1: Configure**

```bash
cd ~/embedded-workspace/kernel/linux

# No ARCH or CROSS_COMPILE needed (uses host by default)
make x86_64_defconfig  # For 64-bit x86
# OR
make i386_defconfig    # For 32-bit x86

# Customize if needed
make menuconfig
```

**Step 2: Build Kernel Image**

```bash
# Build kernel with all available CPU cores
make -j$(nproc)

# This builds:
# - vmlinux (uncompressed ELF - for debugging)
# - arch/x86/boot/bzImage (compressed bootable kernel)
```

**Why -j$(nproc)?**

- `$(nproc)` returns number of CPU cores
- Parallel compilation dramatically reduces build time
- Example: 8 cores → 8 files compiled simultaneously

**Build time comparison:**

```
Single thread (make):      45-90 minutes
Parallel (make -j8):       8-15 minutes  (8 core CPU)
Parallel (make -j16):      5-10 minutes  (16 core CPU)
```

**Step 3: Build Modules**

```bash
# Build all modules (already done if you ran 'make')
make modules

# Modules are created as .ko files throughout the tree:
find . -name "*.ko" | head -5
# ./drivers/char/lp.ko
# ./drivers/char/ppdev.ko
# ./drivers/gpio/gpio-mcp23s08.ko
```

**Step 4: Install Kernel**

```bash
# Install kernel to /boot (requires root)
sudo make install

# This installs:
# /boot/vmlinuz-6.1.0          ← Compressed kernel
# /boot/System.map-6.1.0       ← Symbol table
# /boot/config-6.1.0           ← Kernel config used
# /boot/initrd.img-6.1.0       ← Initial RAM disk

# Updates bootloader configuration (GRUB)
sudo update-grub  # On Debian/Ubuntu
```

**Step 5: Install Modules**

```bash
# Install modules to /lib/modules/$(uname -r)
sudo make modules_install

# Creates:
# /lib/modules/6.1.0/
# ├── kernel/           ← All .ko modules
# ├── modules.dep       ← Module dependencies
# ├── modules.alias     ← Module aliases
# └── ...
```

**Step 6: Reboot**

```bash
# Reboot to use new kernel
sudo reboot

# After reboot, verify
uname -r
# Output: 6.1.0
```

### Native Build Output Files

**Important files after build:**

```bash
# Main kernel image
arch/x86/boot/bzImage        # Compressed bootable kernel (3-8 MB)

# Debugging file
vmlinux                       # Uncompressed ELF (200-500 MB)

# Symbol information
System.map                    # Kernel symbol addresses

# Modules (examples)
drivers/net/ethernet/intel/e1000/e1000.ko
drivers/usb/storage/usb-storage.ko
drivers/gpu/drm/i915/i915.ko

# Configuration
.config                       # Used configuration
```

---

## 1.13 Cross-Compilation (ARM)

### Why Cross-Compilation?

**Problem with native compilation on embedded target:**

- Very slow CPU (hours to compile)
- Limited RAM (compilation fails)
- Limited storage (source tree won't fit)
- No development tools installed

**Solution: Cross-compile on powerful host, run on target**

### Setting Up Cross-Compilation

**Prerequisites:**

```bash
# Install ARM cross-compiler (if not done)
sudo apt-get install gcc-arm-linux-gnueabihf  # ARM 32-bit
# OR
sudo apt-get install gcc-aarch64-linux-gnu    # ARM 64-bit

# Verify installation
arm-linux-gnueabihf-gcc --version
```

**Environment setup:**

```bash
# Method 1: Export variables (recommended)
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

# Verify
echo $ARCH
# arm
echo $CROSS_COMPILE
# arm-linux-gnueabihf-
```

### Cross-Compilation Build Steps

**Step 1: Clean Previous Build (if any)**

```bash
cd ~/embedded-workspace/kernel/linux

# Remove previous build artifacts
make distclean
# OR keep .config
make mrproper
```

**Step 2: Configure for ARM**

```bash
# Set architecture
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

# Load default config for your SoC
make imx_v6_v7_defconfig    # For i.MX6/i.MX7

# Other common ARM defconfigs:
# make multi_v7_defconfig   # Generic ARMv7
# make sunxi_defconfig      # Allwinner
# make omap2plus_defconfig  # TI OMAP
# make exynos_defconfig     # Samsung

# Customize
make menuconfig
```

**Step 3: Build Kernel Image**

```bash
# Build compressed kernel image for ARM
make -j$(nproc) zImage

# Build progress output:
# CC      init/main.o
# CC      init/version.o
# LD      init/built-in.o
# ...
# Kernel: arch/arm/boot/zImage is ready

# Result:
ls -lh arch/arm/boot/zImage
# -rwxr-xr-x 1 user user 4.2M Dec 12 10:30 arch/arm/boot/zImage
```

**Why zImage for ARM?**

- **zImage**: Compressed kernel (most common for ARM)
- **Image**: Uncompressed kernel (rarely used)
- **uImage**: U-Boot wrapped image (legacy, less common now)

**Step 4: Build Device Trees**

```bash
# Build all device tree binaries for current ARCH
make dtbs

# Result: .dtb files in arch/arm/boot/dts/
ls arch/arm/boot/dts/*.dtb | head -5
# arch/arm/boot/dts/imx6q-sabrelite.dtb
# arch/arm/boot/dts/imx6q-udoo.dtb
# arch/arm/boot/dts/imx6dl-sabresd.dtb

# Build specific DTB only
make imx6q-udoo.dtb

# Verify DTB
file arch/arm/boot/dts/imx6q-udoo.dtb
# imx6q-udoo.dtb: Device Tree Blob version 17, size=48234
```

**Step 5: Build Modules**

```bash
# Build all modules
make -j$(nproc) modules

# Find built modules
find . -name "*.ko" | wc -l
# 2847  ← Example: 2847 modules built

# List some modules
find drivers/gpio -name "*.ko"
# drivers/gpio/gpio-mcp23s08.ko
# drivers/gpio/gpio-pca953x.ko
```

**Step 6: Install Modules (for target)**

```bash
# Create target rootfs directory (if not exists)
mkdir -p ~/embedded-workspace/rootfs

# Install modules to target directory
make INSTALL_MOD_PATH=~/embedded-workspace/rootfs modules_install

# Result:
tree -L 2 ~/embedded-workspace/rootfs/lib/modules/
# rootfs/lib/modules/
# └── 6.1.0/
#     ├── kernel/              ← All modules organized by subsystem
#     ├── modules.alias
#     ├── modules.dep          ← Module dependencies
#     ├── modules.dep.bin
#     ├── modules.symbols
#     └── ...
```

**Understanding INSTALL_MOD_PATH:**

- **Without it**: Installs to `/lib/modules` on host (WRONG!)
- **With it**: Installs to specified directory for target (CORRECT!)

### Cross-Compilation Output Files

**Output structure:**

```bash
arch/arm/boot/
├── zImage                    # Compressed kernel (~4-8 MB)
├── dts/
│   ├── imx6q-udoo.dtb       # Device tree binary (~40-60 KB each)
│   ├── imx6dl-sabresd.dtb
│   └── ...
└── compressed/
    └── piggy.gzip           # Compressed kernel data

Modules: (if INSTALL_MOD_PATH set)
~/embedded-workspace/rootfs/lib/modules/6.1.0/
└── kernel/
    ├── drivers/
    │   ├── gpio/
    │   │   └── gpio-mcp23s08.ko
    │   ├── i2c/
    │   ├── spi/
    │   └── ...
    ├── fs/
    ├── net/
    └── ...
```

---

## 1.14 Build Options and Optimization

### Parallel Compilation

**Basic usage:**

```bash
# Auto-detect CPU cores
make -j$(nproc)

# Explicit number of jobs
make -j8        # 8 parallel jobs
make -j16       # 16 parallel jobs

# Unlimited jobs (not recommended)
make -j         # May overload system
```

**How many jobs to use?**

```
CPU Cores    Recommended -j value
─────────    ────────────────────
2 cores      -j2 or -j4
4 cores      -j4 or -j8
8 cores      -j8 or -j16
16 cores     -j16 or -j32
```

**Why not use too many jobs?**

- I/O bottleneck (disk can't keep up)
- Memory pressure (too many gcc instances)
- Diminishing returns above 2× CPU cores

### Compiler Cache (ccache)

**Install ccache:**

```bash
sudo apt-get install ccache
```

**Enable ccache:**

```bash
# Method 1: Export variable
export CROSS_COMPILE="ccache arm-linux-gnueabihf-"

# Method 2: Add to PATH
export PATH=/usr/lib/ccache:$PATH

# Build
make -j$(nproc)
```

**Benefits:**

- **First build**: Normal speed (populates cache)
- **Rebuild**: 5-7x faster (uses cached objects)
- **After 'make clean'**: Much faster

**Example rebuild times:**

```
Without ccache:
  make clean; make -j8    →  12 minutes

With ccache:
  make clean; make -j8    →  2 minutes  (7x faster!)
```

### Verbose Build Output

**Normal build** (shows only summary):

```bash
make -j8
# CC      init/main.o
# LD      init/built-in.o
```

**Verbose build** (shows full commands):

```bash
make V=1 -j8
# arm-linux-gnueabihf-gcc -Wp,-MD,init/.main.o.d -nostdinc \
#   -isystem /usr/lib/gcc/arm-linux-gnueabihf/11/include \
#   -I./arch/arm/include -I./include -I./arch/arm/include/uapi \
#   ... (full command)
```

**When to use V=1?**

- Debugging build errors
- Understanding compiler flags
- Verifying cross-compiler usage

### Saving Build Time

**Tip 1: Don't clean unnecessarily**

```bash
# Only rebuilds changed files
make -j$(nproc)              # Good: incremental build

# Rebuilds everything (slow!)
make clean && make -j$(nproc)  # Avoid unless needed
```

**Tip 2: Build only what you need**

```bash
# Build only drivers subsystem
make drivers/ -j$(nproc)

# Build specific module
make drivers/gpio/gpio-mcp23s08.ko

# Build only device trees
make dtbs
```

**Tip 3: Use distcc (distributed compilation)**

```bash
# For large teams with multiple machines
sudo apt-get install distcc
# Configure to use multiple build servers
# Can reduce build time by 10x
```

---

## 1.15 Build Output and Installation

### Understanding Output Files

**After successful build:**

```bash
# Kernel image (compressed)
arch/arm/boot/zImage          # 4-8 MB

# Kernel image (uncompressed, for debugging)
vmlinux                        # 200-400 MB

# System map (symbol addresses)
System.map                     # 2-5 MB

# Device tree binaries
arch/arm/boot/dts/*.dtb       # 40-80 KB each

# Modules
drivers/**/*.ko                # 100-500 KB each
fs/**/*.ko
net/**/*.ko
```

### Installing to Target

**Method 1: Manual Copy (Simple)**

```bash
# Copy kernel image
scp arch/arm/boot/zImage root@target:/boot/

# Copy device tree
scp arch/arm/boot/dts/imx6q-udoo.dtb root@target:/boot/

# Copy modules
scp -r ~/embedded-workspace/rootfs/lib/modules/6.1.0 \
    root@target:/lib/modules/

# Reboot target
ssh root@target reboot
```

**Method 2: SD Card Installation**

```bash
# Mount SD card
sudo mount /dev/sdb1 /mnt/boot     # Boot partition
sudo mount /dev/sdb2 /mnt/rootfs   # Root partition

# Copy files
sudo cp arch/arm/boot/zImage /mnt/boot/
sudo cp arch/arm/boot/dts/imx6q-udoo.dtb /mnt/boot/
sudo cp -r ~/embedded-workspace/rootfs/lib/modules/6.1.0 \
    /mnt/rootfs/lib/modules/

# Sync and unmount
sync
sudo umount /mnt/boot /mnt/rootfs
```

**Method 3: Network Boot (TFTP + NFS)**

```bash
# Copy to TFTP server (for kernel and DTB)
sudo cp arch/arm/boot/zImage /var/lib/tftpboot/
sudo cp arch/arm/boot/dts/imx6q-udoo.dtb /var/lib/tftpboot/

# Copy modules to NFS root
sudo cp -r ~/embedded-workspace/rootfs/lib/modules/6.1.0 \
    /nfs/rootfs/lib/modules/

# Configure U-Boot on target to use TFTP/NFS
# (covered in bootloader chapter)
```

### Verifying Installation on Target

**Boot target and check:**

```bash
# SSH to target
ssh root@target

# Check kernel version
uname -r
# 6.1.0

# Check loaded modules
lsmod

# Check available modules
ls /lib/modules/$(uname -r)/kernel/drivers/

# Try loading a module
modprobe gpio-mcp23s08
lsmod | grep mcp23s08
```

---

## 1.16 Build Troubleshooting

### Common Build Errors

**Error 1: Missing dependencies**

```
*** Unable to find the ncurses libraries or the
*** required header files.
```

**Solution:**

```bash
sudo apt-get install libncurses-dev
```

**Error 2: Wrong compiler**

```
arm-linux-gnueabihf-gcc: command not found
```

**Solution:**

```bash
# Check CROSS_COMPILE
echo $CROSS_COMPILE

# Install toolchain
sudo apt-get install gcc-arm-linux-gnueabihf

# Or fix PATH
export PATH=/path/to/toolchain/bin:$PATH
```

**Error 3: ARCH not set**

```
  HOSTCC  scripts/basic/fixdep
  HOSTCC  scripts/kconfig/conf.o
  ...
  (compiling for x86 instead of ARM!)
```

**Solution:**

```bash
# Always set ARCH
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

# Or add to every make command
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- ...
```

**Error 4: Out of disk space**

```
No space left on device
```

**Solution:**

```bash
# Check available space
df -h

# Clean build
make distclean

# Remove old kernel sources
# Need at least 20GB free for kernel build
```

**Error 5: Build fails after config change**

```
Error: implicit declaration of function 'xxx'
```

**Solution:**

```bash
# Clean and rebuild
make mrproper
make imx_v6_v7_defconfig
make menuconfig
make -j$(nproc) zImage modules dtbs
```

### Cleaning Build

**Different clean levels:**

```bash
# 1. Clean most generated files (keeps .config)
make clean
# Removes: *.o, built-in.a, vmlinux, modules
# Keeps: .config

# 2. Remove all generated files + config
make mrproper
# Removes: everything from 'make clean' + .config
# Use when switching architecture or major config change

# 3. Distclean (mrproper + editor backups)
make distclean
# Removes: everything + *~ .orig .rej files

# 4. Remove only modules
make clean-modules
```

**When to clean?**

```
┌────────────────────────┬───────────────┐
│ Scenario               │ Clean Command │
├────────────────────────┼───────────────┤
│ Changed source file    │ (none)        │
│ Changed .config        │ (none)        │
│ Changed ARCH           │ mrproper      │
│ Major config change    │ mrproper      │
│ Switching kernel ver   │ mrproper      │
│ Build error            │ clean         │
└────────────────────────┴───────────────┘
```

### Build Performance Tips

**1. Use tmpfs for faster builds:**

```bash
# Mount tmpfs (if you have 32GB+ RAM)
sudo mount -t tmpfs -o size=20G tmpfs /tmp/kernel-build

# Copy source
cp -r ~/embedded-workspace/kernel/linux /tmp/kernel-build/

# Build in RAM (much faster!)
cd /tmp/kernel-build/linux
make -j$(nproc)
```

**2. Build only changed subsystems:**

```bash
# After modifying GPIO driver
make drivers/gpio/ -j$(nproc)

# Rebuild only modules
make modules -j$(nproc)
```

**3. Skip documentation:**

```bash
# Documentation takes time to build
# Disable if not needed
make -j$(nproc) NO_DOC=1
```

---

## 1.17 Build System Summary

### Complete Build Workflow

**For ARM Cross-Compilation:**

```bash
#!/bin/bash
# Complete build script for ARM

# Setup environment
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export KERNEL_DIR=~/embedded-workspace/kernel/linux
export INSTALL_PATH=~/embedded-workspace/rootfs

# Navigate to kernel
cd $KERNEL_DIR

# Configure
make imx_v6_v7_defconfig
make menuconfig  # Optional: customize

# Build everything
make -j$(nproc) zImage modules dtbs

# Install modules
make INSTALL_MOD_PATH=$INSTALL_PATH modules_install

# Summary
echo "Build complete!"
echo "Kernel: arch/arm/boot/zImage"
echo "DTBs:   arch/arm/boot/dts/*.dtb"
echo "Modules: $INSTALL_PATH/lib/modules/$(make kernelversion)"
```

**Save as:** `~/embedded-workspace/build-kernel.sh`

**Usage:**

```bash
chmod +x ~/embedded-workspace/build-kernel.sh
./build-kernel.sh
```

### Key Takeaways

✅ **Always set ARCH and CROSS_COMPILE** for cross-compilation

✅ **Use -j$(nproc)** for faster parallel builds

✅ **Use ccache** to speed up rebuilds

✅ **Set INSTALL_MOD_PATH** when installing modules for target

✅ **Build outputs:** zImage (kernel), *.dtb (device trees), *.ko (modules)

✅ **Clean selectively:** Don't `make clean` unnecessarily

✅ **Verify output files** before deploying to target