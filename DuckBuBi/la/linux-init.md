# 想法
- [ ] 如果想要支持键盘，那么最小量的代码是什么 ?

# Find out
代码主要出现的地方:
1. 内核

ptrace.c                                      96             51            600
signal.c                                     120             96            594
fpu.S                                         56            130            587

2. la64
3. mem

question:
- [ ] 内存的如何收集的
- [ ] dtb
- [ ] relocate.c 是怎么处理的?
- [ ] /sys/firmware
- [ ] https://wiki.gentoo.org/wiki/IOMMU_SWIOTLB
- [ ] /home/maritns3/core/loongson-dune/la-4.19/arch/loongarch/la64/env.c : BIOS 参数解析
- [ ] /home/maritns3/core/loongson-dune/la-4.19/arch/loongarch/la64/init.c : PROM ???
- [ ] /home/maritns3/core/loongson-dune/la-4.19/arch/loongarch/la64/irq.c : 存在一些 vi 中断等东西
- [ ] MSI 是怎么处理的 ?

```
[    0.000000] PCH_PIC[0]: pch_pic_id 0, version 0, address 0xe0010000000, IRQ 64-127

[    0.000000] irq: Added domain unknown-1
[    0.000000] irq: irq_domain_associate_many(<no-node>, irqbase=50, hwbase=0, count=14)

[    0.000000] irq: Added domain irqchip@(____ptrval____) 

[    0.000000] Support EXT interrupt.
[    0.000000] irq: Added domain irqchip@(____ptrval____)

[    0.000000] pch-msi: Registering 192 MSIs, starting at 64 // 创建出来两个
[    0.000000] irq: Added domain irqchip@(____ptrval____)
[    0.000000] irq: Added domain irqchip@(____ptrval____)

[    0.000000] huxueshi : of_setup_pch_irqs
[    0.000000] 0
[    0.000000] irq: Added domain irqchip@(____ptrval____)
[    0.000000] irq: Added domain unknown-2
[    0.000000] irq: irq_domain_associate_many(<no-node>, irqbase=0, hwbase=0, count=16)
```


- [x] 在 irq_set_chained_handler_and_data 中，虽然给将 3 号注册给了 extioi_irq_dispatch 了，但是 3 号的 irq_desc 是初始化就是那个时候完成的

在 liointc_init 中，其 parent irq 是，这应该说明，这个东西本身是挂载到什么东西上的:
```c
static int parent_irq[LIOINTC_NUM_PARENT] = {LOONGSON_LINTC_IRQ, LOONGSON_BRIDGE_IRQ};
```

实际上，irq_domain_ops::map 中, 
- loongarch_cpu_intc_map
  - plat_irq_dispatch : 默认的 int 入口这个
    - 会根据 irq_domain 找到 hwriq 对应的 irq ，已经进一步的 irq_desc
- handle_percpu_irq : 设置 irq 对应的 desc 以及 handler, 这里是 handle_percpu_irq

of_setup_pch_irqs 挂载到 parent 靠 irq_find_matching_fwnode 实现


# 问题
- [ ] bt tree 如何读去的 ?

- [ ] 有没有什么 pcie 上的特殊处理啊 ?
- [ ] ejtag 的使用

- [ ] 多核启动的时候，一个核启动，如何让第二个核启动?
- [ ] dmi / smbios / acpi / efi

- [ ] 忽然发现时钟系统有点麻烦
- [ ] arch/loongarch/kernel/vmlinux.lds.S 是如何被使用的 ?

- [ ] 最开始的时候，内核是被加载什么位置，让其可以在直接映射的地址空间的

- [ ] 应该分析一下  -M ls3a5k 

- [ ] vint 的内容是什么？

- [ ] 似乎中断就是只是两个数值，并不是所有的号码都用上了
- [ ] ipi 是怎么配置的，如果不小心将消息转发出去了，那岂不是很尴尬
- [ ] irq_common_data 来实现 irq 的 affinility
- [ ] 从这个函数的计算 set_vi_handler, 分析一下 vectored interrupt 是干什么的?

- ip2_irqdispatch
  - do_IRQ
    - generic_handle_irq
      - generic_handle_irq_desc
        - irq_desc::handle_irq

- extioi_irq_dispatch : 本身就是在中断上下文中间，现在在做下一级的跳转, TODO 问题是，怎么知道是从上一级跳转到下
  - irq_linear_revmap : 通过 iocsr_writeq 可以获取物理上的中断
  - generic_handle_irq 

- [ ] 到底存在那些 irq domain

- [ ] 真正让人恐惧的是， acpi_bus_init_irq 之类的初始化实际上是在 rest_init 后面

## TODO
- [ ] 中断系统初始化
- [ ] 参数解析到底怎么回事
  - prom_init_cmdline
  - setup_command_line

## notes
- 设备树信息可能被编译到内核中间，可能在通过 bios 传递过来的
- extioi_vec_init 中描述，所有的外部中断都是经过 extioi 进行的
