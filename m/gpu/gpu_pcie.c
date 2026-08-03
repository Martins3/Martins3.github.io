#include "driver.h"
#include "hw.h"
#include <linux/dev_printk.h>

wait_queue_head_t wq;
volatile int irq_fired = 0; // TODO volatile 不可以这样用的
static struct class *gpu_class;

static irqreturn_t irq_handler(int irq, void *data)
{
	GpuState *gpu = data;
	(void)gpu;
	irq_fired = 1;
	wake_up_interruptible(&wq);
	return IRQ_HANDLED;
}

static int setup_msi(GpuState *gpu)
{
	int err;
	int msi_vecs;
	int irq_num;

	init_waitqueue_head(&wq);

	msi_vecs = pci_alloc_irq_vectors(gpu->pdev, IRQ_COUNT, IRQ_COUNT,
					 PCI_IRQ_MSIX | PCI_IRQ_MSI);
	if (msi_vecs < 0) {
		pr_err("Could not allocate MSI vectors");
		return -ENOSPC;
	}

	irq_num = pci_irq_vector(gpu->pdev, IRQ_DMA_DONE_NR);
	// TODO 测试中断线程化的好地方
	pr_info("Got MSI vec %d, IRQ num %d", msi_vecs, irq_num);
	err = request_threaded_irq(irq_num, irq_handler, NULL, 0, "GPU-Dma0",
				   gpu);
	if (err)
		pr_err("Failed to get threaded IRQ: %d", err);

	irq_num = pci_irq_vector(gpu->pdev, IRQ_TEST);
	pr_info("Got [test] IRQ num %d", irq_num);
	err = request_threaded_irq(irq_num, irq_handler, NULL, 0, "GPU-Test",
				   gpu);
	if (err)
		pr_err("Failed to get threaded IRQ: %d", err);
	return 0;
}

static int gpu_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int bars;
	int err;
	unsigned long mmio_start, mmio_len;
	GpuState *gpu =
		devm_kmalloc(&pdev->dev, sizeof(struct GpuState), GFP_KERNEL);
	gpu->pdev = pdev;
	dev_info(&pdev->dev, "GpuState : %px\n", gpu);
	dev_info(&pdev->dev, "called probe");

	// create a bitfield of the available BARs
	bars = pci_select_bars(pdev, IORESOURCE_MEM);
	if (pci_enable_device_mem(pdev))
		return -EINVAL;

	// claim ownership of the address space for each BAR in the bitfield
	if (pci_request_region(pdev, bars, "gpu-pci"))
		return -ENOMEM;

	mmio_start = pci_resource_start(pdev, 0);
	mmio_len = pci_resource_len(pdev, 0);

	// map physical address to virtual
	gpu->hwmem = ioremap(mmio_start, mmio_len);
	pr_info("mmio starts at 0x%lx; hwmem 0x%px", mmio_start, gpu->hwmem);

	mmio_start = pci_resource_start(pdev, 1);
	mmio_len = pci_resource_len(pdev, 1);

	// map physical address to virtual
	gpu->fbmem = ioremap(mmio_start, mmio_len);
	pr_info("fb starts at 0x%lx; hwmem 0x%px", mmio_start, gpu->fbmem);
	gpu->fb_phys = mmio_start;

	setup_chardev(gpu, gpu_class, pdev);

	setup_fbdev(gpu, pdev);
	// need to set dma_mask & bus master to get IRQs
	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (err) {
		pr_err("could not set dma mask: %d\n", err);
	}
	pci_set_master(pdev);
	setup_msi(gpu);
	return 0;
};

static void gpu_remove(struct pci_dev *pdev)
{
	// TODO 这个要配置一下的，现在获取到的 gpu = NULL
	struct GpuState *gpu = pci_get_drvdata(pdev);
	dev_info(&pdev->dev, "GpuState : %px\n", gpu);
	pci_free_irq_vectors(pdev);
}

static struct pci_device_id gpu_id_tbl[] = {
	{ PCI_DEVICE(0x1234, 0x1337) },
	{},
};
MODULE_DEVICE_TABLE(pci, gpu_id_tbl);
static struct pci_driver gpu_driver = {
	.name = "gpu",
	.id_table = gpu_id_tbl,
	.probe = gpu_probe,
	.remove = gpu_remove,
};

static int __init gpu_init(void)
{
	gpu_class = class_create("gpu_pcie");
	return pci_register_driver(&gpu_driver);
}

static void __exit gpu_exit(void)
{
	pci_unregister_driver(&gpu_driver);
}

MODULE_AUTHOR("Martins3 <hubachelar@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("GPU driver for education");
module_init(gpu_init);
module_exit(gpu_exit);
