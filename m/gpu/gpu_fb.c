#include "driver.h"

static struct fb_fix_screeninfo myfb_fix = {
	.id = "gpufb",
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR,
	.xpanstep = 0,
	.ypanstep = 0,
	.ywrapstep = 0,
	.line_length = 640 * 4,
	.accel = FB_ACCEL_NONE,
};

static struct fb_var_screeninfo myfb_var = {
	.xres = 640,
	.yres = 480,
	.xres_virtual = 640,
	.yres_virtual = 480,
	.bits_per_pixel = 32,
	.red = { 16, 8, 0 },
	.green = { 8, 8, 0 },
	.blue = { 0, 8, 0 },
	.activate = FB_ACTIVATE_NOW,
};

// copied from efifb
static int myfb_setcolreg(unsigned regno, unsigned red, unsigned green,
			  unsigned blue, unsigned transp, struct fb_info *info)
{
	if (regno < 16) {
		red >>= 8;
		green >>= 8;
		blue >>= 8;
		((u32 *)(info->pseudo_palette))[regno] =
			(red << info->var.red.offset) |
			(green << info->var.green.offset) |
			(blue << info->var.blue.offset);
	}
	return 0;
}

static struct fb_ops myfb_ops = {
	.owner = THIS_MODULE,
	.fb_setcolreg = myfb_setcolreg,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
};

int setup_fbdev(GpuState *gpu, struct pci_dev *pdev)
{
	gpu->info = framebuffer_alloc(sizeof(u32) * 16, &pdev->dev);
	gpu->info->pseudo_palette = gpu->info->par;
	gpu->info->par = NULL;
	gpu->info->screen_base = gpu->fbmem;
	gpu->info->fbops = &myfb_ops;
	gpu->info->fix = myfb_fix;
	gpu->info->fix.smem_start = gpu->fb_phys;
	gpu->info->fix.smem_len = 640 * 480 * 4;
	gpu->info->var = myfb_var;
	// 这里的 api 变化了
	// TODO https://lore.kernel.org/lkml/20230715185343.7193-1-tzimmermann@suse.de/T/#r79be7cbed2b7cd5040789661b37af1b08d8a744a
	/* gpu->info->flags = FBINFO_FLAG_DEFAULT; */
	gpu->info->screen_size = 640 * 480 * 4;
	if (fb_alloc_cmap(&gpu->info->cmap, 256, 0) < 0) {
		pr_err("NOMEM");
		return -ENOMEM;
	}

	pr_info("registering info, fbmem=%px, smem_start=%lx",
		gpu->info->screen_base, gpu->info->fix.smem_start);
	return register_framebuffer(gpu->info);
}
