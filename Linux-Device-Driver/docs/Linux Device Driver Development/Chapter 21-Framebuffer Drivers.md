```bash
# Chapter 21 - Framebuffer Drivers
```
Video cards always have a certain amount of RAM. This RAM is where the bitmap of image data is buffered for display. From the software point of view, the framebuffer is a character device providing access to this RAM.
That said, a framebuffer driver provides an interface for the following:
The display mode setting
Memory access to the video buffer
```c
Basic 2D acceleration operations (for example, scrolling)
```
To provide this interface, the framebuffer driver generally talks to the hardware directly.
There are well-known framebuffer drivers, such as:
intelfb: A framebuffer for various Intel 8xx/9xx compatible graphic devices vesafb: A framebuffer driver that uses the VESA standard interface to talk to the video hardware mxcfb: A framebuffer driver for the i.MX6 chip series
Framebuffer drivers are the simplest form of graphics drivers under Linux. They should not be confused with X.org drivers, which implement advanced features, such as 3D
acceleration, or Kernel mode setting (KMS) drivers, which expose both framebuffer and
GPU functionalities (like X.org drivers do).
The i.MX6 X.org driver is a closed source and is called vivante.
Back to our framebuffer drivers, they are very simple API drivers that expose video card functionalities by means of character devices, accessible from the user space through
/dev/fbX entries. You can find more information on the Linux graphical stack in the comprehensive talk Linux graphics demystified by Martin Fiedler:
http://keyj.emphy.de/files/linuxgraphics_en.pdf.
In this chapter, we will cover the following topics:
Framebuffer driver data structures and methods, thus covering the whole driver architecture
Framebuffer device operations, accelerated and non-accelerated
Accessing a framebuffer from the user space
## Driver data structures
Framebuffer drivers depend heavily on four data structures, all defined in include/linux/fb.h, which is also the header you should include in your code in order to deal with framebuffer drivers:
```c
#include <linux/fb.h>
```
These structures are fb_var_screeninfo, fb_fix_screeninfo, fb_cmap, and fb_info.
The first three are made available to and from the user space code. Now let's describe the purpose of each structure, their meaning, and what they are used for:
1. The kernel uses an instance of struct fb_var_screeninfo to hold variable properties of the video card. These values are those defined by the user, such as resolution depth:
```c
struct fb_var_screeninfo {
```
__u32 xres; /* visible resolution */
__u32 yres;
__u32 xres_virtual; /* virtual resolution */
__u32 yres_virtual;
__u32 xoffset; /* offset from virtual to visible resolution */
__u32 yoffset;
__u32 bits_per_pixel; /* # of bits needed to hold a pixel */
[...]
/* Timing: All values in pixclocks, except pixclock (of course)
*/
__u32 pixclock; /* pixel clock in ps (pico seconds) */
__u32 left_margin; /* time from sync to picture */
__u32 right_margin; /* time from picture to sync */
__u32 upper_margin; /* time from sync to picture */
__u32 lower_margin;
__u32 hsync_len; /* length of horizontal sync */
__u32 vsync_len; /* length of vertical sync */
__u32 rotate; /* angle we rotate counter clockwise */
```c
};
```
This can be summarized in the following diagram:
2. There are properties of the video card that are fixed, either by the manufacturer,
or applied when a mode is set, and can't be changed. This is generally hardware information. A good example of this is the start of the framebuffer memory,
which cannot change, even by the user program. The kernel holds such information in an instance of the struct fb_fix_screeninfo structure:
```c
struct fb_fix_screeninfo {
```
char id[16]; /* identification string example "TT Builtin"
*/
```c
unsigned long smem_start; /* Start of frame buffer mem */
```
/* (physical address) */
__u32 smem_len;/* Length of frame buffer mem */
__u32 type; /* see FB_TYPE_* */
__u32 type_aux; /* Interleave for interleaved Planes */
__u32 visual; /* see FB_VISUAL_* */
__u16 xpanstep; /* zero if no hardware panning */
__u16 ypanstep; /* zero if no hardware panning */
__u16 ywrapstep; /* zero if no hardware ywrap */
__u32 line_length; /* length of a line in bytes */
```c
unsigned long mmio_start; /* Start of Memory Mapped I/O
```
*(physical address)
*/
__u32 mmio_len; /* Length of Memory Mapped I/O */
__u32 accel; /* Indicate to driver which */
/* specific chip/card we have */
__u16 capabilities; /* see FB_CAP_* */
```c
};
```
3. The struct fb_cmap structure specifies the color map, which is used to store the user's definition of colors in a manner the kernel can understand, in order to send it to the underlying video hardware. You can use this structure to define the
RGB ratio that you desire for different colors:
```c
struct fb_cmap {
```
__u32 start; /* First entry */
__u32 len; /* Number of entries */
__u16 *red; /* Red values */
__u16 *green; /* Green values */
__u16 *blue; /* Blue values */
__u16 *transp; /* Transparency. Discussed later on */
```c
};
```
4. The struct fb_info structure, which represents the framebuffer itself, is the main data structure of framebuffer drivers. Unlike the other preceding structures we've discussed, fb_info exists only in the kernel, and is not part of the user space framebuffer API:
```c
struct fb_info {
```
[...]
```c
struct fb_var_screeninfo var; /* Variable screen information.
```
Discussed earlier. */
```c
struct fb_fix_screeninfo fix; /* Fixed screen information. */
struct fb_cmap cmap; /* Color map. */
struct fb_ops *fbops; /* Driver operations.*/
```
char __iomem *screen_base; /* Frame buffer's virtual address */
```c
unsigned long screen_size; /* Frame buffer's size */
```
[...]
```c
struct device *device; /* This is the parent */
struct device *dev; /* This is this fb device */
```
#ifdef CONFIG_FB_BACKLIGHT
/* assigned backlight device */
/* set before framebuffer registration,
remove after unregister */
```c
struct backlight_device *bl_dev;
```
/* Backlight level curve */
```c
struct mutex bl_curve_mutex;
```
u8 bl_curve[FB_BACKLIGHT_LEVELS];
#endif
[...]
```c
void *par; /* Pointer to private memory */
};
```
The struct fb_info structure should always be allocated dynamically, using framebuffer_alloc(), which is a kernel (framebuffer core) helper function to allocate memory for framebuffer devices, along with their private data memory:
```c
struct fb_info *framebuffer_alloc(size_t size, struct device *dev)
```
In this prototype, size represents the size of the private area as an argument and appends that to the end of the allocated fb_info. This private area can be referenced using the .par pointer in the fb_info structure. Now, framebuffer_release() does the reverse operation:
```c
void framebuffer_release(struct fb_info *info)
```
Once set up, a framebuffer should be registered with the kernel using register_framebuffer(), which returns a negative errno on error, or zero for success:
```c
int register_framebuffer(struct fb_info *fb_info)
```
Once registered, you can unregister the framebuffer with the unregister_framebuffer() function, which also returns a negative errno on error, or zero for success:
```c
int unregister_framebuffer(struct fb_info *fb_info)
```
Allocation and registering should be done during device probing, whereas unregistering and deallocation (release) should be done from within the driver's remove() function.
## Device methods
In the struct fb_info structure, there is a .fbops field, which is an instance of the struct fb_ops structure. This structure contains a collection of functions that are needed to perform some operations on the framebuffer device. These are entry points for the fbdev and fbcon tools. Some methods in that structure are mandatory, the minimum required for a framebuffer to work, whereas others are optional, and depend on the features the driver needs to expose, assuming the device itself supports those features.
The following is the definition of the struct fb_ops structure:
```c
struct fb_ops {
```
/* open/release and usage marking */
```c
struct module *owner;
int (*fb_open)(struct fb_info *info, int user);
int (*fb_release)(struct fb_info *info, int user);
```
/* For framebuffers with strange nonlinear layouts or that do not
* work with normal memory mapped access
*/
ssize_t (*fb_read)(struct fb_info *info, char __user *buf,
size_t count, loff_t *ppos);
ssize_t (*fb_write)(struct fb_info *info, const char __user *buf,
size_t count, loff_t *ppos);
/* checks var and eventually tweaks it to something supported,
* DO NOT MODIFY PAR */
```c
int (*fb_check_var)(struct fb_var_screeninfo *var, struct fb_info
```
*info);
```c
/* set the video mode according to info->var */
int (*fb_set_par)(struct fb_info *info);
```
/* set color register */
```c
int (*fb_setcolreg)(unsigned regno, unsigned red, unsigned green,
unsigned blue, unsigned transp, struct fb_info *info);
```
/* set color registers in batch */
```c
int (*fb_setcmap)(struct fb_cmap *cmap, struct fb_info *info);
```
/* blank display */
```c
int (*fb_blank)(int blank_mode, struct fb_info *info);
```
/* pan display */
```c
int (*fb_pan_display)(struct fb_var_screeninfo *var, struct fb_info
```
*info);
/* Draws a rectangle */
```c
void (*fb_fillrect) (struct fb_info *info, const struct fb_fillrect
```
*rect);
/* Copy data from area to another */
```c
void (*fb_copyarea) (struct fb_info *info, const struct fb_copyarea
```
*region);
/* Draws a image to the display */
```c
void (*fb_imageblit) (struct fb_info *info, const struct fb_image
```
*image);
/* Draws cursor */
```c
int (*fb_cursor) (struct fb_info *info, struct fb_cursor *cursor);
```
/* wait for blit idle, optional */
```c
int (*fb_sync)(struct fb_info *info);
```
/* perform fb specific ioctl (optional) */
```c
int (*fb_ioctl)(struct fb_info *info, unsigned int cmd,
unsigned long arg);
```
/* Handle 32bit compat ioctl (optional) */
```c
int (*fb_compat_ioctl)(struct fb_info *info, unsigned cmd,
unsigned long arg);
```
/* perform fb specific mmap */
```c
int (*fb_mmap)(struct fb_info *info, struct vm_area_struct *vma);
```
/* get capability given var */
```c
void (*fb_get_caps)(struct fb_info *info, struct fb_blit_caps *caps,
struct fb_var_screeninfo *var);
```
/* teardown any resources to do with this framebuffer */
```c
void (*fb_destroy)(struct fb_info *info);
```
[...]
```c
};
```
Different callbacks can be set depending on what functionality you wish to implement.
In Chapter 4, Character Device Drivers, we learned that character devices, by means of the struct file_operations structure, can export a collection of file operations, which are entry points for file-related system calls, such as open(), close(), read(), write(),
```c
mmap(), and ioctl().
```
That being said, do not confuse fb_ops with the file_operations structure. fb_ops offers an abstraction of low-level operations, while file_operations is for an upper-level system call interface. The kernel implements framebuffer file operations in drivers/video/fbdev/core/fbmem.c, which internally calls the methods we defined in fb_ops. In this manner, you can implement the low-level hardware operations according to the needs of the system call interface; namely, the file_operations structure. For example, when the user opens the device, the core's open file operation method will perform some core operations, and execute the fb_ops.fb_open() method if set; the same applies forrelease, mmap, and others.
Framebuffer devices support some ioctl commands defined in include/uapi/linux/fb.h, that user programs can use to operate on hardware. These commands are all handled by the core's fops.ioctl method. For some of those commands, the core's ioctl method may internally execute methods defined in the fb_ops structure.
You may wonder what the fb_ops.fb_ioctl is used for. The framebuffer core executes fb_ops.fb_ioctl only when the given ioctl command is not known to the kernel. In other words, fb_ops.fb_ioctl is executed in the default statement of the framebuffer core's fops.ioctl method.
## Driver methods
Drivers methods consist of probe() and remove() functions. Prior to going further with these method descriptions, let us set up our fb_ops structure:
```c
static struct fb_ops myfb_ops = {
```
.owner = THIS_MODULE,
.fb_check_var = myfb_check_var,
.fb_set_par = myfb_set_par,
.fb_setcolreg = myfb_setcolreg,
.fb_fillrect = cfb_fillrect, /* Those three hooks are */
.fb_copyarea = cfb_copyarea, /* non accelerated and */
.fb_imageblit = cfb_imageblit, /* are provided by kernel */
.fb_blank = myfb_blank,
```c
};
```
probe: The driver probe function is in charge of initializing the hardware,
```c
creating the struct fb_info structure using the framebuffer_alloc()
```
function, and using register_framebuffer() on it. The following sample assumes the device is memory mapped. Therefore, your non-memory map can exist, such as screen sitting on SPI buses. In this case, bus-specific routines should be used:
```c
static int myfb_probe(struct platform_device *pdev)
{
struct fb_info *info;
struct resource *res;
```
[...]
```c
dev_info(&pdev->dev, "My framebuffer driver\n");
```
/*
* Query resource, like DMA channels, I/O memory,
* regulators, and so on.
*/
res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
```c
if (!res)
return -ENODEV;
```
/* use request_mem_region(), ioremap() and so on */
[...]
```c
pwr = regulator_get(&pdev->dev, "lcd");
```
info = framebuffer_alloc(sizeof(
```c
struct my_private_struct), &pdev->dev);
if (!info)
return -ENOMEM;
```
/* Device init and default info value*/
[...]
```c
info->fbops = &myfb_ops;
```
/* Clock setup, using devm_clk_get() and so on */
[...]
/* DMA setup using dma_alloc_coherent() and so on*/
[...]
/* Register with the kernel */
ret = register_framebuffer(info);
```c
hardware_enable_controller(my_private_struct);
return 0;
}
```
remove: The remove() function should release whatever was acquired in probe(), and call the following:
```c
static int myfb_remove(struct platform_device *pdev)
{
```
/* iounmap() memory and release_mem_region() */
[...]
/* Reverse DMA, dma_free_*();*/
[...]
```c
hardware_disable_controller(fbi);
```
/* first unregister, */
```c
unregister_framebuffer(info);
```
/* and then free the memory */
```c
framebuffer_release(info);
return 0;
}
```
Assuming you used the manager version for resource allocation, you'll just need to use unregister_framebuffer() and framebuffer_release().
Everything else will be done by the kernel.
## Detailed fb_ops
Let us describe some of the hooks declared in fb_ops structure. That being said, for an idea on writing framebuffer drivers, you can have a look at drivers/video/fbdev/vfb.c,
which is a simple virtual framebuffer driver in the kernel. You can also have a look at other specific framebuffer drivers, such as the i.MX6 driver, at drivers/video/fbdev/imxfb.c, or at the kernel documentation about framebuffer driver APIs at Documentation/fb/api.txt.
## Checking information
```c
The fb_ops->fb_check_var hook is responsible for checking framebuffer parameters. Its prototype is as follows:
int (*fb_check_var)(struct fb_var_screeninfo *var,
struct fb_info *info);
```
This function should check framebuffer variable parameters and adjust to valid values. var represents the framebuffer variable parameters, which should be checked and adjusted:
```c
static int myfb_check_var(struct fb_var_screeninfo *var,
struct fb_info *info)
{
if (var->xres_virtual < var->xres)
var->xres_virtual = var->xres;
if (var->yres_virtual < var->yres)
var->yres_virtual = var->yres;
if ((var->bits_per_pixel != 32) &&
(var->bits_per_pixel != 24) &&
(var->bits_per_pixel != 16) &&
(var->bits_per_pixel != 12) &&
(var->bits_per_pixel != 8))
var->bits_per_pixel = 16;
switch (var->bits_per_pixel) {
```
case 8:
/* Adjust red*/
```c
var->red.length = 3;
var->red.offset = 5;
var->red.msb_right = 0;
```
/*adjust green*/
```c
var->green.length = 3;
var->green.offset = 2;
var->green.msb_right = 0;
```
/* adjust blue */
```c
var->blue.length = 2;
var->blue.offset = 0;
var->blue.msb_right = 0;
```
/* Adjust transparency */
```c
var->transp.length = 0;
var->transp.offset = 0;
var->transp.msb_right = 0;
```
break;
case 16:
[...]
break;
case 24:
[...]
break;
case 32:
```c
var->red.length = 8;
var->red.offset = 16;
var->red.msb_right = 0;
var->green.length = 8;
var->green.offset = 8;
var->green.msb_right = 0;
var->blue.length = 8;
var->blue.offset = 0;
var->blue.msb_right = 0;
var->transp.length = 8;
var->transp.offset = 24;
var->transp.msb_right = 0;
```
break;
```c
}
```
/*
* Any other field in *var* can be adjusted
```c
* like var->xres, var->yres, var->bits_per_pixel,
* var->pixclock and so on.
```
*/
```c
return 0;
}
```
The preceding code adjusts variable framebuffer properties according to the configuration chosen by the user.
## Setting the controller's parameters
```c
The fp_ops->fb_set_par hook is another hardware-specific hook, responsible for sending parameters to the hardware. It programs the hardware based on user settings
(info->var):
static int myfb_set_par(struct fb_info *info)
{
struct fb_var_screeninfo *var = &info->var;
```
/* Make some compute or other sanity check */
[...]
/*
* This function writes value to the hardware,
* in the appropriate registers
*/
```c
set_controller_vars(var, info);
return 0;
}
```
## Screen blanking
```c
The fb_ops->fb_blank hook is a hardware-specific hook, responsible for screen blanking.
```
Its prototype is as follows:
```c
int (*fb_blank)(int blank_mode, struct fb_info *info)
```
The blank_mode parameter is always one of the following values:
```c
enum {
```
/* screen: unblanked, hsync: on, vsync: on */
FB_BLANK_UNBLANK = VESA_NO_BLANKING,
/* screen: blanked, hsync: on, vsync: on */
FB_BLANK_NORMAL = VESA_NO_BLANKING + 1,
/* screen: blanked, hsync: on, vsync: off */
FB_BLANK_VSYNC_SUSPEND = VESA_VSYNC_SUSPEND + 1,
/* screen: blanked, hsync: off, vsync: on */
FB_BLANK_HSYNC_SUSPEND = VESA_HSYNC_SUSPEND + 1,
/* screen: blanked, hsync: off, vsync: off */
FB_BLANK_POWERDOWN = VESA_POWERDOWN + 1
```c
};
```
The usual way of doing a blank display is to do a switch case on the blank_mode parameter, as follows:
```c
static int myfb_blank(int blank_mode, struct fb_info *info)
{
pr_debug("fb_blank: blank=%d\n", blank);
switch (blank) {
```
case FB_BLANK_POWERDOWN:
case FB_BLANK_VSYNC_SUSPEND:
case FB_BLANK_HSYNC_SUSPEND:
case FB_BLANK_NORMAL:
```c
myfb_disable_controller(fbi);
```
break;
case FB_BLANK_UNBLANK:
```c
myfb_enable_controller(fbi);
```
break;
```c
}
return 0;
}
```
The blanking operation should disable the controller, stop its clocks, and power it down.
Unblanking should perform the reverse operations.
## Accelerated methods
User video operations, such as blending, stretching, moving bitmaps, or dynamic gradient generation, are all heavy-duty tasks. They require graphics acceleration to obtain acceptable performance. You can implement framebuffer-accelerated methods using the following fields of the struct fp_ops structure:
.fb_imageblit(): This method draws an image on the display and is very useful
.fb_copyarea(): This method copies a rectangular area from one screen region to another
.fb_fillrect(): This method fills in an optimized manner a rectangle with pixel lines
Therefore, kernel developers devised controllers that did not have hardware acceleration,
and provided a software-optimized method. This makes acceleration implementation optional, since a software fallback exists. That said, if the framebuffer controller does not provide any acceleration mechanism, you must populate these methods using the kernel generic routines.
These routines are as follows:
```c
cfb_imageblit(): This is a kernel-provided fallback for imageblit. The kernel uses it to output a logo to the screen during start up.
cfb_copyarea(): This is for area copy operations.
cfb_fillrect(): This is the framebuffer core non-accelerated method for achieving operations of the same name.
```
## Putting it all together
In this section, let us summarize the things discussed in the preceding section. In order to write framebuffer driver, you have to do the following:
1. Fill a struct fb_var_screeninfo structure in order to provide information on framebuffer variable properties. Those properties can be changed by user space.
2. Fill a struct fb_fix_screeninfo structure, to provide fixed parameters.
3. Set up a struct fb_ops structure, providing necessary callback functions,
which will used by the framebuffer subsystem in response to user actions.
4. Still in the struct fb_ops structure, you have to provide accelerated functions callback, if supported by the device.
5. Set up a struct fb_info structure, feeding it with structures filled in the previous steps, and call register_framebuffer() on it in order to have it registered with the kernel.
For information about writing a simple framebuffer driver, you can have a look at drivers/video/fbdev/vfb.c, which is a virtual framebuffer driver in the kernel. You can enable this in the kernel by means of the CONGIF_FB_VIRTUAL option.
## Framebuffer from user space
You usually access framebuffer memory by means of the mmap() command in order to map the framebuffer memory to the part of the system RAM, so that drawing pixels on the screen becomes a simple matter of affecting memory value. Screen parameters (variable and fixed) are extracted by means of ioctl commands, especially FBIOGET_VSCREENINFO and
FBIOGET_FSCREENINFO. The complete list is available at include/uapi/linux/fb.h in the kernel source.
The following is a sample code to draw a 300*300 square on the framebuffer:
```c
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#define FBCTL(_fd, _cmd, _arg) \
```
if(ioctl(_fd, _cmd, _arg) == -1) { \
ERROR("ioctl failed"); \
exit(1); }
```c
int main()
{
int fd;
int x, y, pos;
int r, g, b;
unsigned short color;
void *fbmem;
struct fb_var_screeninfo var_info;
struct fb_fix_screeninfo fix_info;
```
fd = open(FBVIDEO, O_RDWR);
```c
if (tfd == -1 || vfd == -1) {
exit(-1);
}
```
/* Gather variable screen info (virtual and visible) */
```c
FBCTL(fd, FBIOGET_VSCREENINFO, &var_info);
```
/* Gather fixed screen info */
```c
FBCTL(fd, FBIOGET_FSCREENINFO, &fix_info);
printf("****** Frame Buffer Info ******\n");
```
printf("Visible: %d,%d \nvirtual: %d,%d \n line_len %d\n",
```c
var_info.xres, this->var_info.yres,
```
var_info.xres_virtual, var_info.yres_virtual,
fix_info.line_length);
```c
printf("dim %d,%d\n\n", var_info.width, var_info.height);
```
/* Let's mmap frame buffer memory */
fbmem = mmap(0, v_var.yres_virtual * v_fix.line_length, \
PROT_WRITE | PROT_READ, \
MAP_SHARED, fd, 0);
```c
if (fbmem == MAP_FAILED) {
perror("Video or Text frame buffer mmap failed");
exit(1);
}
```
/* upper left corner (100,100). The square is 300px width */
```c
for (y = 100; y < 400; y++) {
for (x = 100; x < 400; x++) {
```
pos = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8)
+ (y + vinfo.yoffset) * finfo.line_length;
/* if 32 bits per pixel */
```c
if (vinfo.bits_per_pixel == 32) {
```
/* We prepare some blue color */
*(fbmem + pos) = 100;
/* adding a little green */
*(fbmem + pos + 1) = 15+(x-100)/2;
/* With lot of read */
*(fbmem + pos + 2) = 200-(y-100)/5;
/* And no transparency */
*(fbmem + pos + 3) = 0;
```c
} else { /* This assume 16bpp */
```
r = 31-(y-100)/16;
g = (x-100)/6;
b = 10;
/* Compute color */
color = r << 11 | g << 5 | b;
*((unsigned short int*)(fbmem + pos)) = color;
```c
}
}
}
munmap(fbp, screensize);
close(fbfd);
return 0;
}
```
You can also dump the framebuffer memory into a raw image, using the cat or dd command:
```bash
# cat /dev/fb0 > my_image
```
Write it back using:
```bash
# cat my_image > /dev/fb0
```
It is possible to blank/unblank the screen through a special
```bash
/sys/class/graphics/fb<N>/blank sysfs file, where <N> is the framebuffer index.
```
Writing 1 will blank the screen, whereas 0 will unblank it:
```bash
# echo 0 > /sys/class/graphics/fb0/blank
# echo 1 > /sys/class/graphics/fb0/blank
```
## Summary
Framebuffer drivers are the simplest form of Linux graphics drivers, requiring little implementation work. They heavily abstract hardware. At this stage, you should be able to either enhance an existing driver (with graphical acceleration functions, for example), or write a fresh one from scratch. However, it is recommended to rely on an existing driver whose hardware shares as many characteristics as possible with the one you need to write the driver for. Let us jump to the next and last chapter, dealing with network devices