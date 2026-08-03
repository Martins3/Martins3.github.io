#include "internal.h"
#include <asm/io.h>
#include <linux/pci.h>

// https://stackoverflow.com/questions/18350021/how-to-print-exact-value-of-the-program-counter-in-c
static __attribute__((__noinline__)) void *get_pc(void)
{
	return __builtin_return_address(0);
}

/*
 * https://nixhacker.com/digging-into-smm/
 * */
static void basic_trigger(void)
{
	uint8_t smm_response = 0x0;
	printk(KERN_INFO "Hello world.\n");

	//check the initial value recieve from 0xB2 port
	smm_response = inb(0xb2);

	printk(KERN_INFO "data recieved from port 0xb2 is %x.\n", smm_response);
	//write to 0xB2 port to cause SMI
	outb(0x80, 0xb2);
	// Check the respose
	smm_response = inb(0xb2);

	printk(KERN_INFO "data recieved after is %x.\n", smm_response);
}

static void basic_check(void)
{
	uint8_t smm_response = 0x0;
	uint32_t smi_count = 0x0;
	printk(KERN_INFO "Hello world.\n");

	// check smi count
	rdmsrl(MSR_SMI_COUNT, smi_count);
	printk(KERN_INFO "SMI count earlier is %x.\n", smi_count);
	//check the initial value recieve from 0xB3 port
	smm_response = inb(0xb2);

	printk(KERN_INFO "data recieved from port 0xb2 is %x.\n", smm_response);
	//write to 0xB2 port to cause SMI
	outb(0x80, 0xb2);
	// Check the respose in port 0xB3
	smm_response = inb(0xb2);

	printk(KERN_INFO "data recieved after is %x.\n", smm_response);

	rdmsrl(MSR_SMI_COUNT, smi_count);
	printk(KERN_INFO "SMI count later is- %x.\n", smi_count);
}

static int smram(void)
{
	struct pci_dev *dev;
	u8 pci_data;
	//get pci device
	/* dev = pci_get_device(0x8086, 0x1237, NULL); */
	dev = pci_get_device(0x8086, 0x29c0, NULL);
	if (!dev) {
		printk(KERN_INFO "FAILED to get pci device\n");
		pci_dev_put(dev);
		return -ENODEV;
	}
	printk(KERN_INFO "Attached to the pci device\n");
	//Offset 0x88 in DRAM controller
	pci_read_config_byte(dev, 0x88, &pci_data);
	printk(KERN_INFO "SMRAMC data recieved is 0x%x.\n", pci_data);

	// Bit 3 in SMRAMC is G_SMRAME
	pci_data = pci_data >> 3;
	pci_data = pci_data & 0x1;
	printk(KERN_INFO "G_SMRAME is set to %x.\n", pci_data);

	//cleanup after use
	pci_dev_put(dev);

	return 0;
}

int test_smm(long action)
{
	switch (action) {
	case 0:
		basic_check();
		break;
	case 1:
		basic_trigger();
		break;
	case 2:
		smram();
		break;
	case 3:
		pr_info("%lx\n", (long)get_pc());
		break;
	}
	return 0;
}
