#define BAR_COUNT 	2

#define CTRL_BAR 	0
#define FB_BAR 		1

#define REG_DMA_DIR 		0
#define REG_DMA_ADDR_SRC 	1
#define REG_DMA_ADDR_DST 	2
#define REG_DMA_LEN 		3
#define REG_DMA_IRQ_SET 	4

#define CMD_ADDR_BASE 		0xf00
#define MSIX_ADDR_BASE 		0x1000
#define PBA_ADDR_BASE 		0x3000
#define CMD_DMA_START 		(CMD_ADDR_BASE + 0)


#define IRQ_DMA_DONE_NR 	0
#define IRQ_TEST 			1

// should be power of 2
#define IRQ_COUNT 		2


#define DIR_HOST_TO_GPU  0
#define DIR_GPU_TO_HOST  1
