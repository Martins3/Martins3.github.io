#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "hw/pci/pci.h"
#include "hw/pci/msi.h"
#include "hw/pci/msix.h"
#include "qemu/timer.h"
#include "qom/object.h"
#include "qemu/main-loop.h" /* iothread mutex */
#include "qemu/module.h"
#include "qapi/visitor.h"
#include "hw.h"
#include "ui/console.h"

#define TYPE_PCI_GPU_DEVICE 	"gpu"
#define GPU_DEVICE_ID         	0x1337
typedef struct GpuState GpuState;
DECLARE_INSTANCE_CHECKER(GpuState, GPU, TYPE_PCI_GPU_DEVICE)

struct GpuState {
    PCIDevice pdev;
    MemoryRegion mem;
    MemoryRegion fbmem;
    MemoryRegion msix;
	QemuConsole* con;
	uint32_t registers[0x100000 / 32]; // 1 MiB = 32k, 32 bit registers
	uint32_t framebuffer[0x200000]; // barely enough for 1920x1080 at 32bpp
};

static void pci_gpu_register_types(void);
static void gpu_instance_init(Object *obj);
static void gpu_class_init(ObjectClass *class, const void *data);
static void pci_gpu_realize(PCIDevice *pdev, Error **errp);
static void pci_gpu_uninit(PCIDevice *pdev);

type_init(pci_gpu_register_types)

static void pci_gpu_register_types(void)
{
    static InterfaceInfo interfaces[] = {
        { INTERFACE_PCIE_DEVICE },
        { },
    };
    static const TypeInfo gpu_info = {
        .name          = TYPE_PCI_GPU_DEVICE,
        .parent        = TYPE_PCI_DEVICE,
        .instance_size = sizeof(GpuState),
        .instance_init = gpu_instance_init,
        .class_init    = gpu_class_init,
        .interfaces = interfaces,
    };

    type_register_static(&gpu_info);
}

static void gpu_instance_init(Object *obj) {
    printf("GPU instance init\n");
}

static void gpu_class_init(ObjectClass *class, const void *data) {
    PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

    k->realize = pci_gpu_realize;
    k->exit = pci_gpu_uninit;
    k->vendor_id = PCI_VENDOR_ID_QEMU;
    k->device_id = GPU_DEVICE_ID;
    k->class_id = PCI_CLASS_DISPLAY_OTHER;

    DeviceClass *dc = DEVICE_CLASS(class);
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static uint64_t lower_n_bytes(uint64_t data, unsigned nbytes) {
	uint64_t result;
	if (nbytes < 8) {
		uint64_t bitcount = ((uint64_t)nbytes)<<3;
		uint64_t mask = (1ULL << bitcount)-1;
		result = data & mask;
	} else {
		result = data;
	}
	return result;
}

static uint64_t gpu_control_read(void *opaque, hwaddr addr, unsigned size) {
	GpuState *gpu = opaque;
	uint64_t index = lower_n_bytes(addr, size);
	uint32_t index_u32 = index / sizeof(uint32_t);
	uint64_t result = ((uint32_t*)gpu->registers)[index_u32];
	printf("reading idx %lu = %lu\n", index, result);
	return result;
}

static void gpu_control_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) {
	GpuState *gpu = opaque;
	val = lower_n_bytes(val, size);
	uint32_t reg = addr / 4;
	printf("writing addr %ld [reg %d], size %u = %lu\n", addr, reg, size, val);
	// TODO: should write only masked bits, not whole u64
	if (reg < CMD_ADDR_BASE) { // register
		gpu->registers[reg] = (uint32_t)val;
		return;
	}
	switch (reg) {
		case CMD_DMA_START:
			// should mask address space
			if (gpu->registers[REG_DMA_DIR] == DIR_HOST_TO_GPU) {
				printf("Copy to GPU, from 0x%x (host) to offset 0x%x (dev), len 0x%x\n",
						gpu->registers[REG_DMA_ADDR_SRC],
						gpu->registers[REG_DMA_ADDR_DST],
						gpu->registers[REG_DMA_LEN]);

				int err = pci_dma_read(&gpu->pdev,
							 gpu->registers[REG_DMA_ADDR_SRC], // host addr
							 (gpu->framebuffer + gpu->registers[REG_DMA_ADDR_DST]), // dev addr
							 gpu->registers[REG_DMA_LEN]);
                if(err)
				    printf("pci_dma_read\n");
				msix_notify(&gpu->pdev, IRQ_DMA_DONE_NR);

			} else {
				printf("Unimplemented DMA direction\n");
			}
			break;
		case REG_DMA_IRQ_SET:
			// debugging
			printf("!supposed to set the IRQ value to %ld\n", val);
			if (val == 0) {
				msix_notify(&gpu->pdev, IRQ_TEST);
				//msix_clr_pending(&gpu->pdev, IRQ_TEST);
			} else {
				msix_notify(&gpu->pdev, IRQ_DMA_DONE_NR);
				//msix_clr_pending(&gpu->pdev, IRQ_DMA_DONE_NR);
			}
			break;
	}
}

static const MemoryRegionOps gpu_mem_ops = {
    .read = gpu_control_read,
    .write = gpu_control_write,
	.valid = {
        .min_access_size = 0x1,
        .max_access_size = 0x4,
    },
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void vga_invalidate_display(void *opaque) {
	printf("invalidated display\n");
}
static void vga_update_text(void *opaque, console_ch_t *chardata) {
	printf("updated text\n");
}
static void vga_update_display(void *opaque) {
//	printf("updated display\n");
	GpuState* gpu = opaque;
    DisplaySurface *surface = qemu_console_surface(gpu->con);
	for(int i = 0; i<640*480; i++) {
		((uint32_t*)surface_data(surface))[i] = gpu->framebuffer[i % 0x200000 ];
	}

	dpy_gfx_update(gpu->con, 0, 0, 640, 480);
	//qemu_console_resize(gpu->con, 800, 600);
}

static const GraphicHwOps ghwops = {
   .invalidate  = vga_invalidate_display,
   .gfx_update  = vga_update_display,
   .text_update = vga_update_text,
};

static void pci_gpu_realize(PCIDevice *pdev, Error **errp) {
    printf("GPU Realize\n");
    GpuState *gpu = GPU(pdev);
    memory_region_init_io(&gpu->mem, OBJECT(gpu), &gpu_mem_ops, gpu, "gpu-control-mem", 16 * MiB);
    //memory_region_init(&gpu->fbmem, OBJECT(gpu), "gpu-fb-mem", 16 * MiB);
    memory_region_init_ram(&gpu->fbmem, OBJECT(gpu), "gpu-fb-mem", 16 * MiB, errp);
    //memory_region_init(&gpu->msix, OBJECT(gpu), "gpu-msix", 16 * KiB);

    pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &gpu->mem);
    pci_register_bar(pdev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &gpu->fbmem);
    //pci_register_bar(pdev, 2, PCI_BASE_ADDRESS_SPACE_MEMORY, &gpu->msix);

	int ret = msix_init(pdev, IRQ_COUNT, &gpu->mem, 0 /* table_bar_nr  = bar id */, MSIX_ADDR_BASE,
						&gpu->mem, 0 /* pba_bar_nr  = bar id */, PBA_ADDR_BASE, 0x0 /* cap_pos ?? */,errp);
	if (ret) {
		printf("msi init ret %d\n", ret);
	}
	for(int i = 0; i < IRQ_COUNT; i++) {
		msix_vector_use(pdev, i);
	}

    gpu->con = graphic_console_init(DEVICE(pdev), 0, &ghwops, gpu);
    DisplaySurface *surface = qemu_console_surface(gpu->con);
	for(int i = 0; i<640*480; i++) {
		((uint32_t*)surface_data(surface))[i] = i;
	}
}

static void pci_gpu_uninit(PCIDevice *pdev) {
    printf("GPU un-init\n");
}
