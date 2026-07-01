// SPDX-License-Identifier: MIT
/*
 * Learning-only fbdev userspace test.
 *
 * Draws a rectangle into /dev/fb0 using FBIOGET_*SCREENINFO and mmap().
 * This is intentionally small: it demonstrates stride, offsets, mmap length,
 * and color bitfields. It is not a production graphics stack.
 */

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

static uint32_t scale_to_field(uint8_t value, uint32_t length)
{
	if (length == 0)
		return 0;

	if (length >= 8)
		return value << (length - 8);

	return value >> (8 - length);
}

static uint32_t pack_rgb(const struct fb_var_screeninfo *var,
			 uint8_t red, uint8_t green, uint8_t blue)
{
	uint32_t pixel = 0;

	pixel |= scale_to_field(red, var->red.length) << var->red.offset;
	pixel |= scale_to_field(green, var->green.length) << var->green.offset;
	pixel |= scale_to_field(blue, var->blue.length) << var->blue.offset;

	if (var->transp.length) {
		uint32_t alpha = var->transp.length >= 32 ?
				 UINT32_MAX : ((1U << var->transp.length) - 1U);

		pixel |= alpha << var->transp.offset;
	}

	return pixel;
}

static void put_pixel(uint8_t *fb, const struct fb_var_screeninfo *var,
		      const struct fb_fix_screeninfo *fix,
		      unsigned int x, unsigned int y, uint32_t pixel)
{
	unsigned int bytes_per_pixel = var->bits_per_pixel / 8;
	size_t offset = (size_t)(y + var->yoffset) * fix->line_length +
			(size_t)(x + var->xoffset) * bytes_per_pixel;

	switch (bytes_per_pixel) {
	case 2:
	{
		uint16_t value = (uint16_t)pixel;

		memcpy(fb + offset, &value, sizeof(value));
		break;
	}
	case 4:
		memcpy(fb + offset, &pixel, sizeof(pixel));
		break;
	default:
		break;
	}
}

int main(int argc, char **argv)
{
	const char *fbdev = argc > 1 ? argv[1] : "/dev/fb0";
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	unsigned int bytes_per_pixel;
	unsigned int x0, y0, x1, y1;
	uint32_t pixel;
	size_t map_len;
	uint8_t *fb;
	int fd;

	fd = open(fbdev, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open %s: %s\n", fbdev, strerror(errno));
		return EXIT_FAILURE;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
		fprintf(stderr, "FBIOGET_VSCREENINFO: %s\n", strerror(errno));
		close(fd);
		return EXIT_FAILURE;
	}

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0) {
		fprintf(stderr, "FBIOGET_FSCREENINFO: %s\n", strerror(errno));
		close(fd);
		return EXIT_FAILURE;
	}

	bytes_per_pixel = var.bits_per_pixel / 8;
	if (bytes_per_pixel != 2 && bytes_per_pixel != 4) {
		fprintf(stderr,
			"unsupported %u bpp; this learning test handles 16/32 bpp only\n",
			var.bits_per_pixel);
		close(fd);
		return EXIT_FAILURE;
	}

	map_len = (size_t)fix.line_length * var.yres_virtual;
	fb = mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (fb == MAP_FAILED) {
		fprintf(stderr, "mmap: %s\n", strerror(errno));
		close(fd);
		return EXIT_FAILURE;
	}

	printf("fbdev: %s\n", fbdev);
	printf("visible: %ux%u, virtual: %ux%u, offset: %u,%u\n",
	       var.xres, var.yres, var.xres_virtual, var.yres_virtual,
	       var.xoffset, var.yoffset);
	printf("bpp: %u, line_length: %u, mapped: %zu bytes\n",
	       var.bits_per_pixel, fix.line_length, map_len);
	printf("red(off=%u,len=%u) green(off=%u,len=%u) blue(off=%u,len=%u)\n",
	       var.red.offset, var.red.length,
	       var.green.offset, var.green.length,
	       var.blue.offset, var.blue.length);

	x0 = var.xres / 8;
	y0 = var.yres / 8;
	x1 = var.xres * 5 / 8;
	y1 = var.yres * 5 / 8;
	pixel = pack_rgb(&var, 0x20, 0x80, 0xff);

	for (unsigned int y = y0; y < y1; y++) {
		for (unsigned int x = x0; x < x1; x++)
			put_pixel(fb, &var, &fix, x, y, pixel);
	}

	if (msync(fb, map_len, MS_SYNC) < 0)
		fprintf(stderr, "msync warning: %s\n", strerror(errno));

	munmap(fb, map_len);
	close(fd);

	return EXIT_SUCCESS;
}
