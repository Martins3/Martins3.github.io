## MSI
Generic MSIs : https://en.wikipedia.org/wiki/Message_Signaled_Interrupts

linux/drivers/pci/msi.c

use nvme as example:
- nvme_pci_enable
  - pci_alloc_irq_vectors
    - pci_alloc_irq_vectors_affinity
      - `__pci_enable_msix_range`
        - `__pci_enable_msix`
          - **msix_capability_init** : [^3] page 130

[ARM](https://elinux.org/images/8/8c/Zyngier.pdf)

# kernel/irq
> 缺乏相关的背景内容，根本没有办法动笔!

## todo
1. 以 x86 为例，想知道从 architecture 的 entry.S 触发 到指向对应的 handler 的过程是怎样的 ?
2. 中断控制器很简单，其大概是怎么实现的 ?
    1. CPU 提供给 interrupt controller 的接口是什么 ?
        1. 我(CPU) TM 怎么知道这一个信号是来自于 interrupt controller 的 ?
        2. 是不是 CPU 专门为其提供了引脚，各种 CPU 的引脚的数量是什么 ?
        3. IC 的输入输出的引脚的数量是多少 ?
    2. 当 IC 可以进行层次架构之后，

3. 为了多核，是不是也是需要进一步修改 IC 来支持。
    1. affinity 提供硬件支持

## wowotech 相关的内容

1. 中断描述符中应该会包括底层 irq chip 相关的数据结构，linux kernel 中把这些数据组织在一起，形成`struct irq_data`
2. 中断有两种形态，一种就是直接通过 signal 相连，用电平或者边缘触发。另外一种是基于消息的，被称为 MSI (Message Signaled Interrupts)。
3. Interrupt controller 描述符（struct irq_chip）包括了若干和具体 Interrupt controller 相关的`callback`函数

## irq domain

## 所以 msi 是怎么路由的
- [x] msi_domain_alloc 为什么会去调用 extioi_domain_alloc
  - 因为 msi domain 的 parent 是 extioi
- [x] msi domain 是什么时候创建的
  - msi_create_irq_domain

- [x] 从下面的函数中，其中的 arg 中告诉了 hwirq，什么时候装载的
```c
#0  extioi_domain_alloc (domain=0x900000027c024800, virq=22, nr_irqs=1, arg=0x900000027cd338f0) at drivers/irqchip/irq-loongarch-extioi.c:303
#1  0x900000000028d8cc in msi_domain_alloc (domain=0x900000027c08c600, virq=22, nr_irqs=<optimized out>, arg=0x900000027cd338f0) at kernel/irq/msi.c:150
#2  0x9000000000f70464 in irq_domain_alloc_irqs_hierarchy (arg=<optimized out>, nr_irqs=<optimized out>, irq_base=<optimized out>, domain=<optimized out>) at kernel/irq/irqdomain.c:1271
#3  __irq_domain_alloc_irqs (domain=0x900000027c08c600, irq_base=22, nr_irqs=1, node=<optimized out>, arg=0x900000027cd338f0, realloc=192, affinity=<optimized out>) atkernel/irq/irqdomain.c:1328
#4  0x900000000028e11c in msi_domain_alloc_irqs (domain=0x900000027c08c600, dev=0x900000027d4140a8, nvec=<optimized out>) at ./include/linux/device.h:1075
#5  0x90000000008e156c in msix_capability_init (affd=<optimized out>, nvec=<optimized out>, entries=<optimized out>, dev=<optimized out>) at drivers/pci/msi.c:759
#6  __pci_enable_msix (affd=<optimized out>, nvec=<optimized out>, entries=<optimized out>, dev=<optimized out>) at drivers/pci/msi.c:967
#7  __pci_enable_msix_range (affd=<optimized out>, maxvec=<optimized out>, minvec=<optimized out>, entries=<optimized out>, dev=<optimized out>) at drivers/pci/msi.c:1100
#8  __pci_enable_msix_range (dev=0x900000027d414000, entries=0x0, minvec=2, maxvec=2, affd=0x900000027cd33b98) at drivers/pci/msi.c:1081
#9  0x90000000008e1fc4 in pci_alloc_irq_vectors_affinity (dev=0x900000027d414000, min_vecs=2, max_vecs=2, flags=12, affd=0x900000027cd33b98) at drivers/pci/msi.c:1170
#10 0x9000000000963384 in vp_request_msix_vectors (desc=<optimized out>, per_vq_vectors=<optimized out>, nvectors=<optimized out>, vdev=<optimized out>) at drivers/virtio/virtio_pci_common.c:136
#11 vp_find_vqs_msix (vdev=0x900000027dafe800, nvqs=1, vqs=<optimized out>, callbacks=0x900000027d234780, names=0x900000027d234740, per_vq_vectors=true, ctx=0x0, desc=0x900000027cd33b98) at drivers/virtio/virtio_pci_common.c:307
#12 0x9000000000963748 in vp_find_vqs (vdev=0x900000027dafe800, nvqs=1, vqs=0x900000027d2347c0, callbacks=0x900000027d234780, names=0x900000027d234740, ctx=0x0, desc=0x900000027cd33b98) at drivers/virtio/virtio_pci_common.c:403
#13 0x900000000096221c in vp_modern_find_vqs (vdev=0x900000027dafe800, nvqs=<optimized out>, vqs=<optimized out>, callbacks=<optimized out>, names=<optimized out>, ctx=<optimized out>, desc=<optimized out>) at drivers/virtio/virtio_pci_modern.c:413
#14 0x9000000000a51ac8 in virtio_find_vqs (desc=<optimized out>, names=<optimized out>, callbacks=<optimized out>, vqs=<optimized out>, nvqs=<optimized out>, vdev=<optimized out>) at ./include/linux/virtio_config.h:192
#15 init_vq (vblk=0x900000027d5a5800) at drivers/block/virtio_blk.c:542
#16 0x9000000000a52b7c in virtblk_probe (vdev=0x900000027c024800) at drivers/block/virtio_blk.c:774
#17 0x900000000095f4c0 in virtio_dev_probe (_d=0x900000027dafe810) at drivers/virtio/virtio.c:245
#18 0x9000000000a29208 in really_probe (dev=0x900000027dafe810, drv=0x90000000014359c0 <virtio_blk>) at drivers/base/dd.c:506
#19 0x9000000000a29440 in driver_probe_device (drv=0x90000000014359c0 <virtio_blk>, dev=0x900000027dafe810) at drivers/base/dd.c:667
#20 0x9000000000a295e8 in __driver_attach (data=<optimized out>, dev=<optimized out>) at drivers/base/dd.c:903
#21 __driver_attach (dev=0x900000027dafe810, data=0x90000000014359c0 <virtio_blk>) at drivers/base/dd.c:872
#22 0x9000000000a26f3c in bus_for_each_dev (bus=<optimized out>, start=<optimized out>, data=0x1, fn=0x900000027cd338f0) at drivers/base/bus.c:279
#23 0x9000000000a28a4c in driver_attach (drv=<optimized out>) at drivers/base/dd.c:922
#24 0x9000000000a284b8 in bus_add_driver (drv=0x90000000014359c0 <virtio_blk>) at drivers/base/bus.c:672
#25 0x9000000000a2a22c in driver_register (drv=0x90000000014359c0 <virtio_blk>) at drivers/base/driver.c:170
#26 0x900000000095ee9c in register_virtio_driver (driver=<optimized out>) at drivers/virtio/virtio.c:296
#27 0x90000000014ee768 in init () at drivers/block/virtio_blk.c:1019
#28 0x9000000000200b8c in do_one_initcall (fn=0x90000000014ee6f0 <init>) at init/main.c:884
#29 0x90000000014a4e8c in do_initcall_level (level=<optimized out>) at ./include/linux/init.h:131
#30 do_initcalls () at init/main.c:960
#31 do_basic_setup () at init/main.c:978
#32 kernel_init_freeable () at init/main.c:1145
#33 0x9000000000f790d8 in kernel_init (unused=<optimized out>) at init/main.c:1062
#34 0x900000000020316c in ret_from_kernel_thread () at arch/loongarch/kernel/entry.S:85
Backtrace stopped: frame did not save the PC
```

通过:
```c
#0  pci_msi_domain_set_desc (arg=0x900000027cd338f0, desc=0x900000027d266a00) at drivers/irqchip/irq-loongson-pch-msi.c:113
#1  0x900000000028e0fc in msi_domain_alloc_irqs (domain=0x900000027c08c600, dev=0x900000027d5210a8, nvec=<optimized out>) at kernel/irq/msi.c:415
```
通过 pci_msi_domain_set_desc 分配出来了 32 和 33
```c
static void pci_msi_domain_set_desc(msi_alloc_info_t *arg,
            struct msi_desc *desc)
{
  struct pch_msi_data *priv;
  struct msi_domain_info *info = (struct msi_domain_info *)(*(u64 *)&arg->param[IRQ_DOMAIN_IRQ_SPEC_PARAMS - 2]);
  priv = (struct pch_msi_data *)info->data;

  arg->param_count = 1;
  arg->param[0] = pch_msi_allocate_hwirq(desc->nvec_used, priv);
  pr_info("huxueshi:%s %d\n", __FUNCTION__, arg->param[0]);
}
```
- [x] 验证一下，32 和 33 是 extioi 的两个中断号
  - 可以通过 extioi_domain_alloc 很容易的验证

## msi domain 的创建过程
```c
#0  msi_create_irq_domain (fwnode=0x900000027c011480, info=0x90000000015f1e28 <pch_msi_domain_info>, parent=0x900000027c024800) at kernel/irq/msi.c:287
#1  0x90000000008e0b44 in pci_msi_create_irq_domain (fwnode=0x900000027c011480, info=0x90000000015f1e28 <pch_msi_domain_info>, parent=<optimized out>) at drivers/pci/msi.c:1459
#2  0x90000000008a355c in pch_msi_init_domain (id=<optimized out>, priv=<optimized out>, ext=<optimized out>, parent_handle=<optimized out>) at drivers/irqchip/irq-loongson-pch-msi.c:154
#3  pch_msi_init (irq_handle=0x900000027c011480, parent_handle=0x900000027c011300, msg_address=<optimized out>, ext=true, start=32, count=<optimized out>) at drivers/irqchip/irq-loongson-pch-msi.c:190
#4  0x9000000000f6e360 in pch_msi_domain_init (count=<optimized out>, start=<optimized out>) at arch/loongarch/la64/irq.c:184
#5  irqchip_init_default () at arch/loongarch/la64/irq.c:284
#6  0x90000000014a8e78 in setup_IRQ () at arch/loongarch/la64/irq.c:311
#7  0x90000000014a8ea4 in arch_init_irq () at arch/loongarch/la64/irq.c:360
#8  0x90000000014aa708 in init_IRQ () at arch/loongarch/kernel/irq.c:59
#9  0x90000000014a4a40 in start_kernel () at init/main.c:636
#10 0x9000000000f78fd4 in kernel_entry () at arch/loongarch/kernel/head.S:129
Backtrace stopped: frame did not save the PC
```
