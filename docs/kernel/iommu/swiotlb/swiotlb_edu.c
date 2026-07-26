// SPDX-License-Identifier: GPL-2.0
/*
 * Deterministic SWIOTLB exercise using QEMU's EDU PCI device.
 *
 * The VM must run the EDU device with dma_mask=0xffffffff and use an
 * identity/passthrough IOMMU domain.  This driver gives only EDU a 32-bit
 * DMA mask, allocates a page above 4 GiB, and performs a DMA round trip.
 */

#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>

#define EDU_VENDOR_ID		0x1234
#define EDU_DEVICE_ID		0x11e8

#define EDU_DMA_SRC		0x80
#define EDU_DMA_DST		0x88
#define EDU_DMA_COUNT		0x90
#define EDU_DMA_CMD		0x98
#define EDU_DMA_RUN		BIT_ULL(0)
#define EDU_DMA_TO_RAM		BIT_ULL(1)
#define EDU_INTERNAL_BUFFER	0x40000

#define TEST_SIZE		256
#define MAX_LOW_PAGES		1024

struct swiotlb_edu {
	void __iomem *mmio;
	struct page *page;
};

static struct page *alloc_page_above_4g(void)
{
	struct page **low_pages;
	struct page *page = NULL;
	unsigned int nr_low = 0;
	unsigned int i;

	low_pages = kcalloc(MAX_LOW_PAGES, sizeof(*low_pages), GFP_KERNEL);
	if (!low_pages)
		return NULL;

	for (i = 0; i < MAX_LOW_PAGES; i++) {
		page = alloc_page(GFP_KERNEL | __GFP_ZERO | __GFP_NOWARN);
		if (!page)
			break;
		if (page_to_phys(page) > DMA_BIT_MASK(32))
			break;
		low_pages[nr_low++] = page;
		page = NULL;
	}

	for (i = 0; i < nr_low; i++)
		__free_page(low_pages[i]);
	kfree(low_pages);

	return page;
}

static int edu_dma_transfer(void __iomem *mmio, dma_addr_t ram,
			    bool to_ram)
{
	u64 command;
	u64 status;

	if (to_ram) {
		writeq(EDU_INTERNAL_BUFFER, mmio + EDU_DMA_SRC);
		writeq(ram, mmio + EDU_DMA_DST);
		command = EDU_DMA_RUN | EDU_DMA_TO_RAM;
	} else {
		writeq(ram, mmio + EDU_DMA_SRC);
		writeq(EDU_INTERNAL_BUFFER, mmio + EDU_DMA_DST);
		command = EDU_DMA_RUN;
	}

	writeq(TEST_SIZE, mmio + EDU_DMA_COUNT);
	writeq(command, mmio + EDU_DMA_CMD);

	return readq_poll_timeout(mmio + EDU_DMA_CMD, status,
				  !(status & EDU_DMA_RUN), 1000, 500000);
}

static void fill_pattern(u8 *buffer)
{
	unsigned int i;

	for (i = 0; i < TEST_SIZE; i++)
		buffer[i] = (i * 37) ^ 0xa5;
}

static int verify_pattern(const u8 *buffer)
{
	unsigned int i;

	for (i = 0; i < TEST_SIZE; i++) {
		if (buffer[i] != (u8)((i * 37) ^ 0xa5))
			return -EIO;
	}
	return 0;
}

static int swiotlb_edu_probe(struct pci_dev *pdev,
			     const struct pci_device_id *id)
{
	struct device *dev = &pdev->dev;
	struct swiotlb_edu *edu;
	dma_addr_t dma;
	phys_addr_t phys;
	u8 *buffer;
	int ret;

	ret = pcim_enable_device(pdev);
	if (ret)
		return dev_err_probe(dev, ret, "cannot enable EDU device\n");

	ret = pcim_iomap_regions(pdev, BIT(0), KBUILD_MODNAME);
	if (ret)
		return dev_err_probe(dev, ret, "cannot map EDU BAR0\n");

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return dev_err_probe(dev, ret, "cannot set 32-bit DMA mask\n");

	edu = devm_kzalloc(dev, sizeof(*edu), GFP_KERNEL);
	if (!edu)
		return -ENOMEM;
	edu->mmio = pcim_iomap_table(pdev)[0];
	pci_set_drvdata(pdev, edu);
	pci_set_master(pdev);

	edu->page = alloc_page_above_4g();
	if (!edu->page)
		return dev_err_probe(dev, -ENOMEM,
				     "cannot allocate a page above 4 GiB\n");

	buffer = page_address(edu->page);
	phys = page_to_phys(edu->page);
	fill_pattern(buffer);

	dma = dma_map_single(dev, buffer, TEST_SIZE, DMA_TO_DEVICE);
	if (dma_mapping_error(dev, dma)) {
		ret = -EIO;
		dev_err(dev, "DMA_TO_DEVICE mapping failed\n");
		goto free_page;
	}

	dev_info(dev,
		 "DMA_TO_DEVICE original phys=%pa mapped dma=%pad (%s)\n",
		 &phys, &dma, dma == phys ? "not bounced" : "SWIOTLB bounced");
	if (dma == phys) {
		ret = -EINVAL;
		dev_err(dev, "mapping did not use SWIOTLB; check iommu.passthrough=1\n");
		goto unmap_to_device;
	}

	ret = edu_dma_transfer(edu->mmio, dma, false);
	if (ret) {
		dev_err(dev, "RAM-to-EDU DMA timed out\n");
		goto unmap_to_device;
	}

	dma_unmap_single(dev, dma, TEST_SIZE, DMA_TO_DEVICE);
	memset(buffer, 0, TEST_SIZE);

	dma = dma_map_single(dev, buffer, TEST_SIZE, DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, dma)) {
		ret = -EIO;
		dev_err(dev, "DMA_FROM_DEVICE mapping failed\n");
		goto free_page;
	}

	dev_info(dev,
		 "DMA_FROM_DEVICE original phys=%pa mapped dma=%pad (%s)\n",
		 &phys, &dma, dma == phys ? "not bounced" : "SWIOTLB bounced");
	if (dma == phys) {
		ret = -EINVAL;
		dev_err(dev, "DMA_FROM_DEVICE mapping did not use SWIOTLB\n");
		goto unmap_from_device;
	}

	ret = edu_dma_transfer(edu->mmio, dma, true);
	if (ret)
		dev_err(dev, "EDU-to-RAM DMA timed out\n");

unmap_from_device:
	dma_unmap_single(dev, dma, TEST_SIZE, DMA_FROM_DEVICE);
	if (ret)
		goto free_page;

	ret = verify_pattern(buffer);
	if (ret)
		dev_err(dev, "DMA round-trip data verification failed\n");
	else
		dev_info(dev, "SWIOTLB DMA round-trip passed (%u bytes)\n",
			 TEST_SIZE);
	if (!ret)
		return 0;

free_page:
	__free_page(edu->page);
	edu->page = NULL;
	return ret;

unmap_to_device:
	dma_unmap_single(dev, dma, TEST_SIZE, DMA_TO_DEVICE);
	goto free_page;
}

static void swiotlb_edu_remove(struct pci_dev *pdev)
{
	struct swiotlb_edu *edu = pci_get_drvdata(pdev);

	if (edu->page)
		__free_page(edu->page);
}

static const struct pci_device_id swiotlb_edu_ids[] = {
	{ PCI_DEVICE(EDU_VENDOR_ID, EDU_DEVICE_ID) },
	{ }
};
MODULE_DEVICE_TABLE(pci, swiotlb_edu_ids);

static struct pci_driver swiotlb_edu_driver = {
	.name = KBUILD_MODNAME,
	.id_table = swiotlb_edu_ids,
	.probe = swiotlb_edu_probe,
	.remove = swiotlb_edu_remove,
};
module_pci_driver(swiotlb_edu_driver);

MODULE_AUTHOR("martins3");
MODULE_DESCRIPTION("Exercise SWIOTLB with QEMU EDU PCI DMA");
MODULE_LICENSE("GPL");
