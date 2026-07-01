# 29 - Framebuffer And Display Basics Interview Questions

Strong candidates should be able to reason from userspace drawing all the way down to framebuffer memory, fbdev core callbacks, display controller programming, and modern DRM/KMS tradeoffs. The best answers avoid slogans like “framebuffer is just memory” and explain stride, pixel format, lifetime, and ownership.

## Beginner Questions

### 1. What is a framebuffer in Linux?

- Level: Beginner
- Question: What is a framebuffer, and why does Linux expose `/dev/fb0`?

**Short Answer**

A framebuffer is memory that represents pixels for a display. Linux fbdev exposes it through `/dev/fb0` so userspace can query display information and draw by reading, writing, or `mmap()`ing the framebuffer.

**Deep Explanation**

The simple mental model is:

```text
pixel memory -> display controller -> panel/monitor
```

Userspace does not normally program display controller registers directly. The kernel driver configures hardware, while the fbdev core exposes a standard character-device ABI. Applications can ask for screen information with framebuffer ioctls and map the framebuffer memory to draw pixels.

This model is simple and useful for diagnostics, old software, framebuffer console, and some legacy/simple embedded displays. Modern full display drivers usually use DRM/KMS, but DRM can still expose `/dev/fb0` through fbdev emulation.

**API / Code Anchor**

- `/dev/fb0`
- `FBIOGET_VSCREENINFO`
- `FBIOGET_FSCREENINFO`
- `mmap()`
- `struct fb_info`
- `struct fb_ops`

**Production or Debugging Angle**

If `/dev/fb0` exists, first identify what created it: native fbdev, `simplefb`, `simpledrm`, or DRM fbdev emulation. Debugging the wrong stack wastes time.

**Common Traps**

- Saying the framebuffer is always physically contiguous RAM.
- Assuming `/dev/fb0` means a real legacy fbdev driver owns the display.
- Confusing fbdev with DRM/KMS or V4L2.

**Follow-up Questions**

- How does userspace learn the framebuffer stride?
- Why might `/dev/fb0` exist but the screen remain black?
- What does DRM fbdev emulation provide?

### 2. Why is `fix.line_length` important?

- Level: Beginner
- Question: Why should drawing code use `fix.line_length` instead of `xres * bytes_per_pixel`?

**Short Answer**

`fix.line_length` is the real number of bytes per framebuffer row. Hardware often aligns or pads scanlines, so `xres * bytes_per_pixel` can be wrong.

**Deep Explanation**

Framebuffer memory is not guaranteed to be tightly packed. A 800-pixel row at 32 bpp might appear to need 3200 bytes, but the hardware may align each row to a larger boundary. `fix.line_length` tells userspace and drivers the actual row stride.

Correct offset calculation looks like:

```c
offset = (y + var.yoffset) * fix.line_length +
         (x + var.xoffset) * bytes_per_pixel;
```

Ignoring stride causes diagonal images, shifted rows, tearing-looking artifacts, or memory writes into the wrong part of the buffer.

**API / Code Anchor**

- `struct fb_fix_screeninfo fix`
- `fix.line_length`
- `struct fb_var_screeninfo var`
- `var.xoffset`
- `var.yoffset`

**Production or Debugging Angle**

For distorted output, print `xres`, `yres`, `bits_per_pixel`, `xoffset`, `yoffset`, and `line_length`. A stride bug is one of the first suspects when every row starts in the wrong place.

**Common Traps**

- Calculating `screensize = xres * yres * bpp / 8` for mmap length.
- Ignoring virtual resolution.
- Forgetting offsets when panning is active.

**Follow-up Questions**

- What is virtual resolution?
- How do `xoffset` and `yoffset` affect drawing?
- What symptom would you expect from a stride mismatch?

### 3. What are `fb_var_screeninfo` and `fb_fix_screeninfo`?

- Level: Beginner
- Question: Compare `struct fb_var_screeninfo` and `struct fb_fix_screeninfo`.

**Short Answer**

`fb_var_screeninfo` holds changeable mode information such as resolution, bits per pixel, offsets, timing, and color bitfields. `fb_fix_screeninfo` holds fixed or mode-derived information such as framebuffer memory size, type, visual mode, and line length.

**Deep Explanation**

Userspace needs both views:

- Variable information tells the requested/current display mode and pixel layout.
- Fixed information tells how the framebuffer memory is actually organized.

The names are a little misleading because some fixed fields can depend on the selected mode, but userspace should treat them as not directly editable through the same path.

**API / Code Anchor**

- `FBIOGET_VSCREENINFO`
- `FBIOPUT_VSCREENINFO`
- `FBIOGET_FSCREENINFO`
- `struct fb_var_screeninfo`
- `struct fb_fix_screeninfo`

**Production or Debugging Angle**

Wrong colors usually start with `fb_var_screeninfo` color bitfields. Wrong row layout usually starts with `fb_fix_screeninfo.line_length`.

**Common Traps**

- Thinking `bits_per_pixel` alone tells the color order.
- Forgetting `FBIOPUT_VSCREENINFO` may be adjusted by the driver.
- Treating physical address fields as safe to expose or rely on.

**Follow-up Questions**

- Which struct contains color channel offsets?
- Which struct contains `line_length`?
- Why should applications re-read mode info after requesting a change?

### 4. Is `fb_ops` the same thing as `file_operations`?

- Level: Beginner
- Question: Why does a framebuffer driver implement `fb_ops` instead of directly exposing normal `file_operations`?

**Short Answer**

`file_operations` is the generic character-device syscall interface. `fb_ops` is the framebuffer driver callback table used by the fbdev core. The fbdev core owns the generic file operations and dispatches to driver-specific `fb_ops`.

**Deep Explanation**

Userspace still calls `open()`, `ioctl()`, `mmap()`, `read()`, and `write()` on `/dev/fb0`. But the framebuffer driver usually does not register its own `cdev` with its own `file_operations`.

Instead:

```text
userspace syscall
  -> fbdev core file operation
  -> driver fb_ops callback
  -> hardware behavior
```

This lets the kernel provide a common framebuffer ABI while each driver handles the hardware-specific parts.

**API / Code Anchor**

- `struct fb_ops`
- `struct file_operations`
- `drivers/video/fbdev/core/fbmem.c`
- `.fb_mmap`
- `.fb_ioctl`
- `.fb_blank`

**Production or Debugging Angle**

When tracing behavior, remember there is a core layer between syscalls and the driver. A userspace ioctl may be handled entirely by the core, or it may call a driver callback.

**Common Traps**

- Writing a standalone char driver when the fbdev core already provides the ABI.
- Assuming `.fb_ioctl` handles all framebuffer ioctls.
- Forgetting fbcon can call framebuffer operations too.

**Follow-up Questions**

- When does `.fb_ioctl` get called?
- What does `.fb_mmap` customize?
- Why does this split help ABI stability?

## Mid-level Questions

### 5. Walk through framebuffer driver probe and remove.

- Level: Mid
- Question: What should a simple fbdev driver's probe and remove paths do?

**Short Answer**

Probe gets hardware resources, prepares memory and power, allocates `fb_info`, fills mode/fix data and `fb_ops`, then registers the framebuffer. Remove unregisters first, stops hardware safely, releases framebuffer memory, releases `fb_info`, and disables resources.

**Deep Explanation**

A typical probe sequence:

```text
probe()
  -> get MMIO resource / framebuffer memory / IRQ if needed
  -> enable clocks, regulators, pinctrl, reset, panel/backlight as needed
  -> allocate or map framebuffer memory
  -> framebuffer_alloc()
  -> fill info->var, info->fix, info->screen_base, info->screen_size
  -> set info->fbops
  -> store private data in info->par
  -> register_framebuffer()
```

Remove must reverse the externally visible lifetime first:

```text
remove()
  -> unregister_framebuffer()
  -> stop scanout or blank display
  -> free framebuffer backing memory
  -> framebuffer_release()
  -> disable clocks/regulators/resources
```

The exact order around stopping scanout depends on hardware, but the driver must not leave registered userspace access pointing at freed memory.

**API / Code Anchor**

- `framebuffer_alloc()`
- `register_framebuffer()`
- `unregister_framebuffer()`
- `framebuffer_release()`
- `devm_register_framebuffer()`
- `platform_get_resource()`
- `devm_ioremap_resource()`

**Production or Debugging Angle**

Unload crashes often indicate lifetime bugs: unregister too late, free too early, or leave scanout/DMA using released memory.

**Common Traps**

- Freeing `fb_info` before unregistering.
- Publishing `/dev/fb0` before `fb_info` fields are valid.
- Not unwinding failed probe paths.
- Forgetting backlight, clocks, regulators, or reset GPIOs.

**Follow-up Questions**

- What belongs in `info->par`?
- Why might `devm_register_framebuffer()` help?
- What resources must stay valid after mmap?

### 6. What is the role of `.fb_check_var` versus `.fb_set_par`?

- Level: Mid
- Question: Why are `.fb_check_var` and `.fb_set_par` separate callbacks?

**Short Answer**

`.fb_check_var` validates and adjusts a requested mode. `.fb_set_par` commits the accepted mode to hardware.

**Deep Explanation**

Userspace may request unsupported resolution, bpp, timings, or color layout. The framebuffer core asks the driver to check the request. The driver may reject it or adjust it to the nearest supported values.

Once the mode is accepted, `.fb_set_par` programs the controller registers, framebuffer base address, timing, format, and other hardware state derived from `info->var`.

Keeping these separate prevents speculative validation from accidentally reprogramming active hardware.

**API / Code Anchor**

- `struct fb_ops`
- `.fb_check_var`
- `.fb_set_par`
- `FBIOPUT_VSCREENINFO`
- `struct fb_var_screeninfo`

**Production or Debugging Angle**

If a mode request changes the screen before the ioctl should commit, suspect side effects in `.fb_check_var`. If the ioctl succeeds but hardware does not change, inspect `.fb_set_par`.

**Common Traps**

- Programming registers in `.fb_check_var`.
- Failing to update color bitfields for the selected bpp.
- Accepting modes the hardware cannot scan out.
- Not rechecking the final values returned to userspace.

**Follow-up Questions**

- What fields might `.fb_check_var` adjust?
- How should unsupported bpp be handled?
- Why should userspace inspect the returned mode after `FBIOPUT_VSCREENINFO`?

### 7. How does userspace draw a pixel correctly?

- Level: Mid
- Question: Show the correct data needed to draw pixel `(x, y)` into an mmaped framebuffer.

**Short Answer**

Userspace needs variable screen info, fixed screen info, the mmaped base pointer, stride, offsets, bytes per pixel, and color bitfields.

**Deep Explanation**

For a simple packed 32 bpp framebuffer:

```c
off = (y + var.yoffset) * fix.line_length +
      (x + var.xoffset) * 4;
*(uint32_t *)(fb + off) = packed_color;
```

But production code should not assume all 32 bpp layouts are the same. It should pack color according to:

- `var.red.offset`, `var.red.length`
- `var.green.offset`, `var.green.length`
- `var.blue.offset`, `var.blue.length`
- `var.transp.offset`, `var.transp.length`

For 16 bpp, 24 bpp, big-endian framebuffers, or non-packed visuals, the calculation changes.

**API / Code Anchor**

- `FBIOGET_VSCREENINFO`
- `FBIOGET_FSCREENINFO`
- `mmap()`
- `fix.line_length`
- `var.bits_per_pixel`
- `var.red/green/blue/transp`

**Production or Debugging Angle**

Use solid color fills to debug channel order. If red appears blue, your color packing is wrong. If rows are angled, your stride is wrong.

**Common Traps**

- Assuming RGB888 or XRGB8888 without checking bitfields.
- Ignoring virtual offsets.
- Writing past the mapped length.
- Treating raw framebuffer dumps as portable images.

**Follow-up Questions**

- Why is 24 bpp awkward?
- How do you calculate mmap length?
- What fields would you print when debugging wrong colors?

### 8. How does framebuffer blanking work?

- Level: Mid
- Question: What should `.fb_blank` do, and why can blanking bugs look like a dead display?

**Short Answer**

`.fb_blank` handles display blanking and unblanking requests. It may disable or enable scanout, clocks, panel power, and backlight depending on hardware.

**Deep Explanation**

Blanking is not merely “paint black pixels.” It is often a power/display-state transition:

- `FB_BLANK_UNBLANK`: restore display output.
- `FB_BLANK_NORMAL`: blank screen.
- `FB_BLANK_VSYNC_SUSPEND`: suspend vertical sync.
- `FB_BLANK_HSYNC_SUSPEND`: suspend horizontal sync.
- `FB_BLANK_POWERDOWN`: power down display path.

The right implementation is hardware-specific. Some controllers need scanout stopped before clocks are disabled. Some panels need reset, regulator, and backlight sequencing.

**API / Code Anchor**

- `.fb_blank`
- `FB_BLANK_UNBLANK`
- `FB_BLANK_NORMAL`
- `FB_BLANK_POWERDOWN`
- `/sys/class/graphics/fb0/blank`

**Production or Debugging Angle**

If `/dev/fb0` works but the screen is black, check blank state and backlight before rewriting the whole driver. A powered-off backlight can look like a dead framebuffer.

**Common Traps**

- Turning off clocks while scanout is active.
- Enabling backlight before valid video reaches the panel.
- Treating blank/unblank as unrelated to suspend/resume.
- Forgetting the framebuffer console can trigger blanking.

**Follow-up Questions**

- How would you test blank/unblank?
- What is the difference between blanking and writing black pixels?
- Which subsystems may be involved besides fbdev?

### 9. How does fbdev relate to `mmap()` and MMIO?

- Level: Mid
- Question: Explain the difference between `ioremap()` and userspace `mmap()` in framebuffer drivers.

**Short Answer**

`ioremap()` maps device physical addresses into kernel virtual address space. Userspace `mmap()` maps framebuffer memory into a process VMA through the kernel's mmap path.

**Deep Explanation**

Kernel code cannot safely dereference arbitrary physical addresses. It uses `ioremap()` or managed resource helpers to access MMIO. Userspace cannot directly access device memory either; it asks the kernel to map an approved range into its virtual address space using `mmap()`.

For framebuffers:

- The driver may use `ioremap()` for controller registers.
- The fbdev core or driver may map framebuffer memory to userspace.
- I/O-memory-backed framebuffers and system-memory-backed framebuffers can require different helpers/protection attributes.

**API / Code Anchor**

- `ioremap()`
- `devm_ioremap_resource()`
- `mmap()`
- `struct vm_area_struct`
- `remap_pfn_range()`
- `io_remap_pfn_range()`
- `.fb_mmap`

**Production or Debugging Angle**

Cacheability and memory attributes matter. Wrong mapping attributes can cause slow drawing, stale pixels, or CPU/display coherency problems.

**Common Traps**

- Saying `ioremap()` gives userspace access.
- Mapping register MMIO when userspace should map framebuffer memory.
- Using normal cached mapping for memory that hardware scans out without coherency.

**Follow-up Questions**

- When would `.fb_mmap` be needed?
- What is a VMA?
- Why can write-combining matter for framebuffer performance?

## Senior Questions

### 10. Should you write a new fbdev driver for new hardware?

- Level: Senior
- Question: A team wants a new display-output driver. Should they implement fbdev directly?

**Short Answer**

Usually no. New display-output drivers should normally use DRM/KMS. Direct fbdev is mainly for legacy maintenance, constrained simple hardware, diagnostics, or cases where DRM/KMS is genuinely not appropriate.

**Deep Explanation**

fbdev is simple, but it lacks the modern display model:

- No proper atomic modesetting model.
- No rich connector/encoder/CRTC/plane abstraction.
- Weak fit for multi-display and composition.
- Limited buffer-sharing story compared with GEM/DMABUF/PRIME.
- Modern userspace display stacks expect DRM/KMS.

DRM/KMS can still provide compatibility through fbdev emulation, so old `/dev/fb0` programs and fbcon may continue to work.

**API / Code Anchor**

- fbdev: `struct fb_info`, `struct fb_ops`, `/dev/fb0`
- DRM/KMS: DRM framebuffer object, planes, CRTCs, connectors, GEM, DMABUF
- `DRM_FBDEV_EMULATION`
- `simpledrm`

**Production or Debugging Angle**

Architecture choice affects maintainability. A quick fbdev driver may bring up pixels, but it can paint the team into a corner when they need hotplug, multiple planes, page flips, zero-copy video, or Wayland/DRM userspace.

**Common Traps**

- Choosing fbdev because the first test is “draw a rectangle.”
- Assuming DRM is only for GPUs.
- Forgetting small SPI displays can also use DRM tiny/simple helpers.
- Treating fbdev emulation as proof the native driver is fbdev.

**Follow-up Questions**

- When might fbdev still be acceptable?
- What does DRM fbdev emulation do?
- How does DMABUF change camera-to-display pipelines?

### 11. How do you debug a black screen when `/dev/fb0` exists?

- Level: Senior
- Question: The framebuffer device exists, ioctls succeed, and writes to mmaped memory do not show anything. How do you debug it?

**Short Answer**

First prove which driver owns `/dev/fb0`, then check blanking, backlight, power, clocks, reset, panel timing, scanout base address, pixel format, and whether another display stack owns the hardware.

**Deep Explanation**

Existence of `/dev/fb0` only proves that a framebuffer device is registered. It does not prove:

- The panel is powered.
- The backlight is enabled.
- The controller is scanning out.
- The scanout address points to the memory userspace writes.
- The pixel format matches.
- The visible output is routed to the connector/panel.

A structured flow:

```text
identify fb0 provider
  -> native fbdev, simplefb, simpledrm, DRM emulation?
check blank state
check dmesg for display/panel/backlight/regulator/clock errors
write test pattern
verify stride and bpp
check scanout/panel enable sequence
check suspend/runtime PM state
```

**API / Code Anchor**

- `/sys/class/graphics/fb0/name`
- `/sys/class/graphics/fb0/blank`
- `FBIOGET_VSCREENINFO`
- `FBIOGET_FSCREENINFO`
- `.fb_blank`
- `.fb_set_par`
- `dmesg`
- DRM tools such as `modetest` when applicable

**Production or Debugging Angle**

Black screen bugs often live outside the framebuffer memory path. Backlight and power sequencing are classic culprits.

**Common Traps**

- Rewriting drawing code before checking backlight.
- Debugging fbdev when DRM owns the display.
- Ignoring boot firmware framebuffer handoff.
- Assuming successful mmap means scanout is active.

**Follow-up Questions**

- What would you inspect in sysfs?
- How do you distinguish wrong pixels from no scanout?
- What role can `simpledrm` play during boot?

### 12. What lifetime risks exist with mmaped framebuffer memory?

- Level: Senior
- Question: What can go wrong if userspace has mmaped `/dev/fb0` while the driver is removed or the display mode changes?

**Short Answer**

Userspace may retain a mapping to memory that the driver wants to free, reuse, resize, or stop scanning out. The driver/core must manage registration and memory lifetime so mappings do not become use-after-free or corrupt unrelated memory.

**Deep Explanation**

Framebuffer memory has multiple possible users:

- Userspace mappings.
- fbcon.
- fbdev core read/write paths.
- Hardware scanout or DMA.
- Driver suspend/resume paths.

If remove frees backing memory before unregistering and quiescing access, userspace can still write to stale memory. If mode changes shrink or move the buffer without handling mmap users carefully, applications may write beyond the new valid range.

Some drivers avoid complex dynamic resizing. Others rely on core helpers, reference handling, or modesetting rules to avoid unsafe changes while mappings exist.

**API / Code Anchor**

- `mmap()`
- `.fb_mmap`
- `unregister_framebuffer()`
- `framebuffer_release()`
- `struct vm_area_struct`
- `info->screen_base`
- `info->screen_size`

**Production or Debugging Angle**

Crashes on module unload, memory corruption after display unplug, or random scribbles after mode changes are often lifetime bugs.

**Common Traps**

- Thinking `munmap()` is guaranteed before driver remove.
- Freeing framebuffer memory before unregistering.
- Forgetting hardware can still scan out released memory.
- Ignoring fbcon as another user.

**Follow-up Questions**

- What order should remove use?
- How would you design mode changes safely?
- What would you look for in a KASAN report?

### 13. How do you compare fbdev framebuffer and DRM framebuffer object?

- Level: Senior
- Question: Both fbdev and DRM use the word framebuffer. What is the difference?

**Short Answer**

An fbdev framebuffer is a legacy character-device view of display memory exposed as `/dev/fbN`. A DRM framebuffer is a KMS object that describes scanout storage, pixel format, pitches, offsets, modifiers, and lifetime inside the DRM display pipeline.

**Deep Explanation**

fbdev is centered around one memory-like device node. Userspace generally maps and writes pixels directly.

DRM/KMS models the display pipeline:

- Framebuffer object describes buffer layout.
- Plane scans out a framebuffer.
- CRTC generates timing.
- Encoder/connector route output.
- GEM/DMABUF manage/share backing memory.

DRM can expose an fbdev-compatible `/dev/fb0` using fbdev emulation, but that compatibility layer does not make the native driver a legacy fbdev driver.

**API / Code Anchor**

- fbdev: `/dev/fb0`, `struct fb_info`, `struct fb_ops`
- DRM: `drmModeAddFB2()`, `drmModeSetPlane()`, DRM framebuffer object, GEM, DMABUF/PRIME
- `DRM_FBDEV_EMULATION`

**Production or Debugging Angle**

For camera-to-display zero-copy, V4L2 buffers are often exported as DMABUF and imported into DRM. That is not the same workflow as copying pixels into `/dev/fb0`.

**Common Traps**

- Treating DRM framebuffer ID as a pointer to `/dev/fb0` memory.
- Assuming fbdev supports modern plane composition.
- Debugging DRM modesetting problems with only fbdev ioctls.

**Follow-up Questions**

- What is a DRM plane?
- Why does DMABUF matter?
- What does fbdev emulation sacrifice compared with native DRM usage?

### 14. Debugging scenario: distorted image with correct colors

- Level: Senior
- Question: A test app draws a blue rectangle, but the rectangle appears slanted and each row starts slightly farther to the right. What is the likely bug?

**Short Answer**

The app probably calculates row offsets with the wrong stride, usually `xres * bytes_per_pixel` instead of `fix.line_length`.

**Deep Explanation**

A slanted image often means each row starts at the wrong memory address. The color is correct, so pixel packing may be fine. The row stepping is wrong.

Correct formula:

```c
off = (y + var.yoffset) * fix.line_length +
      (x + var.xoffset) * bytes_per_pixel;
```

If the hardware aligns rows to a larger boundary, using visible width will under-step each row. That accumulates row by row and creates a diagonal distortion.

**API / Code Anchor**

- `FBIOGET_FSCREENINFO`
- `fix.line_length`
- `FBIOGET_VSCREENINFO`
- `var.xres`
- `var.bits_per_pixel`

**Production or Debugging Angle**

Have the test program print the mode values and draw vertical/horizontal lines. Stride bugs are easier to see with grid patterns than with photos.

**Common Traps**

- Assuming stride equals visible width.
- Ignoring virtual offsets.
- Mapping only visible size, not virtual stride times virtual height.

**Follow-up Questions**

- What symptom suggests wrong color bitfields instead?
- How would virtual resolution affect this?
- How do you calculate a safe mmap length?

### 15. Debugging scenario: mode change ioctl succeeds but nothing changes

- Level: Senior
- Question: Userspace calls `FBIOPUT_VSCREENINFO`, the ioctl succeeds, but the controller stays in the old mode. Where do you look?

**Short Answer**

Inspect `.fb_check_var`, `.fb_set_par`, returned `fb_var_screeninfo`, and whether the driver actually supports changing that mode at runtime.

**Deep Explanation**

The ioctl may succeed even if the driver adjusts the requested mode. Userspace must check the returned values. If the returned values show the new mode but hardware does not change, suspect `.fb_set_par` or missing hardware programming.

Possible causes:

- `.fb_check_var` adjusts request back to old mode.
- `.fb_set_par` is missing or incomplete.
- Required clocks/timings are not reprogrammed.
- Panel cannot support requested mode.
- Runtime PM leaves hardware suspended.
- DRM/fbdev emulation layer abstracts away actual KMS mode control.

**API / Code Anchor**

- `FBIOPUT_VSCREENINFO`
- `FBIOGET_VSCREENINFO`
- `.fb_check_var`
- `.fb_set_par`
- `struct fb_var_screeninfo`

**Production or Debugging Angle**

Dynamic mode changes are hardware-sensitive. Some simple fbdev drivers support only a fixed boot mode and should reject or clamp unsupported modes clearly.

**Common Traps**

- Not checking returned mode values.
- Changing `info->var` without programming hardware.
- Programming hardware in `.fb_check_var`.
- Ignoring panel timing limits.

**Follow-up Questions**

- How should unsupported modes be reported?
- Why might fixed-mode embedded panels reject mode changes?
- What logs would you add?

## Common Trap Review

- `fb_ops` is not `file_operations`.
- `bits_per_pixel` is not enough to know color order.
- `fix.line_length` is the real row stride.
- `/dev/fb0` does not prove native fbdev owns the hardware.
- A black screen can be a backlight, blanking, power, reset, or routing problem.
- `ioremap()` is for kernel virtual access; userspace uses `mmap()`.
- `unregister_framebuffer()` should happen before freeing `fb_info` or backing memory.
- New display drivers usually need a DRM/KMS discussion before choosing fbdev.
