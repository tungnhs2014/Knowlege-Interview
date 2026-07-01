# 29 - Framebuffer And Display Basics

## Learning Goal
After this chapter, you should understand what a Linux framebuffer is, how legacy fbdev drivers expose display memory to userspace, and where that model fits beside modern DRM/KMS display drivers.

You should be able to:

- Explain `/dev/fb0` as a memory-oriented graphics ABI.
- Describe `struct fb_info`, `struct fb_ops`, `fb_var_screeninfo`, and `fb_fix_screeninfo`.
- Trace probe, registration, userspace `mmap()`, drawing, blanking, and removal.
- Debug common display symptoms such as black screen, wrong colors, shifted image, and mmap failure.
- Decide when fbdev is acceptable and when DRM/KMS is the better production path.

## Why This Matters In Real Work
Display bring-up often starts with a deceptively simple question: “Can I put pixels on the screen?” Framebuffer basics answer that question without requiring the full DRM/KMS stack on day one.

Framebuffer knowledge is useful when you:

- Bring up simple embedded displays or legacy SoC display controllers.
- Debug early boot graphics, console output, `/dev/fb0`, `simplefb`, or `simpledrm`.
- Maintain an older fbdev driver.
- Write userspace diagnostics that draw directly into a framebuffer.
- Need to understand why a modern DRM driver still creates `/dev/fb0` through fbdev emulation.

But there is an important production rule: **new display output drivers usually belong in DRM/KMS, not fbdev**, unless the hardware or product constraints strongly justify a legacy/simple framebuffer path.

## Mental Model
A framebuffer is a block of memory where bytes become pixels. The display controller repeatedly scans that memory and sends the pixel stream to a panel or monitor.

Think of it like this:

```text
userspace app
    |
    | open("/dev/fb0")
    | ioctl(FBIOGET_*SCREENINFO)
    | mmap()
    v
framebuffer memory  --->  display controller  --->  panel/monitor
```

The driver’s job is not only “allocate memory.” It must also make sure the display controller knows:

- Where the framebuffer memory is.
- How wide and tall the visible image is.
- How many bytes each line occupies.
- Which pixel format and color bit layout are used.
- Which clocks, regulators, panel reset lines, backlight, and timing registers are needed.

## Core Concepts
Framebuffer terminology is small, but the details bite. Most bugs come from assuming the pixel layout instead of asking the kernel what layout is active.

| Concept | Meaning | Practical Impact |
| --- | --- | --- |
| `fbdev` | Legacy Linux framebuffer subsystem | Exposes `/dev/fbN` and framebuffer ioctls. |
| `/dev/fb0` | Character device for framebuffer 0 | Userspace can read, write, seek, ioctl, and usually `mmap()` it. |
| Visible resolution | Current visible `xres` x `yres` | What the screen shows. |
| Virtual resolution | Backing buffer `xres_virtual` x `yres_virtual` | Allows panning/double-buffer-like layouts. |
| Offset | `xoffset`, `yoffset` | Visible window inside virtual framebuffer. |
| Stride / pitch | `fix.line_length` | Bytes per row. Never assume it is `xres * bytes_per_pixel`. |
| Bits per pixel | `var.bits_per_pixel` | Storage size per pixel, not enough by itself to know color order. |
| Color bitfields | `var.red`, `var.green`, `var.blue`, `var.transp` | Tell where color channels live inside a pixel. |
| Blanking | Turning scanout/panel/backlight off or on | Used by console, power management, and sysfs blank controls. |

### fbdev vs DRM/KMS vs V4L2
These subsystems can all mention “frame,” “video,” or “display,” but they are not the same API.

| Subsystem | Main Device Node | Main Purpose | Typical Use |
| --- | --- | --- | --- |
| fbdev | `/dev/fbN` | Legacy/simple pixel memory ABI | Simple drawing, fbcon, older display stacks. |
| DRM/KMS | `/dev/dri/cardN` | Modern display mode setting and scanout | New display drivers, planes, CRTCs, connectors, GEM/DMABUF. |
| V4L2 | `/dev/videoN` | Video capture/output and media devices | Cameras, capture buffers, codecs, some video output paths. |

**Interview trap:** a DRM framebuffer object is not the same thing as an fbdev `/dev/fb0` device. Both describe displayable memory, but the API, lifetime, and object model differ.

## Kernel Mechanism
The fbdev core owns the generic character-device layer. A normal framebuffer driver usually does not register its own `struct cdev` and `struct file_operations`; it registers a `struct fb_info` with the framebuffer core.

The split looks like this:

```text
userspace system calls
    |
    v
fbdev core file operations
    |
    v
driver-provided struct fb_ops
    |
    v
hardware registers, framebuffer memory, panel/backlight/power
```

Important ownership rules:

- `struct fb_info` is the central kernel object for one framebuffer instance.
- Driver-private data normally lives behind `info->par`.
- The framebuffer memory must remain valid while userspace, fbcon, or fbdev core can access it.
- Unregister the framebuffer before freeing `fb_info` or the backing memory.
- Do not program hardware from `.fb_check_var`; validate there and commit in `.fb_set_par`.

## Key Structs And APIs
Framebuffer APIs are easier to remember when tied to who consumes them: userspace, fbdev core, or your hardware driver.

### `struct fb_var_screeninfo`
This is user-visible, mutable screen information. Userspace can query it with `FBIOGET_VSCREENINFO` and may request changes with `FBIOPUT_VSCREENINFO`.

Key fields:

- `xres`, `yres`: visible resolution.
- `xres_virtual`, `yres_virtual`: backing buffer resolution.
- `xoffset`, `yoffset`: visible area inside virtual buffer.
- `bits_per_pixel`: storage bits per pixel.
- `red`, `green`, `blue`, `transp`: color channel bit layout.
- `pixclock`, margins, sync lengths: timing-related fields.
- `rotate`: requested rotation.

### `struct fb_fix_screeninfo`
This is user-visible information that is fixed or mode-derived. Userspace queries it with `FBIOGET_FSCREENINFO`.

Key fields:

- `smem_start`, `smem_len`: framebuffer physical memory information.
- `line_length`: bytes per scanline.
- `type`, `visual`: framebuffer organization and color model.
- `xpanstep`, `ypanstep`, `ywrapstep`: panning capability.
- `mmio_start`, `mmio_len`: MMIO information for the device.
- `accel`: acceleration identifier/capability.

**Production warning:** treat physical address exposure carefully. Modern systems may hide or restrict physical framebuffer addresses because leaking them can be unsafe.

### `struct fb_info`
This is the kernel’s per-framebuffer object.

Important fields:

- `var`: current variable mode information.
- `fix`: fixed/mode-derived information.
- `cmap`: color map state.
- `fbops`: driver callbacks.
- `screen_base`: kernel virtual address for framebuffer memory, often `__iomem` or system memory.
- `screen_size`: size of mapped framebuffer memory.
- `device`, `dev`: parent and framebuffer device objects.
- `par`: private driver data.
- optional backlight pointer when configured.

Lifecycle helpers:

```c
struct fb_info *framebuffer_alloc(size_t size, struct device *dev);
void framebuffer_release(struct fb_info *info);
int register_framebuffer(struct fb_info *info);
void unregister_framebuffer(struct fb_info *info);
int devm_register_framebuffer(struct device *dev, struct fb_info *info);
```

### `struct fb_ops`
These callbacks are the driver’s hardware-facing operations. The fbdev core calls them when userspace, fbcon, or framebuffer helpers need real driver behavior.

Common callbacks:

- `.fb_check_var`: validate or adjust a requested `fb_var_screeninfo`.
- `.fb_set_par`: program hardware from `info->var`.
- `.fb_setcolreg`: program one color register.
- `.fb_setcmap`: program a color map.
- `.fb_blank`: blank, unblank, or power down the display path.
- `.fb_pan_display`: move visible area inside virtual framebuffer.
- `.fb_fillrect`: fill a rectangle.
- `.fb_copyarea`: copy one rectangle to another area.
- `.fb_imageblit`: draw an image.
- `.fb_mmap`: map framebuffer memory to userspace when custom behavior is needed.
- `.fb_ioctl`: handle driver-specific ioctls not handled by the core.

Software fallback helpers include `cfb_fillrect()`, `cfb_copyarea()`, and `cfb_imageblit()`. Newer kernels also expose helper families for I/O memory and system memory backed framebuffers.

## Lifecycle / Data Flow
Framebuffer bring-up is a chain of ownership decisions. The exact hardware varies, but the order below is the shape you should expect.

```text
probe()
  -> get MMIO/DMA memory/resources
  -> enable clocks/regulators/pinctrl as needed
  -> allocate or map framebuffer memory
  -> framebuffer_alloc()
  -> fill fb_info: var, fix, screen_base, screen_size, fbops, par
  -> register_framebuffer()
  -> display can be used by fbdev core, fbcon, and userspace

userspace draw
  -> open("/dev/fb0")
  -> ioctl(FBIOGET_VSCREENINFO)
  -> ioctl(FBIOGET_FSCREENINFO)
  -> mmap()
  -> calculate offset with line_length and color bitfields
  -> write pixels

mode or blank change
  -> fbdev core receives ioctl/sysfs/fbcon request
  -> calls fb_ops callback
  -> driver validates or programs hardware

remove()
  -> unregister_framebuffer()
  -> stop scanout / blank display
  -> free framebuffer memory and release fb_info
  -> disable hardware resources
```

The order matters. If you free framebuffer memory while `/dev/fb0` is still registered, userspace or fbcon can touch stale memory. That becomes a use-after-free with a very bright visual symptom: the screen may corrupt before the kernel complains.

## Minimal Practical Example
This userspace example is **learning-only**. It draws a rectangle on `/dev/fb0` when a writable framebuffer exists. It is not a production graphics stack and does not handle every pixel format.

```c
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

static uint32_t pack_rgb888(const struct fb_var_screeninfo *var,
                            uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t pixel = 0;

    pixel |= ((uint32_t)(r >> (8 - var->red.length))) << var->red.offset;
    pixel |= ((uint32_t)(g >> (8 - var->green.length))) << var->green.offset;
    pixel |= ((uint32_t)(b >> (8 - var->blue.length))) << var->blue.offset;

    return pixel;
}

int main(void)
{
    int fd = open("/dev/fb0", O_RDWR);
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    size_t map_len;
    uint8_t *fb;

    if (fd < 0) {
        perror("open /dev/fb0");
        return 1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0 ||
        ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0) {
        perror("FBIOGET_*SCREENINFO");
        close(fd);
        return 1;
    }

    map_len = (size_t)fix.line_length * var.yres_virtual;
    fb = mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    if (var.bits_per_pixel == 32) {
        uint32_t color = pack_rgb888(&var, 0x20, 0x80, 0xff);

        for (unsigned int y = 40; y < 180 && y < var.yres; y++) {
            for (unsigned int x = 40; x < 260 && x < var.xres; x++) {
                size_t off = (size_t)(y + var.yoffset) * fix.line_length +
                             (size_t)(x + var.xoffset) * 4;
                *(uint32_t *)(fb + off) = color;
            }
        }
    } else {
        fprintf(stderr, "This learning example only handles 32 bpp safely\n");
    }

    munmap(fb, map_len);
    close(fd);
    return 0;
}
```

Important details:

- It uses `fix.line_length`, not `var.xres * bytes_per_pixel`.
- It includes `xoffset` and `yoffset`.
- It packs color using the framebuffer color bitfields.
- It does not assume every `/dev/fb0` supports writes or mmap.
- It does not replace DRM/KMS, Wayland, X.org, or a real display server.

## Common Bugs And Debugging
Framebuffer bugs are wonderfully visual, which is polite of them. The hard part is mapping the symptom back to the wrong assumption.

| Symptom | Likely Causes | What To Check |
| --- | --- | --- |
| `/dev/fb0` missing | No fbdev, no DRM fbdev emulation, driver failed probe | `dmesg`, `/sys/class/graphics`, kernel config, `ls /dev/fb*`. |
| `mmap()` fails | Driver/core does not support mmap, permissions, wrong device | `errno`, device permissions, `strace`, driver logs. |
| Image shifted or slanted | Wrong stride calculation | `FBIOGET_FSCREENINFO`, `fix.line_length`. |
| Wrong colors | Wrong channel order or bpp assumption | `FBIOGET_VSCREENINFO`, color bitfields. |
| Drawing appears off-screen | Ignored `xoffset`/`yoffset` or virtual resolution | `var.xoffset`, `var.yoffset`, `xres_virtual`, `yres_virtual`. |
| Black screen but `/dev/fb0` exists | Backlight off, blanked display, clocks/regulators/panel reset missing, scanout disabled | `/sys/class/graphics/fb0/blank`, backlight sysfs, `dmesg`, regulator/clock logs. |
| Kernel crash on unload | Freed memory before unregister, userspace mapping still active, missing lifetime handling | remove path, `unregister_framebuffer()` ordering, ref/lifetime rules. |
| Works on boot, fails after suspend | Missing blank/suspend/resume sequencing | PM callbacks, clocks, regulators, panel reset, backlight order. |

Useful commands:

```bash
ls -l /dev/fb*
ls /sys/class/graphics/
cat /sys/class/graphics/fb0/name 2>/dev/null
cat /sys/class/graphics/fb0/virtual_size 2>/dev/null
cat /sys/class/graphics/fb0/bits_per_pixel 2>/dev/null
cat /sys/class/graphics/fb0/blank
dmesg | grep -Ei 'fb|framebuffer|drm|simpledrm|simplefb|panel|backlight'
```

If available:

```bash
fbset -i
modetest
drm_info
```

Debugging checklist:

- Confirm whether `/dev/fb0` is native fbdev, `simplefb`, `simpledrm`, or DRM fbdev emulation.
- Query screen info before drawing.
- Use simple full-screen fills: red, green, blue, white, black.
- If colors are swapped, inspect color bitfields.
- If rows drift diagonally, inspect stride.
- If nothing lights up, check blank state, backlight, power rails, clocks, reset GPIO, pinctrl, and panel enable order.
- If modern DRM owns the display, debug through DRM tools instead of assuming a legacy fbdev driver owns the hardware.

## Production Checklist
Before reviewing or shipping display-related code, verify the basics with real hardware.

Driver and lifetime:

- `fb_info` is allocated and released correctly.
- Framebuffer memory remains valid until after unregister.
- Remove path unregisters before freeing memory.
- Error paths unwind resources in reverse order.
- `.fb_check_var` validates only; `.fb_set_par` commits hardware state.
- `.fb_blank` handles power/backlight/clock sequencing safely.

Memory and pixel layout:

- `fix.line_length` is correct for hardware alignment.
- `smem_len` and mmap size cannot overrun the real buffer.
- Pixel bitfields match the actual controller/panel format.
- Cacheability and write-combining are appropriate.
- Slow bus-backed displays use suitable shadow/deferred I/O strategies.

Hardware integration:

- Clocks, regulators, reset GPIOs, pinctrl states, panel timing, and backlight polarity are board-validated.
- Suspend/resume and blank/unblank paths restore the display reliably.
- Firmware framebuffer handoff is handled when a native driver replaces `simplefb` or `simpledrm`.
- Physical address exposure is avoided or intentionally justified.

Architecture choice:

- For new display-output drivers, evaluate DRM/KMS first.
- Use fbdev mainly for legacy maintenance, simple diagnostics, compatibility, or constrained simple hardware.
- Do not confuse V4L2 capture buffers, DRM framebuffer objects, and fbdev `/dev/fbN`.

## Interview Readiness
You are ready for framebuffer/display basics interviews when you can explain the system as a flow, not as a list of structs.

Be ready to answer:

- What happens when userspace writes to `/dev/fb0`?
- Why is `fix.line_length` more important than `xres * bytes_per_pixel`?
- Why is `fb_ops` not the same as `file_operations`?
- What does `.fb_check_var` do versus `.fb_set_par`?
- Why might a screen stay black even though `/dev/fb0` exists?
- Why do modern display drivers usually use DRM/KMS?
- What is the difference between an fbdev framebuffer and a DRM framebuffer object?

See `interview/29-framebuffer-and-display-basics.md` for deeper question sets and debugging scenarios.

## Kernel Version Notes
Framebuffer APIs are old, and old examples often need careful reading on newer kernels.

- `unregister_framebuffer()` is `void` in Linux 6.8 headers; do not write code that expects a return value.
- `devm_register_framebuffer()` exists in current headers and can simplify cleanup in some drivers.
- Modern kernels commonly use DRM/KMS with `DRM_FBDEV_EMULATION` for legacy `/dev/fbN` compatibility.
- `simpledrm` and `simplefb` may expose early boot framebuffers before native graphics drivers take over.
- Current fbdev helper families include system-memory and I/O-memory helpers beyond the older `cfb_*` routines often shown in legacy examples.
