# TOOD
- [ ] 中断负载均衡在哪里做的 ?

## msi
- [ ] 应该 msi 是放到 extioi 下，其地位和 pch 差不多的吧




## 谁驱动的 usb 设备注册中断

这些设备都是 local pci probe 获取的:

## 1
- [ ] 分析一下这里的调用规则是什么 ?
```
[    1.348667] [<900000000020864c>] show_stack+0x2c/0x100
[    1.348672] [<9000000000ec3988>] dump_stack+0x90/0xc0
[    1.348676] [<900000000081f550>] pch_pic_alloc+0x48/0xf8
[    1.348680] [<90000000002a0648>] __irq_domain_alloc_irqs+0x1b0/0x2e0
[    1.348682] [<90000000002a0bf0>] irq_create_fwspec_mapping+0x258/0x368
[    1.348685] [<900000000020f3d0>] acpi_register_gsi+0x70/0x100
[    1.348690] [<90000000008a2004>] acpi_pci_irq_enable+0xa4/0x1f0
[    1.348693] [<9000000000d1583c>] pcibios_dev_init+0xac/0xd8
[    1.348695] [<9000000000d15954>] pcibios_enable_device+0xec/0x130
[    1.348698] [<90000000008376d8>] do_pci_enable_device+0x58/0x110
[    1.348701] [<9000000000839a38>] pci_enable_device_flags+0xf0/0x150
[    1.348704] [<900000000084db90>] pcie_port_device_register+0x38/0x520
[    1.348705] [<900000000084e3d4>] pcie_portdrv_probe+0x5c/0xe8
[    1.348708] [<900000000083c958>] local_pci_probe+0x48/0xe0
[    1.348710] [<900000000024cff4>] work_for_cpu_fn+0x1c/0x30
[    1.348712] [<9000000000250778>] process_one_work+0x210/0x418
[    1.348714] [<9000000000250cb8>] worker_thread+0x338/0x5e0
[    1.348716] [<90000000002578fc>] kthread+0x124/0x128
[    1.348719] [<9000000000203cc8>] ret_from_kernel_thread+0xc/0x10
[    1.348721] pch-pic: huxueshi:pch_pic_alloc 33 33
[    1.348724] __irq_set_handler 33
[    1.348726] __irq_set_handler 33
[    1.348728] irq: irq_domain_set_mapping 33 33
[    1.348730] irq: irq_domain_set_mapping 33 33
```

```c
[    1.879533] Call Trace:
[    1.879535] [<900000000020864c>] show_stack+0x2c/0x100
[    1.879538] [<9000000000ec3948>] dump_stack+0x90/0xc0
[    1.879542] [<9000000000beb618>] usb_add_hcd+0x488/0x7f0
[    1.879545] [<9000000000bfffec>] usb_hcd_pci_probe+0x33c/0x388
[    1.879547] [<900000000083c910>] local_pci_probe+0x48/0xe0
[    1.879549] [<900000000024cff4>] work_for_cpu_fn+0x1c/0x30
[    1.879551] [<9000000000250778>] process_one_work+0x210/0x418
[    1.879552] [<9000000000250cb8>] worker_thread+0x338/0x5e0
[    1.879555] [<90000000002578fc>] kthread+0x124/0x128
[    1.879556] [<9000000000203cc8>] ret_from_kernel_thread+0xc/0x10
```


`register_pch_pic(0, LS7A_PCH_REG_BASE, LOONGSON_PCH_IRQ_BASE);`


一些特殊的原因，这两个分配总是同时进行的:
```c

static int extioi_domain_alloc(struct irq_domain *domain, unsigned int virq,
				unsigned int nr_irqs, void *arg)
{
	unsigned int type, i;
	unsigned long hwirq = 0;
	struct extioi *priv = domain->host_data;

	extioi_domain_translate(domain, arg, &hwirq, &type);
	for (i = 0; i < nr_irqs; i++) {
		irq_domain_set_info(domain, virq + i, hwirq + i, &extioi_irq_chip,
					priv, handle_edge_irq, NULL, NULL);
	}

	return 0;
}

static int pch_pic_alloc(struct irq_domain *domain, unsigned int virq,
				unsigned int nr_irqs, void *arg)
{
	int err;
	unsigned int type;
	struct irq_fwspec fwspec;
	struct pch_pic *priv = domain->host_data;
	unsigned long hwirq = 0;

	pch_pic_domain_translate(domain, arg, &hwirq, &type);

	fwspec.fwnode = domain->parent->fwnode;
	fwspec.param_count = 1;
	fwspec.param[0] = hwirq;
  pr_info("huxueshi:%s %d %ld\n", __FUNCTION__, virq, hwirq);
	err = irq_domain_alloc_irqs_parent(domain, virq, 1, &fwspec);
	if (err)
		return err;
	irq_domain_set_info(domain, virq, hwirq,
				&pch_pic_irq_chip, priv,
				handle_level_irq, NULL, NULL);
	irq_set_noprobe(virq);

	return 0;
}
```
在 `extioi_vec_init` 中，
`irq_set_chained_handler_and_data(LOONGSON_BRIDGE_IRQ, extioi_irq_dispatch, extioi_priv);` 
的参数 parent_irq = LOONGSON_BRIDGE_IRQ, 这个 handler 就是注册到了 extioi_irq_dispatch 上了

注意，LOONGSON_BRIDGE_IRQ = 50 + 3
```c
#define LOONGARCH_CPU_IRQ_TOTAL 	14
#define LOONGARCH_CPU_IRQ_BASE 		50
#define LOONGSON_LINTC_IRQ   		(LOONGARCH_CPU_IRQ_BASE + 2) /* IP2 for CPU legacy I/O interrupt controller */
#define LOONGSON_BRIDGE_IRQ 		(LOONGARCH_CPU_IRQ_BASE + 3) /* IP3 for bridge */
```

应该是最开始的位置，这个设置应该也是没有任何意义的:
```c
static int loongarch_cpu_intc_map(struct irq_domain *d, unsigned int irq,
			     irq_hw_number_t hw)
{
	struct irq_chip *chip;

	chip = &loongarch_cpu_irq_controller;

	if (cpu_has_vint)
		set_vi_handler(EXCCODE_INT_START + hw, plat_irq_dispatch);

	irq_set_chip_and_handler(irq, chip, handle_percpu_irq);

	return 0;
}
```


- `__loongarch_cpu_irq_init`
  - irq_domain_add_legacy
    - irq_domain_associate_many
      - `domain->ops->map` : 如果有的话就会加以执行
      - irq_domain_set_mapping

## LOONGSON_LINTC_IRQ
所以，legacy 的中断控制器到底是在被谁使用的 ?

## 递归向下的调用
找到第三个 irq 然后注册到
```c
set_vi_handler(EXCCODE_IP1, ip3_irqdispatch);

static void ip2_irqdispatch(void)
{
	do_IRQ(LOONGSON_LINTC_IRQ); // 52
}

static void ip3_irqdispatch(void)
{
	do_IRQ(LOONGSON_BRIDGE_IRQ); // 这里查找的 irq 是 53 好吧
}
```

- [x] 这里应该永远不会被调用? 既然所有的 interrupt 都是从 ip2 和 ip3 的位置开始的
```c
asmlinkage void __weak plat_irq_dispatch(void)
{
	unsigned long pending = csr_readl(LOONGARCH_CSR_ECFG)
		& csr_readl(LOONGARCH_CSR_ESTAT) & ECFG0_IM;
	unsigned int virq;
	int irq;

	if (!pending) {
		spurious_interrupt();
		return;
	}

	while (pending) {
		irq = fls(pending) - 1;
    // FIXME 如果打开了 ipi，ipi_domain 是没有初始化的，立刻就存在 dump
		if (IS_ENABLED(CONFIG_GENERIC_IRQ_IPI) && irq < 2)
			virq = irq_linear_revmap(ipi_domain, irq);
		else
			virq = irq_linear_revmap(irq_domain, irq); // 使用 irq 作为参数
		do_IRQ(virq);
		pending &= ~BIT(irq);
	}
}
```

```c
static void irq_domain_set_mapping(struct irq_domain *domain,
				   irq_hw_number_t hwirq,
				   struct irq_data *irq_data)
{
	if (hwirq < domain->revmap_size) {
    pr_info("%s %ld %d\n", __FUNCTION__, hwirq, irq_data->irq);
		domain->linear_revmap[hwirq] = irq_data->irq;
	} else {
		mutex_lock(&domain->revmap_tree_mutex);
		radix_tree_insert(&domain->revmap_tree, hwirq, irq_data);
		mutex_unlock(&domain->revmap_tree_mutex);
	}
}
```

从 extioi 的入口就是这里的了:
```c
static void extioi_irq_dispatch(struct irq_desc *desc)
{
	int i;
	u64 pending;
	bool handled = false;
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct extioi *priv = irq_desc_get_handler_data(desc);
	chained_irq_enter(chip, desc);

	for (i = 0; i < VEC_REG_COUNT; i++) {
		pending = iocsr_readq(LOONGARCH_IOCSR_EXTIOI_ISR_BASE + (i << 3));
		/* Do not write ISR register since it is zero already */
		if (pending == 0)
			continue;
		iocsr_writeq(pending, LOONGARCH_IOCSR_EXTIOI_ISR_BASE + (i << 3));
		while (pending) {
			int bit = __ffs(pending);
			int virq = irq_linear_revmap(priv->extioi_domain,
					bit + VEC_COUNT_PER_REG * i);
			if (virq > 0) generic_handle_irq(virq);
			pending &= ~BIT(bit);
			handled = true;
		}
	}

	if (!handled)
		spurious_interrupt();

	chained_irq_exit(chip, desc);
}
```
从这里，所有的编号都是从 0 开始的，查找  extioi_domain::linear_revmap 来得到 linux irq:

在这里 hwirq 到 virq 的装换就是之前注册的。


- [x] 忽然感觉，其实，都是直接链接了，所以，根本不存在到 pch 的哪一个
- 并不是，至少 pch_pic_ack_irq 在被疯狂的调用的


## 观察 pch_pic_ack_irq 的 dump_stack
```c
  /*
    [    2.318621] Call Trace:
    [    2.318623] [<900000000020864c>] show_stack+0x2c/0x100
    [    2.318625] [<9000000000ec3968>] dump_stack+0x90/0xc0
    [    2.318626] [<900000000081f5cc>] pch_pic_ack_irq+0x30/0x84
    [    2.318628] [<900000000029ac64>] handle_level_irq+0x144/0x188
    [    2.318630] [<90000000002952a4>] generic_handle_irq+0x24/0x40
    [    2.318631] [<900000000081dc28>] extioi_irq_dispatch+0x178/0x210
    [    2.318633] [<90000000002952a4>] generic_handle_irq+0x24/0x40
    [    2.318637] [<9000000000203fdc>] except_vec_vi_end+0x94/0xb8
    [    2.318639] [<9000000000203e60>] __cpu_wait+0x20/0x24
   */
```
中断，的确是首先经过 extioi
- [x] 到底是 level 还是 edge ? 是 level 也就是 pch 的

```c
/*
[    2.739387] Call Trace:
[    2.739389] [<900000000020864c>] show_stack+0x2c/0x100
[    2.739391] [<9000000000ec3988>] dump_stack+0x90/0xc0
[    2.739393] [<9000000000bed114>] usb_hcd_irq+0x14/0x50
[    2.739395] [<9000000000296430>] __handle_irq_event_percpu+0x70/0x1b8
[    2.739396] [<9000000000296598>] handle_irq_event_percpu+0x20/0x88
[    2.739398] [<9000000000296644>] handle_irq_event+0x44/0xa8
[    2.739399] [<900000000029abfc>] handle_level_irq+0xdc/0x188
[    2.739402] [<90000000002952a4>] generic_handle_irq+0x24/0x40
[    2.739403] [<900000000081dc28>] extioi_irq_dispatch+0x178/0x210
[    2.739405] [<90000000002952a4>] generic_handle_irq+0x24/0x40
[    2.739407] [<9000000000ee4e78>] do_IRQ+0x18/0x28
[    2.739409] [<9000000000203fdc>] except_vec_vi_end+0x94/0xb8
[    2.798375] input: SONiX USB Keyboard System Control as /devices/pci0000:00/0000:00:05.0/usb4/4-2/4-2:1.1/0003:0C45:760B.0002/input/input3
```


- [ ] 这些设备的探测功能是需要我来完成吗 ?
  - ramooflax 的做法?
  - 找一个设备分析一下吧

## for 循环的 mapping
- [ ] 从 virq 的分配上，应该可以找到标准入口吧!

`__irq_set_handler` 就是设置的 virq 对应的 handler

这两个循环，占据了
- 50 - 64 : `__loongarch_cpu_irq_init`
- 0 - 15 : `pch_lpc_init`
- 16 : 是为了将 pch_lpc 控制芯片挂载到 pch_ipc 上


```
[    0.000000] irq: irq_domain_associate_many(<no-node>, irqbase=0, hwbase=0, count=16)
[    0.000000] __irq_set_handler 0
[    0.000000] irq: irq_domain_set_mapping 0 0
[    0.000000] __irq_set_handler 1
[    0.000000] irq: irq_domain_set_mapping 1 1
[    0.000000] __irq_set_handler 2
[    0.000000] irq: irq_domain_set_mapping 2 2
[    0.000000] __irq_set_handler 3
[    0.000000] irq: irq_domain_set_mapping 3 3
[    0.000000] __irq_set_handler 4
[    0.000000] irq: irq_domain_set_mapping 4 4
[    0.000000] __irq_set_handler 5
[    0.000000] irq: irq_domain_set_mapping 5 5
[    0.000000] __irq_set_handler 6
[    0.000000] irq: irq_domain_set_mapping 6 6
[    0.000000] __irq_set_handler 7
[    0.000000] irq: irq_domain_set_mapping 7 7
[    0.000000] __irq_set_handler 8
[    0.000000] irq: irq_domain_set_mapping 8 8
[    0.000000] __irq_set_handler 9
[    0.000000] irq: irq_domain_set_mapping 9 9
[    0.000000] __irq_set_handler 10
[    0.000000] irq: irq_domain_set_mapping 10 10
[    0.000000] __irq_set_handler 11
[    0.000000] irq: irq_domain_set_mapping 11 11
[    0.000000] __irq_set_handler 12
[    0.000000] irq: irq_domain_set_mapping 12 12
[    0.000000] __irq_set_handler 13
[    0.000000] irq: irq_domain_set_mapping 13 13
[    0.000000] __irq_set_handler 14
[    0.000000] irq: irq_domain_set_mapping 14 14
[    0.000000] __irq_set_handler 15
[    0.000000] irq: irq_domain_set_mapping 15 15
[    0.000000] CPU: 0 PID: 0 Comm: swapper/0 Not tainted 4.19.167 #18
[    0.000000] Hardware name: Loongson Loongson-LS3A5000-7A1000-1w-V0.1-CRB/Loongson-LS3A5000-7A1000-1w-EVB-V1.21, BIOS Loongson-UDK2018-V2.0.04073-beta2 03/
[    0.000000] Stack : 00000000000000aa 9000000000ec3948 900000000123c000 900000000123f9f0
[    0.000000]         0000000000000000 900000000123f9f0 0000000000000000 9000000001369b48
[    0.000000]         9000000001369b40 0000000000000000 9000000001359302 000000000000008e
[    0.000000]         0000000000000000 9000000001261298 0000000000000001 00000000000000aa
[    0.000000]         900000047aef8000 0000000000000000 0000000000000000 0000000000000001
[    0.000000]         00000000000000aa 0000000476cd0000 9000000476010940 0000000000000000
[    0.000000]         00000000000000b0 0000000000000000 0000000000000001 9000000476010780
[    0.000000]         0000000000000001 90000000011cccf8 0000000000000001 0000000000000000
[    0.000000]         0000000000000000 0000000000000000 0000000000000000 0000000000000000
[    0.000000]         0000000000000000 900000000020864c 0000000000000000 00000000000000b0
[    0.000000]         ...
[    0.000000] Call Trace:
[    0.000000] [<900000000020864c>] show_stack+0x2c/0x100
[    0.000000] [<9000000000ec3948>] dump_stack+0x90/0xc0
[    0.000000] [<900000000081f550>] pch_pic_alloc+0x48/0xf8
[    0.000000] [<90000000002a0648>] __irq_domain_alloc_irqs+0x1b0/0x2e0
[    0.000000] [<90000000002a0bf0>] irq_create_fwspec_mapping+0x258/0x368
[    0.000000] [<900000000081e7c0>] pch_lpc_init+0xe8/0x178
[    0.000000] [<900000000081f33c>] pch_pic_init+0x18c/0x358
[    0.000000] [<90000000012d8424>] setup_IRQ+0xf4/0x110
[    0.000000] [<90000000012d845c>] arch_init_irq+0x1c/0x98
[    0.000000] [<90000000012d9cbc>] init_IRQ+0x4c/0xd4
[    0.000000] [<90000000012d4908>] start_kernel+0x274/0x454
[    0.000000] pch-pic: huxueshi:pch_pic_alloc 16 19
[    0.000000] __irq_set_handler 16
[    0.000000] __irq_set_handler 16
[    0.000000] irq: irq_domain_set_mapping 19 16
[    0.000000] irq: irq_domain_set_mapping 19 16
```

## 动态的 mapping
```
[    0.846098] pch-pic: huxueshi:pch_pic_alloc 18 47
[    0.846099] extioi: huxueshi:extioi_domain_alloc 18 47
[    0.846100] __irq_set_handler 18
[    0.846102] __irq_set_handler 18
[    0.846104] irq: irq_domain_set_mapping 47 18
[    0.846105] irq: irq_domain_set_mapping 47 18
```

最根本的:
```c
/*
[    0.846127] Call Trace:
[    0.846130] [<900000000020864c>] show_stack+0x2c/0x100
[    0.846133] [<9000000000ec3968>] dump_stack+0x90/0xc0
[    0.846136] [<900000000029f8e0>] irq_domain_set_mapping+0x90/0xb8
[    0.846138] [<90000000002a0698>] __irq_domain_alloc_irqs+0x200/0x2e0
[    0.846141] [<90000000002a0bf0>] irq_create_fwspec_mapping+0x258/0x368
[    0.846143] [<900000000020f3d0>] acpi_register_gsi+0x70/0x100
[    0.846145] [<900000000020f750>] acpi_gsi_to_irq+0x28/0x48
[    0.846148] [<9000000000890044>] acpi_os_install_interrupt_handler+0x84/0x148
[    0.846151] [<90000000008ac830>] acpi_ev_install_xrupt_handlers+0x24/0x98
[    0.846155] [<90000000012f7a14>] acpi_init+0xc0/0x338
[    0.846157] [<90000000002004fc>] do_one_initcall+0x6c/0x170
[    0.846159] [<90000000012d4ce0>] kernel_init_freeable+0x1f8/0x2b8
[    0.846162] [<9000000000eda774>] kernel_init+0x10/0xf4
[    0.846164] [<9000000000203cc8>] ret_from_kernel_thread+0xc/0x10
```

一种情况是因为 `irq_domain_set_mapping` => irq_domain_associate_many

```c
/*
[    0.000000] Call Trace:
[    0.000000] [<900000000020864c>] show_stack+0x2c/0x100
[    0.000000] [<9000000000ec3968>] dump_stack+0x90/0xc0
[    0.000000] [<900000000029f8e0>] irq_domain_set_mapping+0x90/0xb8
[    0.000000] [<900000000029f9ac>] irq_domain_associate+0xa4/0x1d8
[    0.000000] [<90000000002a0eec>] irq_domain_associate_many+0x9c/0xc8
[    0.000000] [<900000000029fc34>] irq_domain_add_legacy+0x7c/0x80
[    0.000000] [<90000000012f3ce4>] __loongarch_cpu_irq_init+0x94/0xc8
[    0.000000] [<90000000012d8344>] setup_IRQ+0x14/0x110
[    0.000000] [<90000000012d845c>] arch_init_irq+0x1c/0x98
[    0.000000] [<90000000012d9cbc>] init_IRQ+0x4c/0xd4
[    0.000000] [<90000000012d4908>] start_kernel+0x274/0x454
```

另一种情况是因为
```c
/*
[    0.000000] [<900000000020864c>] show_stack+0x2c/0x100
[    0.000000] [<9000000000ec3968>] dump_stack+0x90/0xc0
[    0.000000] [<900000000029f8e0>] irq_domain_set_mapping+0x90/0xb8
[    0.000000] [<900000000029f9ac>] irq_domain_associate+0xa4/0x1d8
[    0.000000] [<90000000002a0eec>] irq_domain_associate_many+0x9c/0xc8
[    0.000000] [<900000000029fc34>] irq_domain_add_legacy+0x7c/0x80
[    0.000000] [<900000000081e770>] pch_lpc_init+0x88/0x178
[    0.000000] [<900000000081f1f4>] pch_pic_init+0x18c/0x358
[    0.000000] [<90000000012d8424>] setup_IRQ+0xf4/0x110
[    0.000000] [<90000000012d845c>] arch_init_irq+0x1c/0x98
[    0.000000] [<90000000012d9cbc>] init_IRQ+0x4c/0xd4
[    0.000000] [<90000000012d4908>] start_kernel+0x274/0x454
```
