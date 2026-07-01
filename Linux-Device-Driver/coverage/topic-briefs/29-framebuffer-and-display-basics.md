# Topic Brief - 29 - Framebuffer And Display Basics

## Output Targets
- Knowledge: `knowledge/29-framebuffer-and-display-basics.md`
- Interview: `interview/29-framebuffer-and-display-basics.md`
- Example: `examples/29-framebuffer-and-display-basics/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch21` | `docs/Linux Device Driver Development/Chapter 21-Framebuffer Drivers.md` | read/mapped/covered/merged | Primary framebuffer source: `/dev/fbX`, framebuffer memory model, mode setting, `fb_var_screeninfo`, `fb_fix_screeninfo`, `fb_cmap`, `fb_info`, `fb_ops`, registration lifecycle, accelerated/non-accelerated callbacks, userspace `mmap()`/ioctl access, raw dump/restore, and `/sys/class/graphics/fbN/blank`. |
| `ldd1-ch04` | `docs/Linux Device Driver Development/Chapter 4-Character Device Drivers.md` | read/mapped/covered-adjacent | Character-device foundation: `struct file_operations`, `mmap`, `ioctl`, missing-callback error behavior, and userspace ABI thinking. Used to explain how fbdev presents as a character device while the fbdev core maps generic file operations to driver `fb_ops`. |
| `ldd1-ch05` | `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md` | read/mapped/covered-adjacent | Platform resource context: MMIO resources, IRQs, `ioremap()` discussion pointer. Used for framebuffer controller probe/resource framing. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/covered-adjacent | DT/platform resource extraction with `devm_ioremap_resource()`. Used for display-controller register mapping context, not as a display binding source. |
| `ldd1-ch11` | `docs/Linux Device Driver Development/Chapter 11-Kernel Memory Management.md` | read/mapped/covered-adjacent | Memory mapping foundation: MMIO reservation, `ioremap()`, `__iomem`, `remap_pfn_range()`, `io_remap_pfn_range()`, and implementing `.mmap`. Used to explain framebuffer memory mapping and the difference between kernel MMIO mapping and userspace mapping. |
| `ldd1-ch12` | `docs/Linux Device Driver Development/Chapter 12-DMA - Direct Memory Access.md` | searched/mapped/adjacent | DMA chapter was searched for buffer/mmap context. No display-specific content merged beyond the idea that framebuffer memory may be DMA/coherent memory in some drivers. Detailed DMA remains topic 21. |
| `ldd2-source-root` | `docs/Linux Device Driver Development 2/` | searched/mapped/gap | No dedicated framebuffer, fbdev, DRM, or display-output chapter found. Relevant adjacent V4L2/media, MFD backlight, power-domain, and DRM/DMABUF references are mapped separately. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/covered-adjacent | MFD example lists ADP5520 backlight as a child subsystem. Useful for explaining that display brightness/backlight is often handled by another framework, not necessarily by the framebuffer driver itself. |
| `ldd2-ch07` | `docs/Linux Device Driver Development 2/Chapter 7-Demystifying V4L2 and Video Capture Device Drivers.md` | read/mapped/related | V4L2/video source: `/dev/videoX`, video-device mmap path, media entities, and a mention that video-mode subdevices can include framebuffer. Used only as comparison with display/output and future V4L2 topics. |
| `ldd2-ch08` | `docs/Linux Device Driver Development 2/Chapter 8-Integrat with V4L2.md` | read/mapped/related | Media graph and async framework source: DRM also uses graph bindings; MIPI D-PHY targets cameras/displays; V4L2 can cover specific display/mem2mem devices but media controller exists for complex pipelines. Used to position modern display context without merging V4L2. |
| `ldd2-ch09` | `docs/Linux Device Driver Development 2/Chapter 9-Leveraging the V4L2.md` | read/mapped/related | Userspace video buffer source: V4L2 format negotiation, pixel formats, MMAP buffers, DMABUF/DRM handoff, `drmModeAddFB2()`, `drmModeSetPlane()`, and raw image conversion. Used to compare framebuffer memory access with modern zero-copy video-to-display paths. |
| `ldd2-ch10` | `docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md` | read/mapped/covered-adjacent | Power-domain example mentions video core IP sharing a rail with display IP. Used for production display-driver power sequencing context; detailed PM remains topic 24. |
| `notion-source-root` | `docs/Linux-Device-Driver-Notion/` | searched/mapped/gap | No standalone Notion framebuffer/fbdev/display-output chapter found. Relevant mmap, SPI display-controller, V4L2-to-DRM, DRM module, LCD GPIO, and configuration snippets are mapped separately. |
| `notion-ch01-part2` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 2 Kernel Configuration.md` | read/mapped/incidental | Mentions menuconfig display mode only; not technical display-driver content. Mapped to show it was not skipped. |
| `notion-ch01-part3` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 3 Building the Kernel.md` | read/mapped/incidental | Module output example includes `drivers/gpu/drm/i915/i915.ko`; orientation only for modern DRM location. |
| `notion-ch02-part1` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 1 User Space vs Kernel Space & Module Concept.md` | read/mapped/covered-adjacent | System-call-to-driver mapping includes `mmap() -> driver's .mmap()`. Used to reinforce userspace-to-kernel path behind framebuffer memory mapping. |
| `notion-ch04-part2` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 2 Basic File Operations.md` | read/mapped/covered-adjacent | `file_operations` overview includes `.mmap` and `.unlocked_ioctl`. Used as adjacent char-device ABI context. |
| `notion-ch08-part1-spi` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 1 SPI Architecture and Driver Structures.md` | read/mapped/covered-adjacent | Lists SPI display controllers such as ILI9341/ST7735. Used to note that small displays may be SPI-connected and may use fbdev/tiny DRM-style drivers rather than MMIO display-controller assumptions. |
| `notion-ch09-userspace-part1` | `docs/Linux-Device-Driver-Notion/Chapter 9-userspace-part1.md` | read/mapped/related | V4L2 userspace buffer comparison: MMAP, USERPTR, DMABUF, and Camera -> Display V4L2 -> DRM use case. Used as modern display pipeline comparison only. |
| `notion-ch09-userspace-part2` | `docs/Linux-Device-Driver-Notion/Chapter 9-userspace-part2.md` | read/mapped/related | Detailed V4L2-to-DRM DMABUF snippet using `drmModeAddFB2WithModifiers()` and `drmModeSetPlane()`. Used to contrast legacy `/dev/fbX` writes with modern DRM/KMS framebuffer objects. |
| `notion-ch14-part1` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 1 Pin Control Framework.md` | read/mapped/related-false-positive | BeagleBone Black 16x2 GPIO character LCD example. Mapped as a display-adjacent false positive: character LCD over GPIO is not framebuffer/graphics display. |
| `notion-ch14-part3` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 3 Userspace GPIO Access.md` | read/mapped/related-false-positive | Userspace GPIO 16x2 LCD example. Mapped as display-adjacent but out of scope for framebuffer graphics. |

## Source Files Read
- `ldd1-ch21`: complete chapter. Read framebuffer model, driver data structures, `fb_ops`, platform-style probe/remove sketch, `fb_check_var`, `fb_set_par`, `fb_blank`, accelerated callbacks, software fallback callbacks, userspace `/dev/fb0` mmap/ioctl example, raw dump/restore, and sysfs blanking.
- `ldd1-ch04`: targeted `file_operations`, `.mmap`, `.unlocked_ioctl`, missing callback behavior, and ioctl ABI sections.
- `ldd1-ch05`: targeted platform-resource/ioremap references.
- `ldd1-ch06`: targeted `platform_get_resource()` and `devm_ioremap_resource()` examples.
- `ldd1-ch11`: targeted MMIO, `__iomem`, `ioremap()`, `remap_pfn_range()`, `io_remap_pfn_range()`, and `.mmap` implementation sections.
- `ldd1-ch12`: searched for DMA/mmap/framebuffer relevance; no display-specific content merged.
- `ldd2-ch03`: targeted MFD/backlight mention.
- `ldd2-ch07`: targeted V4L2 architecture, video node mmap path, and framebuffer mention in video-mode subdevice context.
- `ldd2-ch08`: targeted graph binding/DRM mention, MIPI D-PHY camera/display mention, and V4L2/display/mem2mem positioning.
- `ldd2-ch09`: targeted V4L2 userspace APIs, display-window wording, pixel format/stride notes, MMAP buffers, DMABUF/DRM handoff, and raw image display/conversion notes.
- `ldd2-ch10`: targeted display/video shared power-domain context.
- `notion-ch01-part2`: targeted incidental menuconfig display-mode phrase; no topic content merged.
- `notion-ch01-part3`: targeted `drivers/gpu/drm/i915/i915.ko` build-output example.
- `notion-ch02-part1`: targeted syscall mapping including `.mmap`.
- `notion-ch04-part2`: targeted file operation and `.mmap` overview.
- `notion-ch08-part1-spi`: targeted SPI display-controller examples.
- `notion-ch09-userspace-part1`: targeted MMAP/USERPTR/DMABUF comparison and Camera -> Display note.
- `notion-ch09-userspace-part2`: targeted DMABUF zero-copy Camera -> Display DRM snippet.
- `notion-ch14-part1`: targeted 16x2 GPIO LCD DT example as a false positive.
- `notion-ch14-part3`: targeted 16x2 GPIO LCD userspace example as a false positive.

### Inventory Decisions
- `ldd1-ch21` is the only dedicated internal framebuffer source and is primary for topic 29.
- `ldd2` has no dedicated framebuffer/display-output chapter. Its V4L2 chapters are related, but they belong primarily to topics 32-34. Only the display-output, pixel-format, MMAP, and DRM handoff comparisons should influence topic 29.
- Notion has no dedicated framebuffer/fbdev chapter. Notion coverage is still recorded through adjacent mmap/file-operation, SPI display-controller, V4L2-to-DRM, and LCD false-positive sources.
- Same-number chapters were not merged across source groups. For example, `ldd1-ch21` is framebuffer, while `ldd2` has no chapter 21; Notion chapter numbers map to different topics and were inspected separately.
- Character LCD examples are intentionally marked false positives. They teach display-like GPIO control, but not framebuffer memory, pixels, mode setting, or graphics scanout.

## Merged Source Notes
- Core mental model:
  - A framebuffer is a memory-backed view of what the display controller scans out to a screen.
  - Userspace can treat `/dev/fbN` like a special character device whose main data payload is pixel memory.
  - The framebuffer driver programs display mode/timing, exposes fixed and variable screen information, and provides callbacks used by the fbdev core and framebuffer console.
- Primary fbdev userspace model from `ldd1-ch21`:
  - Open `/dev/fb0` or another `/dev/fbN`.
  - Query variable screen information with `FBIOGET_VSCREENINFO`.
  - Query fixed screen information with `FBIOGET_FSCREENINFO`.
  - Use `fix.line_length`, `var.xres`, `var.yres`, `var.xoffset`, `var.yoffset`, and `var.bits_per_pixel` to compute byte offsets.
  - `mmap()` the framebuffer and write pixels directly.
  - Optional sysfs blanking through `/sys/class/graphics/fbN/blank`.
- Main kernel structures from `ldd1-ch21`:
  - `struct fb_var_screeninfo`: user-visible mutable mode data: visible/virtual resolution, offsets, bits per pixel, timing, margins, sync lengths, rotation, and color bitfields.
  - `struct fb_fix_screeninfo`: user-visible fixed or mode-derived data: framebuffer physical address, memory length, type, visual, panning steps, `line_length`, MMIO address/length, and acceleration capability.
  - `struct fb_cmap`: color map table for palette-based or color-map operations.
  - `struct fb_info`: kernel-only per-framebuffer object containing `var`, `fix`, `cmap`, `fbops`, `screen_base`, `screen_size`, `device`, `dev`, optional backlight link, and private data in `par`.
  - `struct fb_ops`: hardware-facing callbacks used by fbdev core and fbcon, not the same as `struct file_operations`.
- Lifecycle from `ldd1-ch21` plus adjacent platform/memory sources:
  - Probe obtains resources such as MMIO region, clocks, regulators, DMA/coherent memory, and panel/backlight-related resources.
  - Driver allocates `fb_info` with `framebuffer_alloc(size, dev)` so private data can live in `info->par`.
  - Driver fills `info->var`, `info->fix`, `info->fbops`, `info->screen_base`, `info->screen_size`, and private state.
  - Driver validates mode information through `.fb_check_var` and applies controller settings through `.fb_set_par`.
  - Driver registers with `register_framebuffer(info)` or modern managed registration where available.
  - Remove path unregisters the framebuffer before releasing memory and disabling hardware.
- `fb_ops` behavior:
  - `.fb_check_var` validates or adjusts user-requested mode data before committing it. It should not program hardware.
  - `.fb_set_par` programs hardware based on `info->var`.
  - `.fb_setcolreg`/`.fb_setcmap` update palette/color registers.
  - `.fb_blank` handles display blanking and power-down/unblank sequencing.
  - `.fb_pan_display` changes visible offset inside virtual framebuffer memory.
  - `.fb_fillrect`, `.fb_copyarea`, and `.fb_imageblit` are 2D drawing helpers. Hardware acceleration is optional; `cfb_*` and current helper families provide software fallbacks.
  - `.fb_mmap`, `.fb_read`, `.fb_write`, and `.fb_ioctl` are lower-level hooks called through fbdev core paths, not direct `file_operations` for the driver author in the usual fbdev model.
- Display memory details:
  - `fix.line_length` is the stride/pitch in bytes and must be used instead of assuming `xres * bytes_per_pixel`.
  - `var.xoffset`/`var.yoffset` matter when virtual resolution or panning is used.
  - Pixel format is determined by `bits_per_pixel` and color bitfields, not just by assuming RGB byte order.
  - Raw framebuffer dumps are only meaningful with matching resolution, stride, bpp, endianness, and format.
- Relationship to character devices:
  - `ldd1-ch04` and Notion char-device notes explain the system-call side: `open`, `read`, `write`, `ioctl`, and `mmap`.
  - `ldd1-ch21` explains that fbdev core owns the generic character-device/file-operation layer in `drivers/video/fbdev/core/fbmem.c` and dispatches into driver `fb_ops`.
  - Learner docs should explicitly warn not to implement a normal `cdev` plus `file_operations` when writing an fbdev driver unless building something outside the fbdev framework.
- Relationship to memory mapping:
  - `ldd1-ch11` explains that userspace cannot directly touch device memory; the kernel maps appropriate pages into the process VMA through mmap infrastructure.
  - For I/O memory, `io_remap_pfn_range()` or core fb helpers may be relevant; for RAM-backed framebuffers, system-memory helpers are more appropriate.
  - `ioremap()` is for kernel virtual access to MMIO/device memory; it is not how userspace gets access.
- Relationship to DRM/KMS and V4L2:
  - `ldd1-ch21` says framebuffer drivers are simpler than X.org/KMS drivers and should not be confused with KMS/GPU drivers.
  - `ldd2-ch09` and Notion V4L2 notes show modern video/display pipelines using DMABUF and DRM APIs such as `drmModeAddFB2()`/`drmModeAddFB2WithModifiers()` and `drmModeSetPlane()`.
  - The final lesson should teach fbdev as a legacy/simple model and explain that modern display-output drivers are normally DRM/KMS, often with fbdev emulation for `/dev/fbN` compatibility.
- Backlight/power/panel context:
  - `fb_info` may reference a backlight device when configured, and `ldd2-ch03` shows backlight as its own MFD child/subsystem.
  - `ldd2-ch10` reinforces that display/video IP often participates in power domains.
  - Topic 29 should mention but not deeply teach the backlight, regulator, clock, panel, bridge, pinctrl, runtime PM, or power-domain frameworks.

## Source Differences
- Kernel era:
  - `ldd1-ch21` is older fbdev-centric material. It remains useful for fbdev basics, but modern production display drivers should usually be framed around DRM/KMS unless the hardware/use case is genuinely fbdev-only or a legacy/simple framebuffer.
  - `ldd2` and Notion V4L2 material discusses modern DMABUF/DRM handoff but not display driver internals.
- API drift:
  - `ldd1-ch21` states `unregister_framebuffer()` returns a negative errno or zero. Current local Linux 6.8 headers declare `void unregister_framebuffer(struct fb_info *fb_info)`. Learner docs should avoid claiming a return value.
  - Current local headers expose `devm_register_framebuffer(struct device *dev, struct fb_info *fb_info)`, which is not covered by the book.
  - Current local headers include helper families such as `fb_io_*`, `fb_sys_*`, `FB_DEFAULT_IOMEM_OPS`, and `FB_DEFAULT_SYSMEM_OPS`; the book mainly names `cfb_*` fallbacks.
- Terminology drift:
  - In fbdev, “framebuffer” often means `/dev/fbN` and the visible pixel memory exported by the fbdev core.
  - In DRM/KMS, a “framebuffer” is a DRM object describing scanout storage, format, pitches, offsets, and modifiers; it is not the same API as Linux fbdev even though both describe displayable buffers.
  - V4L2 “video” and DRM “display” are adjacent but distinct subsystems. V4L2 capture/output nodes should not be merged into fbdev.
- Display stack drift:
  - The book mentions X.org/KMS as more advanced graphics paths. Current documentation and kernel config show DRM/KMS is the dominant modern display stack, with `DRM_FBDEV_EMULATION` providing legacy fbdev compatibility on many systems.
  - `simplefb` and `simpledrm` may expose early firmware-provided framebuffers. Modern drivers may need aperture handoff/removal of generic firmware framebuffer drivers before taking over hardware.
- Hardware scope differences:
  - `ldd1-ch21` assumes a memory-mapped video controller in its probe sketch but notes non-MMIO displays such as SPI screens need bus-specific routines.
  - Notion SPI sources list controllers like ILI9341/ST7735, reinforcing that small embedded displays may not have a conventional linear MMIO framebuffer.
  - Notion GPIO LCD examples are display-like but not framebuffer graphics because they expose characters over GPIO, not pixels in a mapped scanout buffer.

## Gaps / Uncertainties
- Internal sources do not provide a modern DRM/KMS display-driver chapter. Topic 29 should teach modern context, but full DRM driver development may need external documentation or a future dedicated topic.
- Internal sources do not cover DRM panel/bridge/component helpers, atomic modesetting, GEM/SHMEM/DMA helpers, or plane/CRTC/encoder/connector objects in depth.
- Internal sources do not provide a buildable modern fbdev driver example for Linux 6.x using current helper macros and managed registration.
- Internal sources do not cover simpledrm/simplefb boot handoff, aperture ownership, or conflicts between firmware framebuffer and native DRM/fbdev drivers.
- Internal sources do not validate current DT YAML bindings for panels, backlights, simple-framebuffer, simpledrm, SPI panels, or display graph endpoints.
- Internal sources do not deeply cover cache coherency, write-combining, deferred I/O, shadow buffers, damage tracking, or slow bus-backed framebuffers.
- Hardware validation is required for any real display example because pixel format, stride, endianness, panel timings, backlight polarity, reset GPIO, regulators, and clocks are board-specific.

## External Validation
- Used: `https://docs.kernel.org/fb/index.html`
  - Purpose: confirm current kernel documentation still groups fbdev docs under Frame Buffer and references API, cmap, deferred I/O, fbcon, internals, and mode database topics.
- Used: `https://docs.kernel.org/fb/api.html`
  - Purpose: validate current userspace framebuffer API framing and the warning that driver behavior differs in subtle ways.
- Used: `https://docs.kernel.org/fb/framebuffer.html`
  - Purpose: validate `/dev/fb*` model, character-device major/minor framing, and read/write/seek/mmap programming model.
- Used: `https://docs.kernel.org/fb/internals.html`
  - Purpose: validate fbdev internal structure roles for `fb_fix_screeninfo`, `fb_var_screeninfo`, `fb_cmap`, `fb_info`, and driver private data.
- Used: `https://docs.kernel.org/gpu/drm-kms.html`
  - Purpose: validate modern DRM/KMS framebuffer object terminology, reference-counted DRM framebuffer lifetime, and helper context for fbdev emulation.
- Used: `https://docs.kernel.org/gpu/drm-kms-helpers.html`
  - Purpose: validate fbdev emulation helper context in DRM/KMS and deferred/shadow-buffer helper concepts.
- Used: `https://docs.kernel.org/gpu/todo.html`
  - Purpose: validate the modern recommendation that very simple fbdev drivers can often be converted by starting with a new DRM driver using simple KMS/SHMEM helpers.
- Used: `https://docs.kernel.org/driver-api/aperture.html`
  - Purpose: validate modern framebuffer aperture ownership and handoff between generic firmware framebuffer drivers and native graphics drivers.
- Local validation: Linux `6.8.0-124-generic` headers and config were inspected:
  - `/lib/modules/6.8.0-124-generic/build/include/linux/fb.h`
  - `/lib/modules/6.8.0-124-generic/build/include/uapi/linux/fb.h`
  - `/lib/modules/6.8.0-124-generic/build/drivers/video/fbdev/Kconfig`
  - `/lib/modules/6.8.0-124-generic/build/drivers/gpu/drm/Kconfig`
  - `/boot/config-6.8.0-124-generic`
  - Confirmed current `struct fb_ops`, `struct fb_info`, `register_framebuffer()`, `devm_register_framebuffer()`, `framebuffer_alloc()`, `framebuffer_release()`, helper callbacks, `CONFIG_FB`, `CONFIG_DRM`, `CONFIG_DRM_FBDEV_EMULATION`, and `CONFIG_DRM_SIMPLEDRM`.

## Learning Content Brief
- Learning path number: `29`.
- Slug: `framebuffer-and-display-basics`.
- Topic scope:
  - Framebuffer mental model, `/dev/fbN`, memory mapping, fbdev core, `fb_info`, `fb_ops`, mode information, basic probe/remove lifecycle, userspace drawing/debugging, and modern display-stack context.
  - Keep full DRM/KMS driver internals out of this topic except as comparison and production guidance.
  - Keep V4L2 capture/video pipeline internals in topics 32-34.
  - Keep DMA fundamentals in topic 21, power sequencing in topic 24, regulators/clocks in topics 22-23, and GPIO/pinctrl in topics 13-14.
- Beginner mental model:
  - “A framebuffer is a block of memory where bytes become pixels. The display controller repeatedly scans that memory and sends it to the panel.”
  - “fbdev gives userspace `/dev/fb0`; DRM/KMS is the modern display subsystem, but may emulate fbdev for old programs and console use.”
- Core mechanism:
  - fbdev core exposes character devices under `/dev/fbN`.
  - Driver allocates and fills `struct fb_info`.
  - Driver provides `struct fb_ops` callbacks.
  - Driver registers with framebuffer core.
  - Userspace queries mode info with ioctl, maps framebuffer memory with `mmap()`, computes offsets with stride and format data, then writes pixels.
  - fbcon and core helpers use `fb_ops` for console rendering, blanking, panning, and blitting.
- Important structs/APIs:
  - Kernel: `struct fb_info`, `struct fb_ops`, `struct fb_var_screeninfo`, `struct fb_fix_screeninfo`, `struct fb_cmap`, `struct vm_area_struct`.
  - Registration/lifetime: `framebuffer_alloc()`, `framebuffer_release()`, `register_framebuffer()`, `unregister_framebuffer()`, `devm_register_framebuffer()`.
  - fb operations: `.fb_check_var`, `.fb_set_par`, `.fb_setcolreg`, `.fb_setcmap`, `.fb_blank`, `.fb_pan_display`, `.fb_fillrect`, `.fb_copyarea`, `.fb_imageblit`, `.fb_mmap`, `.fb_ioctl`.
  - Helper operations: `cfb_fillrect()`, `cfb_copyarea()`, `cfb_imageblit()`, current `sys_*`/`fb_sys_*`/`fb_io_*` helper families where appropriate.
  - Userspace ABI: `FBIOGET_VSCREENINFO`, `FBIOPUT_VSCREENINFO`, `FBIOGET_FSCREENINFO`, `FBIOGETCMAP`, `FBIOPUTCMAP`, `FBIOPAN_DISPLAY`, `/sys/class/graphics/fbN/blank`.
  - Modern context: DRM/KMS, DRM framebuffer object, GEM, DMABUF/PRIME, `drmModeAddFB2()`, `drmModeSetPlane()`, `DRM_FBDEV_EMULATION`, `simpledrm`, `simplefb`.
- Lifecycle/data flow:
  - Probe: map/register hardware resources, enable clocks/regulators as needed, allocate framebuffer memory or map existing memory, allocate `fb_info`, fill mode/fix data, assign ops, register framebuffer.
  - Mode change: userspace requests mode through ioctl; core calls `.fb_check_var`; if accepted, `.fb_set_par` programs hardware.
  - Draw: userspace writes or mmaps `/dev/fbN`; fbcon/core may call blit/fill/copy helpers; display controller scans out memory.
  - Blank/unblank: sysfs/ioctl/core call `.fb_blank`; driver turns display pipeline/backlight/power/clocks on or off in correct order.
  - Remove/suspend: unregister first, stop scanout safely, release/disable resources, free `fb_info` and backing memory after userspace/core no longer owns it.
- Practical example candidates for later:
  - Learning-only userspace program that opens `/dev/fb0`, queries `FBIOGET_*SCREENINFO`, mmaps the buffer, and draws a rectangle using `line_length` and bitfields.
  - README lab using `fbset`, `cat /sys/class/graphics/fb0/*`, `dd if=/dev/fb0`, and blank/unblank commands.
  - Optional kernel skeleton only if clearly marked learning-only; a fake framebuffer module is easy to get subtly wrong and should not imply real panel timing or scanout validation.
- Common bugs:
  - Using `xres * bytes_per_pixel` instead of `fix.line_length`.
  - Ignoring `xoffset`/`yoffset` and virtual resolution.
  - Assuming RGB byte order from `bits_per_pixel` rather than checking color bitfields.
  - Mapping the wrong memory type or confusing `ioremap()` with userspace `mmap()`.
  - Programming hardware in `.fb_check_var` instead of only validating/tweaking.
  - Forgetting to unregister before freeing `fb_info` or backing memory.
  - Not handling blank/suspend/resume power sequencing with clocks/regulators/backlight.
  - Using old fbdev for new hardware when DRM/KMS simple helpers are the better production path.
  - Treating V4L2 `/dev/videoX` buffers, DRM framebuffer objects, and fbdev `/dev/fbX` as one API.
  - Testing raw framebuffer dumps on a different mode/format and expecting a valid image.
- Debugging notes:
  - Inspect `/dev/fb*`, `/sys/class/graphics/fb*/`, `dmesg`, `fbset -i` if available, and kernel config options.
  - Query `FBIOGET_VSCREENINFO` and `FBIOGET_FSCREENINFO` before drawing.
  - Verify `line_length`, bpp, visual type, offsets, and mmap size.
  - Use simple color fills to detect channel ordering, stride, and endianness errors.
  - Check whether `/dev/fb0` is native fbdev, simplefb/simpledrm, or DRM fbdev emulation before debugging the wrong driver.
  - For modern DRM systems, use `modetest`, `drm_info`, debugfs, and DRM dynamic debug in later display-specific work.
- Production concerns:
  - Prefer DRM/KMS for new display-output drivers unless maintaining legacy fbdev or extremely simple hardware with a justified reason.
  - Treat framebuffer physical addresses and direct userspace mapping as security-sensitive.
  - Avoid leaking physical addresses where current kernel config/docs warn against it.
  - Validate panel timings, pixel clock, reset/backlight GPIO polarity, regulators, clocks, pinctrl, and power domains on the actual board.
  - Consider cacheability/write-combining and slow bus-backed displays; deferred I/O or shadow buffers may be needed.
  - Keep userspace ABI stable if exposing fbdev; old programs may rely on `/dev/fb0`, ioctls, and mmap behavior.
- Interview angles:
  - Explain fbdev vs DRM/KMS vs V4L2 in one coherent picture.
  - Explain why `fb_ops` is not `file_operations`.
  - Walk through the offset calculation for pixel `(x, y)`.
  - Explain why `fix.line_length` matters.
  - Describe probe/remove ordering and why unregister must precede freeing.
  - Diagnose a distorted image caused by wrong stride or pixel format.
  - Diagnose a black screen with working `/dev/fb0`: backlight, blanking, clocks, regulators, panel reset, or scanout disabled.
  - Explain why new drivers usually choose DRM/KMS and what fbdev emulation provides.
