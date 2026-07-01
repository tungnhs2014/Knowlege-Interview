# 29 - Framebuffer And Display Basics Example

## Goal
This is a **learning-only** fbdev userspace example. It draws a blue rectangle into `/dev/fb0` using framebuffer ioctls and `mmap()`.

It teaches:

- How userspace discovers framebuffer layout with `FBIOGET_VSCREENINFO` and `FBIOGET_FSCREENINFO`.
- Why `fix.line_length` must be used as the row stride.
- Why color bitfields matter more than guessing RGB byte order.
- How `/dev/fbN` is a userspace ABI exposed by fbdev or DRM fbdev emulation.

This is not production-ready. Production display software should normally use DRM/KMS, a display server, or a proper graphics library. Production display drivers should usually be DRM/KMS drivers rather than new fbdev drivers.

## Kernel Version Assumptions
Tested assumptions are compatible with common Linux 5.x/6.x systems that expose the legacy fbdev userspace ABI:

- `#include <linux/fb.h>` provides `struct fb_var_screeninfo`, `struct fb_fix_screeninfo`, `FBIOGET_VSCREENINFO`, and `FBIOGET_FSCREENINFO`.
- A framebuffer node such as `/dev/fb0` exists and supports writable `mmap()`.
- The active framebuffer is 16 bpp or 32 bpp packed pixels.

On modern desktop or embedded systems, `/dev/fb0` may be provided by native fbdev, `simplefb`, `simpledrm`, or DRM fbdev emulation.

## Files
| File | Purpose |
| --- | --- |
| `fb_draw_rect.c` | Userspace test that queries framebuffer layout, mmaps `/dev/fb0`, and draws a rectangle. |

No kernel module is included, so no kernel `Makefile` is needed.

## Build
Build with the target userspace C compiler:

```bash
cd Linux-Device-Driver/examples/29-framebuffer-and-display-basics
gcc -Wall -Wextra -O2 -o fb_draw_rect fb_draw_rect.c
```

For cross-compilation, replace `gcc` with your target compiler, for example:

```bash
aarch64-linux-gnu-gcc -Wall -Wextra -O2 -o fb_draw_rect fb_draw_rect.c
```

## Load / Unload
There is no kernel module to load or unload.

The example uses an existing framebuffer device:

```bash
ls -l /dev/fb*
ls /sys/class/graphics/
```

If no `/dev/fb0` exists, enable a framebuffer provider for your platform, use a system with DRM fbdev emulation, or skip this lab.

## Test
Run on a machine or board with a visible framebuffer console or display:

```bash
sudo ./fb_draw_rect /dev/fb0
```

If permissions allow direct access, `sudo` may not be required.

Expected terminal output resembles:

```text
fbdev: /dev/fb0
visible: 1024x768, virtual: 1024x768, offset: 0,0
bpp: 32, line_length: 4096, mapped: 3145728 bytes
red(off=16,len=8) green(off=8,len=8) blue(off=0,len=8)
```

Expected display result:

- A blue rectangle appears from roughly one-eighth of the screen to five-eighths of the screen.
- The program exits immediately after drawing.

## Debug Commands
Inspect framebuffer devices:

```bash
ls -l /dev/fb*
find /sys/class/graphics -maxdepth 2 -type f -print
```

Check basic properties:

```bash
cat /sys/class/graphics/fb0/name 2>/dev/null
cat /sys/class/graphics/fb0/virtual_size 2>/dev/null
cat /sys/class/graphics/fb0/bits_per_pixel 2>/dev/null
cat /sys/class/graphics/fb0/blank 2>/dev/null
```

Check kernel logs:

```bash
dmesg | grep -Ei 'fb|framebuffer|drm|simpledrm|simplefb|panel|backlight'
```

Blank and unblank, if supported:

```bash
echo 1 | sudo tee /sys/class/graphics/fb0/blank
echo 0 | sudo tee /sys/class/graphics/fb0/blank
```

If the screen stays black:

- Confirm `/dev/fb0` is the device connected to the visible display.
- Check whether the display is blanked.
- Check backlight, panel power, clocks, and DRM/simpledrm ownership in `dmesg`.
- Try `modetest` or `drm_info` when the display is owned by DRM/KMS.

## Cleanup
Remove the built userspace binary:

```bash
rm -f fb_draw_rect
```

The program modifies visible framebuffer memory. To restore the display:

- Switch virtual consoles.
- Clear the console.
- Restart the graphical session.
- Reboot the board if the display is in an unusual state.

Example:

```bash
clear
```

## Error Paths And What They Mean
The program exits cleanly on common failures:

| Error | Meaning | Fix |
| --- | --- | --- |
| `open /dev/fb0: No such file or directory` | No framebuffer node exists | Enable fbdev/DRM fbdev emulation or run on a target with `/dev/fb0`. |
| `open /dev/fb0: Permission denied` | User lacks access | Run with `sudo` or adjust device permissions for the lab. |
| `FBIOGET_*SCREENINFO` failure | Device does not support expected fbdev ioctls | Verify this is a real framebuffer node. |
| `unsupported <N> bpp` | Example only handles 16/32 bpp packed pixels | Extend the example for the target pixel format. |
| `mmap` failure | Device does not allow writable mmap or permissions/mapping attributes differ | Check driver support, permissions, and whether another stack owns the display. |

The code uses cleanup at each failure point:

- Closes the framebuffer fd after ioctl or bpp failures.
- Unmaps the framebuffer after drawing.
- Closes the fd before exit.

## Userspace ABI Impact
This example does not create a new ABI. It consumes the existing fbdev userspace ABI:

- Device node: `/dev/fb0` or another `/dev/fbN`.
- Ioctls: `FBIOGET_VSCREENINFO`, `FBIOGET_FSCREENINFO`.
- Memory mapping: `mmap()` on the framebuffer device.
- Optional sysfs debugging: `/sys/class/graphics/fbN/*`.

Because the ABI exposes direct pixel memory access, running the program changes visible display contents immediately.

## Why This Is Learning-only
This example intentionally avoids a fake kernel framebuffer module because fake display drivers are easy to make misleading. A real display driver must validate hardware-specific details:

- Panel timings and pixel clock.
- Backlight and power sequencing.
- Reset GPIO polarity.
- Regulator, clock, pinctrl, and power-domain integration.
- Framebuffer memory attributes and cache coherency.
- Firmware framebuffer handoff from `simplefb` or `simpledrm`.
- Whether the driver should be DRM/KMS instead of fbdev.

Use this lab to understand fbdev memory layout and debugging, not as a template for a new production display driver.
