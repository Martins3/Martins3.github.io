#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <stdint.h>

// Simple program to create a red triangle on a framebuffer (for VNC display)
// This approach directly manipulates the framebuffer to draw a triangle.
// When /dev/fb0 is not available, it falls back to an in-memory buffer and
// writes the result to triangle_fb.ppm so the demo can still be inspected.

static void draw_red_triangle_to_buffer(uint8_t *fbp, int width, int height,
					int bpp, int stride)
{
	// Clear the screen (black background)
	for (int y = 0; y < height; y++) {
		memset(fbp + y * stride, 0, (size_t)width * bpp / 8);
	}

	// Define triangle vertices
	int x1 = width / 2; // Top vertex
	int y1 = height / 4;
	int x2 = width / 4; // Bottom left vertex
	int y2 = 3 * height / 4;
	int x3 = 3 * width / 4; // Bottom right vertex
	int y3 = 3 * height / 4;

	// Simple triangle drawing algorithm (using barycentric coordinates)
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			// Calculate barycentric coordinates
			int w0 = (x - x2) * (y1 - y2) - (y - y2) * (x1 - x2);
			int w1 = (x - x3) * (y2 - y3) - (y - y3) * (x2 - x3);
			int w2 = (x - x1) * (y3 - y1) - (y - y1) * (x3 - x1);

			// Check if point is inside triangle
			if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
			    (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
				// Set pixel to red color (format depends on bpp)
				if (bpp == 32) {
					long int location =
						x * (bpp / 8) + y * stride;
					*(uint32_t *)(fbp + location) =
						0xFF0000FF; // Red in RGBA format
				} else if (bpp == 24) {
					long int location =
						x * (bpp / 8) + y * stride;
					*(fbp + location) = 0xFF; // Blue
					*(fbp + location + 1) = 0x00; // Green
					*(fbp + location + 2) = 0x00; // Red
				}
			}
		}
	}
}

static int save_ppm(const char *path, int width, int height,
		    const uint8_t *fbp, int bpp, int stride)
{
	FILE *fp = fopen(path, "wb");
	if (!fp) {
		perror("fopen");
		return -1;
	}

	fprintf(fp, "P6\n%d %d\n255\n", width, height);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			long int location = x * (bpp / 8) + y * stride;
			unsigned char r, g, b;

			// The 32-bit path stored 0xFF0000FF as a uint32_t.  On a
			// little-endian machine the bytes in memory are R=0xFF, G=0x00,
			// B=0x00, A=0xFF when the framebuffer pixel format is RGBA.
			r = fbp[location + 0];
			g = fbp[location + 1];
			b = fbp[location + 2];
			fputc(r, fp);
			fputc(g, fp);
			fputc(b, fp);
		}
	}

	fclose(fp);
	return 0;
}

static int draw_red_triangle_to_framebuffer()
{
	int fb_fd = open("/dev/fb0", O_RDWR);
	if (fb_fd == -1) {
		printf("Cannot open framebuffer device /dev/fb0\n");
		return -1;
	}

	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) ||
	    ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo)) {
		printf("Cannot get framebuffer info\n");
		close(fb_fd);
		return -1;
	}

	printf("Framebuffer: %dx%d, %dbpp\n", vinfo.xres, vinfo.yres,
	       vinfo.bits_per_pixel);

	long int screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
	char *fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE,
				 MAP_SHARED, fb_fd, 0);

	if (fbp == MAP_FAILED) {
		printf("Cannot map framebuffer to memory\n");
		close(fb_fd);
		return -1;
	}

	draw_red_triangle_to_buffer((uint8_t *)fbp, vinfo.xres, vinfo.yres,
				    vinfo.bits_per_pixel, finfo.line_length);

	printf("Red triangle drawn to framebuffer at /dev/fb0\n");

	munmap(fbp, screensize);
	close(fb_fd);
	return 0;
}

static int draw_red_triangle_to_file()
{
	const int width = 512;
	const int height = 512;
	const int bpp = 32;
	const int stride = width * bpp / 8;
	const char *path = "triangle_fb.ppm";

	uint8_t *fbp = calloc(height, stride);
	if (!fbp) {
		printf("Failed to allocate memory buffer\n");
		return -1;
	}

	draw_red_triangle_to_buffer(fbp, width, height, bpp, stride);

	if (save_ppm(path, width, height, fbp, bpp, stride) != 0) {
		free(fbp);
		return -1;
	}

	printf("Framebuffer unavailable; wrote software-rendered triangle to %s\n",
	       path);
	free(fbp);
	return 0;
}

int main()
{
	setbuf(stdout, NULL);

	printf("Drawing a red triangle directly to framebuffer for VNC display\n");
	printf("==============================================================\n");

	// Try to draw red triangle to framebuffer
	if (draw_red_triangle_to_framebuffer() < 0) {
		printf("Framebuffer method failed, falling back to software buffer...\n");
		if (draw_red_triangle_to_file() == 0) {
			printf("Software fallback succeeded.\n");
			return 0;
		}
		printf("Software fallback also failed.\n");
		return 1;
	}

	printf("Red triangle successfully drawn to framebuffer!\n");
	printf("This should be visible in VNC if the framebuffer is accessible.\n");

	return 0;
}
