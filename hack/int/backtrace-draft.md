# Trace x86 interrupt

- [ ] vector_irq 实际上是一个 percpu 的，从 ioapic 如何部署到具体的 cpu 上
- [ ] 目前知道的三个控制器，lapic, ioapic 和 msi 应该都是存在对应的 chip_operation 的才对啊

## /proc/interrupts
```txt
           CPU0
  0:         28   IO-APIC   2-edge      timer
  1:        224   IO-APIC   1-edge      i8042
  4:        323   IO-APIC   4-edge      ttyS0
  6:          2   IO-APIC   6-edge      floppy
  8:         55   IO-APIC   8-edge      rtc0
  9:          0   IO-APIC   9-fasteoi   acpi
 10:         62   IO-APIC  10-fasteoi   eth0
 12:        125   IO-APIC  12-edge      i8042
 14:       1077   IO-APIC  14-edge      ata_piix
 15:         34   IO-APIC  15-edge      ata_piix
 24:          6   PCI-MSI 65536-edge      nvme0q0
 25:         42   PCI-MSI 65537-edge      nvme0q1
 26:          0   PCI-MSI 81920-edge      virtio1-config
 27:         10   PCI-MSI 81921-edge      virtio1-requests
NMI:          0   Non-maskable interrupts
LOC:       3131   Local timer interrupts
SPU:          0   Spurious interrupts
PMI:          0   Performance monitoring interrupts
IWI:          0   IRQ work interrupts
RTR:          0   APIC ICR read retries
RES:          0   Rescheduling interrupts
CAL:          0   Function call interrupts
TLB:          0   TLB shootdowns
TRM:          0   Thermal event interrupts
THR:          0   Threshold APIC interrupts
DFR:          0   Deferred Error APIC interrupts
MCE:          0   Machine check exceptions
MCP:          1   Machine check polls
ERR:          0
MIS:          0
PIN:          0   Posted-interrupt notification event
NPI:          0   Nested posted-interrupt event
PIW:          0   Posted-interrupt wakeup event
```

## msi
根据 https://habr.com/ru/post/501660/ 告诉如何关闭 pci 的 msi，默认情况下，nvme 就是使用 msi 的

https://www.kernel.org/doc/html/latest/PCI/msi-howto.html : 最详细的文档了

- [ ] 应该 msi 应该是可以直接绕过 ioapic 的，msi 是 pcie 的基础功能，msi 功能是不能模拟的

- [ ] pci_alloc_irq_vectors 申请 pci 的入口，为什么最后到达了 msi 的，看漏了其他的入口

- [ ] 键盘等各种 virq 其实是 acpi 决定的，那么，当 nvme 分配的 virq=24 是靠什么让
CPU 知道当 24 号引脚拉高的时候是 msi 中断啊, 从原则上，这是物理决定的才对啊
  - 应该是就是 cpu 了
  - [ ] 感觉已经话费了很长时间了，但是 `__irq_msi_compose_msg` 写入的数据实际上和想想的不一样
- [ ] 当 msi disable 掉，那么 nvme 设备如何通过 pci bridge 实现中断通知


- msix_capability_init : 狮子书 P130
  - msix_map_region : 获取 table 的位置
  - msix_setup_entries : 初始化 msix_entry
    - `list_add_tail(&entry->list, dev_to_msi_list(&dev->dev));` : 将新创建的 msix_entry 加入到 entry 中间
  - pci_msi_setup_msi_irqs
  - msi_domain_alloc_irqs
    - `ops->domain_alloc_irqs` => `__msi_domain_alloc_irqs`
      - irq_domain_alloc_descs : 分配 desc，同时分配 virq 也就是 linux irq
      - irq_domain_activate_irq
        - `__irq_domain_activate_irq`
          - `__irq_domain_activate_irq(irqd->parent_data, reserve)` : 会将 parent 调用一下，也就是 vector
            - x86_vector_activate
          - msi_domain_activate
            - `data->chip->irq_write_msi_msg` 的注册者真不少，但是测试显示只有从 nvme 的路径下 pci_msi_domain_write_msg 被调用而已
              - irq_chip_compose_msi_msg
                - irq_chip_compose_msi_msg
                  - `__irq_msi_compose_msg`
              - pci_msi_domain_write_msg
      - arch_setup_msi_irqs : 这是一个空函数
  - msix_program_entries

从 /sys/devices/pci0000:00/0000.00.04.0/msi_irqs 看，分配了 24 25 26 三个


pci_irq_vector : 获取 virq
device::msi_list 中间获取 `msi_desc->irq`
msix_setup_entries 创建了 msi_desc

- nvme_reset_work
  - nvme_pci_enable => x86_vector_alloc_irqs : 分配 irq_desc
  - nvme_pci_configure_admin_queue => apic_update_vector : 设置 irq_desc

- [ ] msi 还是可以设置 cpu affinity 的
  - [ ] 显示是必须的，因为是直接注入到具体的 cpu 的 lapic 中间的，但是
  - [ ] ioapic 应该也是存在这个问题



```c
/**
 * msi_msg - Representation of a MSI message
 * @address_lo:   Low 32 bits of msi message address
 * @arch_addrlo:  Architecture specific shadow of @address_lo
 * @address_hi:   High 32 bits of msi message address
 *      (only used when device supports it)
 * @arch_addrhi:  Architecture specific shadow of @address_hi
 * @data:   MSI message data (usually 16 bits)
 * @arch_data:    Architecture specific shadow of @data
 */
struct msi_msg {
  union {
    u32     address_lo;
    arch_msi_msg_addr_lo_t  arch_addr_lo;
  };
  union {
    u32     address_hi;
    arch_msi_msg_addr_hi_t  arch_addr_hi;
  };
  union {
    u32     data;
    arch_msi_msg_data_t arch_data;
  };
};
```

[MSI 重点参考](https://www.programmersought.com/article/61643546638/)

```txt
➜  vn git:(master) ✗ sudo cat /proc/iomem | grep apic -i
[sudo] password for maritns3:
  fec00000-fec003ff : IOAPIC 0
fee00000-fee00fff : Local APIC
```
`__irq_msi_compose_msg` 中间组装的 Message Address 就正好是 fee00000

## setup
- [x] i8042 之类如何设置的
- [x] nvme 如何设置的

- [x] timer 是如何注册的 ?
timer 的注册在很早的时候:
```c
/*
>>> bt
#0  setup_default_timer_irq () at arch/x86/kernel/time.c:70
#1  hpet_time_init () at arch/x86/kernel/time.c:82
#2  0xffffffff82b52cdb in x86_late_time_init () at arch/x86/kernel/time.c:94
#3  0xffffffff82b4bfcb in start_kernel () at init/main.c:1051
#4  0xffffffff81000107 in secondary_startup_64 () at arch/x86/kernel/head_64.S:283
#5  0x0000000000000000 in ?? ()
```


#### acpi 注册过程
```c
/*
#0  apic_update_vector (irqd=irqd@entry=0xffff88810004e7c0, newvec=newvec@entry=33, newcpu=0) at arch/x86/kernel/apic/vector.c:134
#1  0xffffffff810478f9 in assign_vector_locked (irqd=irqd@entry=0xffff88810004e7c0, dest=dest@entry=0xffffffff82c97540 <vector_searchmask>) at arch/x86/kernel/apic/vect
or.c:252
#2  0xffffffff81047b0b in assign_irq_vector_any_locked (irqd=0xffff88810004e7c0) at arch/x86/kernel/apic/vector.c:279
#3  0xffffffff81047f17 in activate_reserved (irqd=0xffff88810004e7c0) at arch/x86/kernel/apic/vector.c:393
#4  x86_vector_activate (dom=<optimized out>, irqd=0xffff88810004e7c0, reserve=<optimized out>) at arch/x86/kernel/apic/vector.c:462
#5  0xffffffff810c07ee in __irq_domain_activate_irq (irqd=0xffff88810004e7c0, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1718
#6  0xffffffff810c07cd in __irq_domain_activate_irq (irqd=irqd@entry=0xffff8881000f3a28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1715
#7  0xffffffff810c2350 in irq_domain_activate_irq (irq_data=irq_data@entry=0xffff8881000f3a28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1741
#8  0xffffffff810bf68b in irq_activate (desc=desc@entry=0xffff8881000f3a00) at kernel/irq/chip.c:291
#9  0xffffffff810bcfd5 in __setup_irq (irq=irq@entry=9, desc=desc@entry=0xffff8881000f3a00, new=new@entry=0xffff888100222a00) at kernel/irq/manage.c:1678
#10 0xffffffff810bd4e7 in request_threaded_irq (irq=9, handler=handler@entry=0xffffffff81469af0 <acpi_irq>, thread_fn=thread_fn@entry=0x0 <fixed_percpu_data>, irqflags= irqflags@entry=128, devname=devname@entry=0xffffffff82216ea8 "acpi", dev_id=dev_id@entry=0xffffffff81469af0 <acpi_irq>) at kernel/irq/manage.c:2137
#11 0xffffffff81469ef7 in request_irq (dev=0xffffffff81469af0 <acpi_irq>, name=0xffffffff82216ea8 "acpi", flags=128, handler=0xffffffff81469af0 <acpi_irq>, irq=<optimiz ed out>) at ./include/linux/interrupt.h:164
#12 acpi_os_install_interrupt_handler (gsi=9, handler=handler@entry=0xffffffff81485859 <acpi_ev_sci_xrupt_handler>, context=0xffff888100225840) at drivers/acpi/osl.c:58
6
#13 0xffffffff814858a0 in acpi_ev_install_sci_handler () at drivers/acpi/acpica/evsci.c:156
#14 0xffffffff8148301c in acpi_ev_install_xrupt_handlers () at drivers/acpi/acpica/evevent.c:94
#15 0xffffffff82b7ff45 in acpi_enable_subsystem (flags=flags@entry=2) at drivers/acpi/acpica/utxfinit.c:184
#16 0xffffffff82b7e444 in acpi_bus_init () at drivers/acpi/bus.c:1239
#17 acpi_init () at drivers/acpi/bus.c:1333
#18 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b7e3c5 <acpi_init>) at init/main.c:1249
#19 0xffffffff82b4c26b in do_initcall_level (command_line=0xffff888100123400 "root", level=4) at ./include/linux/compiler.h:234
#20 do_initcalls () at init/main.c:1338
#21 do_basic_setup () at init/main.c:1358
#22 kernel_init_freeable () at init/main.c:1560
#23 0xffffffff81b97129 in kernel_init (unused=<optimized out>) at init/main.c:1447
#24 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#25 0x0000000000000000 in ?? ()
```
#### floppy
```c
/*
#0  apic_update_vector (irqd=irqd@entry=0xffff88810004e640, newvec=newvec@entry=34, newcpu=0) at arch/x86/kernel/apic/vector.c:134
#1  0xffffffff810478f9 in assign_vector_locked (irqd=irqd@entry=0xffff88810004e640, dest=dest@entry=0xffffffff82c97540 <vector_searchmask>) at arch/x86/kernel/apic/vect or.c:252
#2  0xffffffff81047b0b in assign_irq_vector_any_locked (irqd=0xffff88810004e640) at arch/x86/kernel/apic/vector.c:279
#3  0xffffffff81047f17 in activate_reserved (irqd=0xffff88810004e640) at arch/x86/kernel/apic/vector.c:393
#4  x86_vector_activate (dom=<optimized out>, irqd=0xffff88810004e640, reserve=<optimized out>) at arch/x86/kernel/apic/vector.c:462
#5  0xffffffff810c07ee in __irq_domain_activate_irq (irqd=0xffff88810004e640, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1718
#6  0xffffffff810c07cd in __irq_domain_activate_irq (irqd=irqd@entry=0xffff8881000f3428, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1715
#7  0xffffffff810c2350 in irq_domain_activate_irq (irq_data=irq_data@entry=0xffff8881000f3428, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1741
#8  0xffffffff810bf68b in irq_activate (desc=desc@entry=0xffff8881000f3400) at kernel/irq/chip.c:291
#9  0xffffffff810bcfd5 in __setup_irq (irq=irq@entry=6, desc=desc@entry=0xffff8881000f3400, new=new@entry=0xffff888100ffc280) at kernel/irq/manage.c:1678
#10 0xffffffff810bd4e7 in request_threaded_irq (irq=6, handler=handler@entry=0xffffffff816ae420 <floppy_hardint>, thread_fn=thread_fn@entry=0x0 <fixed_percpu_data>, irq flags=irqflags@entry=0, devname=devname@entry=0xffffffff822459f2 "floppy", dev_id=dev_id@entry=0x0 <fixed_percpu_data>) at kernel/irq/manage.c:2137
#11 0xffffffff82b888af in request_irq (dev=0x0 <fixed_percpu_data>, name=0xffffffff822459f2 "floppy", flags=0, handler=0xffffffff816ae420 <floppy_hardint>, irq=<optimiz ed out>) at ./include/linux/interrupt.h:164
#12 fd_request_irq () at ./arch/x86/include/asm/floppy.h:147
#13 floppy_grab_irq_and_dma () at drivers/block/floppy.c:4812
#14 do_floppy_init () at drivers/block/floppy.c:4623
#15 floppy_async_init (data=<optimized out>, cookie=<optimized out>) at drivers/block/floppy.c:4741
#16 0xffffffff8108bccb in async_run_entry_fn (work=0xffff888100faf860) at kernel/async.c:127
#17 0xffffffff8107f9ef in process_one_work (worker=0xffff88810004d9c0, work=0xffff888100faf860) at kernel/workqueue.c:2275
#18 0xffffffff8107fbc5 in worker_thread (__worker=0xffff88810004d9c0) at kernel/workqueue.c:2421
#19 0xffffffff81086109 in kthread (_create=0xffff888100123040) at kernel/kthread.c:313
#20 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#21 0x0000000000000000 in ?? ()
```
#### ata_piix
```c
/*
#0  apic_update_vector (irqd=irqd@entry=0xffff88810004ea40, newvec=newvec@entry=35, newcpu=0) at arch/x86/kernel/apic/vector.c:134
#1  0xffffffff810478f9 in assign_vector_locked (irqd=irqd@entry=0xffff88810004ea40, dest=dest@entry=0xffffffff82c97540 <vector_searchmask>) at arch/x86/kernel/apic/vect
or.c:252
#2  0xffffffff81047b0b in assign_irq_vector_any_locked (irqd=0xffff88810004ea40) at arch/x86/kernel/apic/vector.c:279
#3  0xffffffff81047f17 in activate_reserved (irqd=0xffff88810004ea40) at arch/x86/kernel/apic/vector.c:393
#4  x86_vector_activate (dom=<optimized out>, irqd=0xffff88810004ea40, reserve=<optimized out>) at arch/x86/kernel/apic/vector.c:462
#5  0xffffffff810c07ee in __irq_domain_activate_irq (irqd=0xffff88810004ea40, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1718
#6  0xffffffff810c07cd in __irq_domain_activate_irq (irqd=irqd@entry=0xffff8881000f4428, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1715
#7  0xffffffff810c2350 in irq_domain_activate_irq (irq_data=irq_data@entry=0xffff8881000f4428, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1741
#8  0xffffffff810bf68b in irq_activate (desc=desc@entry=0xffff8881000f4400) at kernel/irq/chip.c:291
#9  0xffffffff810bcfd5 in __setup_irq (irq=irq@entry=14, desc=desc@entry=0xffff8881000f4400, new=new@entry=0xffff888100ffce80) at kernel/irq/manage.c:1678
#10 0xffffffff810bd4e7 in request_threaded_irq (irq=irq@entry=14, handler=handler@entry=0xffffffff816faa20 <ata_bmdma_interrupt>, thread_fn=thread_fn@entry=0x0 <fixed_p
ercpu_data>, irqflags=irqflags@entry=128, devname=devname@entry=0xffffffff822b83cc "ata_piix", dev_id=dev_id@entry=0xffff8881010ebd80) at kernel/irq/manage.c:2137
#11 0xffffffff810c017d in devm_request_threaded_irq (dev=dev@entry=0xffff8881002da0c0, irq=irq@entry=14, handler=handler@entry=0xffffffff816faa20 <ata_bmdma_interrupt>,
 thread_fn=thread_fn@entry=0x0 <fixed_percpu_data>, irqflags=irqflags@entry=128, devname=devname@entry=0xffffffff822b83cc "ata_piix", dev_id=0xffff8881010ebd80) at kern
el/irq/devres.c:67
#12 0xffffffff816f8e31 in devm_request_irq (dev_id=0xffff8881010ebd80, devname=0xffffffff822b83cc "ata_piix", irqflags=128, handler=0xffffffff816faa20 <ata_bmdma_interr
upt>, irq=14, dev=0xffff8881002da0c0) at ./include/linux/interrupt.h:210
#13 ata_pci_sff_activate_host (host=0xffff8881010ebd80, irq_handler=0xffffffff816faa20 <ata_bmdma_interrupt>, sht=sht@entry=0xffffffff825d8680 <piix_sht>) at drivers/at
a/libata-sff.c:2395
#14 0xffffffff817027e0 in piix_init_one (pdev=0xffff8881002da000, ent=<optimized out>) at drivers/ata/ata_piix.c:1742
#15 0xffffffff8143f89d in local_pci_probe (_ddi=_ddi@entry=0xffffc90000013d70) at drivers/pci/pci-driver.c:309
#16 0xffffffff81440c10 in pci_call_probe (id=<optimized out>, dev=0xffff8881002da000, drv=0xffffffff825d7600 <piix_pci_driver>) at drivers/pci/pci-driver.c:366
#17 __pci_device_probe (pci_dev=0xffff8881002da000, drv=0xffffffff825d7600 <piix_pci_driver>) at drivers/pci/pci-driver.c:391
#18 pci_device_probe (dev=0xffff8881002da0c0) at drivers/pci/pci-driver.c:434
#19 0xffffffff8168ad86 in really_probe (dev=dev@entry=0xffff8881002da0c0, drv=drv@entry=0xffffffff825d7670 <piix_pci_driver+112>) at drivers/base/dd.c:576
#20 0xffffffff8168afc1 in driver_probe_device (drv=drv@entry=0xffffffff825d7670 <piix_pci_driver+112>, dev=dev@entry=0xffff8881002da0c0) at drivers/base/dd.c:763
#21 0xffffffff8168b28e in device_driver_attach (drv=drv@entry=0xffffffff825d7670 <piix_pci_driver+112>, dev=dev@entry=0xffff8881002da0c0) at drivers/base/dd.c:1039
#22 0xffffffff8168b2f0 in __driver_attach (dev=0xffff8881002da0c0, data=0xffffffff825d7670 <piix_pci_driver+112>) at drivers/base/dd.c:1117
#23 0xffffffff81688f93 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff825d7670 <piix_pci_driver+112>, fn
=fn@entry=0xffffffff8168b2a0 <__driver_attach>) at drivers/base/bus.c:305
#24 0xffffffff8168a775 in driver_attach (drv=drv@entry=0xffffffff825d7670 <piix_pci_driver+112>) at drivers/base/dd.c:1133
#25 0xffffffff8168a142 in bus_add_driver (drv=drv@entry=0xffffffff825d7670 <piix_pci_driver+112>) at drivers/base/bus.c:622
#26 0xffffffff8168bb87 in driver_register (drv=drv@entry=0xffffffff825d7670 <piix_pci_driver+112>) at drivers/base/driver.c:171
#27 0xffffffff8143f261 in __pci_register_driver (drv=drv@entry=0xffffffff825d7600 <piix_pci_driver>, owner=owner@entry=0x0 <fixed_percpu_data>, mod_name=mod_name@entry=
0xffffffff822b83cc "ata_piix") at drivers/pci/pci-driver.c:1393
#28 0xffffffff82b89ee2 in piix_init () at drivers/ata/ata_piix.c:1771
#29 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b89ecd <piix_init>) at init/main.c:1249
#30 0xffffffff82b4c26b in do_initcall_level (command_line=0xffff888100123400 "root", level=6) at ./include/linux/compiler.h:234
#31 do_initcalls () at init/main.c:1338
#32 do_basic_setup () at init/main.c:1358
#33 kernel_init_freeable () at init/main.c:1560
#34 0xffffffff81b97129 in kernel_init (unused=<optimized out>) at init/main.c:1447
#35 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#36 0x0000000000000000 in ?? ()
>>>
```

#### nvme
```c
/*
#0  apic_update_vector (irqd=irqd@entry=0xffff888101194000, newvec=newvec@entry=36, newcpu=0) at arch/x86/kernel/apic/vector.c:134
#1  0xffffffff810478f9 in assign_vector_locked (irqd=irqd@entry=0xffff888101194000, dest=dest@entry=0xffffffff82c97540 <vector_searchmask>) at arch/x86/kernel/apic/vect
or.c:252
#2  0xffffffff81047b0b in assign_irq_vector_any_locked (irqd=0xffff888101194000) at arch/x86/kernel/apic/vector.c:279
#3  0xffffffff81047f17 in activate_reserved (irqd=0xffff888101194000) at arch/x86/kernel/apic/vector.c:393
#4  x86_vector_activate (dom=<optimized out>, irqd=0xffff888101194000, reserve=<optimized out>) at arch/x86/kernel/apic/vector.c:462
#5  0xffffffff810c07ee in __irq_domain_activate_irq (irqd=0xffff888101194000, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1718
#6  0xffffffff810c07cd in __irq_domain_activate_irq (irqd=irqd@entry=0xffff88810117ee28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1715
#7  0xffffffff810c2350 in irq_domain_activate_irq (irq_data=irq_data@entry=0xffff88810117ee28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1741
#8  0xffffffff810bf68b in irq_activate (desc=desc@entry=0xffff88810117ee00) at kernel/irq/chip.c:291
#9  0xffffffff810bcfd5 in __setup_irq (irq=irq@entry=24, desc=desc@entry=0xffff88810117ee00, new=new@entry=0xffff888100ffcf80) at kernel/irq/manage.c:1678
#10 0xffffffff810bd4e7 in request_threaded_irq (irq=24, handler=handler@entry=0xffffffff816e1720 <nvme_irq>, thread_fn=thread_fn@entry=0x0 <fixed_percpu_data>, irqflags
=irqflags@entry=128, devname=devname@entry=0xffff888100392b78 "nvme0q0", dev_id=dev_id@entry=0xffff88810117ea00) at kernel/irq/manage.c:2137
#11 0xffffffff8144377e in pci_request_irq (dev=0xffff8881002e0800, nr=0, handler=handler@entry=0xffffffff816e1720 <nvme_irq>, thread_fn=thread_fn@entry=0x0 <fixed_percp
u_data>, dev_id=0xffff88810117ea00, fmt=fmt@entry=0xffffffff822b26fd "nvme%dq%d") at drivers/pci/irq.c:48
#12 0xffffffff816e0186 in queue_request_irq (nvmeq=nvmeq@entry=0xffff88810117ea00) at drivers/nvme/host/pci.c:1542
#13 0xffffffff816e2738 in nvme_pci_configure_admin_queue (dev=0xffff8881003f4000) at drivers/nvme/host/pci.c:1742
#14 nvme_reset_work (work=0xffff8881003f46d0) at drivers/nvme/host/pci.c:2609
#15 0xffffffff8107f9ef in process_one_work (worker=0xffff8881002bd6c0, work=0xffff8881003f46d0) at kernel/workqueue.c:2275
#16 0xffffffff8107fbc5 in worker_thread (__worker=0xffff8881002bd6c0) at kernel/workqueue.c:2421
#17 0xffffffff81086109 in kthread (_create=0xffff88810033d680) at kernel/kthread.c:313
#18 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#19 0x0000000000000000 in ?? ()
```

#### i8042
```c
/*
#0  apic_update_vector (irqd=irqd@entry=0xffff88810004e940, newvec=newvec@entry=39, newcpu=0) at arch/x86/kernel/apic/vector.c:134
#1  0xffffffff810478f9 in assign_vector_locked (irqd=irqd@entry=0xffff88810004e940, dest=dest@entry=0xffffffff82c97540 <vector_searchmask>) at arch/x86/kernel/apic/vect
or.c:252
#2  0xffffffff81047b0b in assign_irq_vector_any_locked (irqd=0xffff88810004e940) at arch/x86/kernel/apic/vector.c:279
#3  0xffffffff81047f17 in activate_reserved (irqd=0xffff88810004e940) at arch/x86/kernel/apic/vector.c:393
#4  x86_vector_activate (dom=<optimized out>, irqd=0xffff88810004e940, reserve=<optimized out>) at arch/x86/kernel/apic/vector.c:462
#5  0xffffffff810c07ee in __irq_domain_activate_irq (irqd=0xffff88810004e940, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1718
#6  0xffffffff810c07cd in __irq_domain_activate_irq (irqd=irqd@entry=0xffff8881000f4028, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1715
#7  0xffffffff810c2350 in irq_domain_activate_irq (irq_data=irq_data@entry=0xffff8881000f4028, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1741
#8  0xffffffff810bf68b in irq_activate (desc=desc@entry=0xffff8881000f4000) at kernel/irq/chip.c:291
#9  0xffffffff810bcfd5 in __setup_irq (irq=irq@entry=12, desc=desc@entry=0xffff8881000f4000, new=new@entry=0xffff888101166980) at kernel/irq/manage.c:1678
#10 0xffffffff810bd4e7 in request_threaded_irq (irq=12, handler=handler@entry=0xffffffff82b8b6ee <i8042_aux_test_irq>, thread_fn=thread_fn@entry=0x0 <fixed_percpu_data>
, irqflags=irqflags@entry=128, devname=devname@entry=0xffffffff822cc25f "i8042", dev_id=<optimized out>) at kernel/irq/manage.c:2137
#11 0xffffffff82b8bc09 in request_irq (dev=<optimized out>, name=0xffffffff822cc25f "i8042", flags=128, handler=0xffffffff82b8b6ee <i8042_aux_test_irq>, irq=<optimized
out>) at ./include/linux/interrupt.h:164
#12 i8042_check_aux () at drivers/input/serio/i8042.c:881
#13 i8042_setup_aux () at drivers/input/serio/i8042.c:1452
#14 i8042_probe (dev=<optimized out>) at drivers/input/serio/i8042.c:1560
#15 0xffffffff8168d6aa in platform_probe (_dev=0xffff888101393410) at drivers/base/platform.c:1447
#16 0xffffffff8168b4d6 in really_probe (dev=dev@entry=0xffff888101393410, drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>) at drivers/base/dd.c:576
#17 0xffffffff8168b711 in driver_probe_device (drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>, dev=dev@entry=0xffff888101393410) at drivers/base/dd.c:763
#18 0xffffffff8168b9de in device_driver_attach (drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>, dev=dev@entry=0xffff888101393410) at drivers/base/dd.c:1039
#19 0xffffffff8168ba40 in __driver_attach (dev=0xffff888101393410, data=0xffffffff825e75e8 <i8042_driver+40>) at drivers/base/dd.c:1117
#20 0xffffffff816896e3 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff825e75e8 <i8042_driver+40>, fn=fn@
entry=0xffffffff8168b9f0 <__driver_attach>) at drivers/base/bus.c:305
#21 0xffffffff8168aec5 in driver_attach (drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>) at drivers/base/dd.c:1133
#22 0xffffffff8168a892 in bus_add_driver (drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>) at drivers/base/bus.c:622
#23 0xffffffff8168c2d7 in driver_register (drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>) at drivers/base/driver.c:171
#24 0xffffffff8168dc5e in __platform_driver_register (owner=<optimized out>, drv=0xffffffff825e75c0 <i8042_driver>) at drivers/base/platform.c:894
#25 __platform_driver_probe (drv=0xffffffff825e75c0 <i8042_driver>, probe=<optimized out>, module=<optimized out>) at drivers/base/platform.c:962
#26 0xffffffff8168e0fa in __platform_create_bundle (driver=driver@entry=0xffffffff825e75c0 <i8042_driver>, probe=probe@entry=0xffffffff82b8b983 <i8042_probe>, res=res@e
ntry=0x0 <fixed_percpu_data>, n_res=n_res@entry=0, data=data@entry=0x0 <fixed_percpu_data>, size=size@entry=0, module=0x0 <fixed_percpu_data>) at drivers/base/platform.
c:1026
#27 0xffffffff82b8c44f in i8042_init () at drivers/input/serio/i8042.c:1629
#28 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b8c02c <i8042_init>) at init/main.c:1249
#29 0xffffffff82b4c26b in do_initcall_level (command_line=0xffff888100123400 "root", level=6) at ./include/linux/compiler.h:234
#30 do_initcalls () at init/main.c:1338
#31 do_basic_setup () at init/main.c:1358
#32 kernel_init_freeable () at init/main.c:1560
#33 0xffffffff81b979d9 in kernel_init (unused=<optimized out>) at init/main.c:1447
#34 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#35 0x0000000000000000 in ?? ()
>>>

#0  apic_update_vector (irqd=irqd@entry=0xffff88810004e3c0, newvec=newvec@entry=40, newcpu=0) at arch/x86/kernel/apic/vector.c:134
#1  0xffffffff810478f9 in assign_vector_locked (irqd=irqd@entry=0xffff88810004e3c0, dest=dest@entry=0xffffffff82c97540 <vector_searchmask>) at arch/x86/kernel/apic/vect
or.c:252
#2  0xffffffff81047b0b in assign_irq_vector_any_locked (irqd=0xffff88810004e3c0) at arch/x86/kernel/apic/vector.c:279
#3  0xffffffff81047f17 in activate_reserved (irqd=0xffff88810004e3c0) at arch/x86/kernel/apic/vector.c:393
#4  x86_vector_activate (dom=<optimized out>, irqd=0xffff88810004e3c0, reserve=<optimized out>) at arch/x86/kernel/apic/vector.c:462
#5  0xffffffff810c07ee in __irq_domain_activate_irq (irqd=0xffff88810004e3c0, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1718
#6  0xffffffff810c07cd in __irq_domain_activate_irq (irqd=irqd@entry=0xffff888100060a28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1715 //
#7  0xffffffff810c2350 in irq_domain_activate_irq (irq_data=irq_data@entry=0xffff888100060a28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1741
#8  0xffffffff810bf68b in irq_activate (desc=desc@entry=0xffff888100060a00) at kernel/irq/chip.c:291
#9  0xffffffff810bcfd5 in __setup_irq (irq=irq@entry=1, desc=desc@entry=0xffff888100060a00, new=new@entry=0xffff888101168a00) at kernel/irq/manage.c:1678
#10 0xffffffff810bd4e7 in request_threaded_irq (irq=1, handler=handler@entry=0xffffffff817d5980 <i8042_interrupt>, thread_fn=thread_fn@entry=0x0 <fixed_percpu_data>, ir
qflags=irqflags@entry=128, devname=devname@entry=0xffffffff822cc2af "i8042", dev_id=<optimized out>) at kernel/irq/manage.c:2137
#11 0xffffffff82b8bf53 in request_irq (dev=<optimized out>, name=0xffffffff822cc2af "i8042", flags=128, handler=0xffffffff817d5980 <i8042_interrupt>, irq=<optimized out
>) at ./include/linux/interrupt.h:164
#12 i8042_setup_kbd () at drivers/input/serio/i8042.c:1496
#13 i8042_probe (dev=<optimized out>) at drivers/input/serio/i8042.c:1566
#14 0xffffffff8168d61a in platform_probe (_dev=0xffff888101393410) at drivers/base/platform.c:1447
#15 0xffffffff8168b446 in really_probe (dev=dev@entry=0xffff888101393410, drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>) at drivers/base/dd.c:576
#16 0xffffffff8168b681 in driver_probe_device (drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>, dev=dev@entry=0xffff888101393410) at drivers/base/dd.c:763
#17 0xffffffff8168b94e in device_driver_attach (drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>, dev=dev@entry=0xffff888101393410) at drivers/base/dd.c:1039
#18 0xffffffff8168b9b0 in __driver_attach (dev=0xffff888101393410, data=0xffffffff825e75e8 <i8042_driver+40>) at drivers/base/dd.c:1117
#19 0xffffffff81689653 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff825e75e8 <i8042_driver+40>, fn=fn@
entry=0xffffffff8168b960 <__driver_attach>) at drivers/base/bus.c:305
#20 0xffffffff8168ae35 in driver_attach (drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>) at drivers/base/dd.c:1133
#21 0xffffffff8168a802 in bus_add_driver (drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>) at drivers/base/bus.c:622
#22 0xffffffff8168c247 in driver_register (drv=drv@entry=0xffffffff825e75e8 <i8042_driver+40>) at drivers/base/driver.c:171
#23 0xffffffff8168dbce in __platform_driver_register (owner=<optimized out>, drv=0xffffffff825e75c0 <i8042_driver>) at drivers/base/platform.c:894
#24 __platform_driver_probe (drv=0xffffffff825e75c0 <i8042_driver>, probe=<optimized out>, module=<optimized out>) at drivers/base/platform.c:962
#25 0xffffffff8168e06a in __platform_create_bundle (driver=driver@entry=0xffffffff825e75c0 <i8042_driver>, probe=probe@entry=0xffffffff82b8b983 <i8042_probe>, res=res@e
ntry=0x0 <fixed_percpu_data>, n_res=n_res@entry=0, data=data@entry=0x0 <fixed_percpu_data>, size=size@entry=0, module=0x0 <fixed_percpu_data>) at drivers/base/platform.
c:1026
#26 0xffffffff82b8c44f in i8042_init () at drivers/input/serio/i8042.c:1629
#27 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b8c02c <i8042_init>) at init/main.c:1249
#28 0xffffffff82b4c26b in do_initcall_level (command_line=0xffff888100123400 "root", level=6) at ./include/linux/compiler.h:234
#29 do_initcalls () at init/main.c:1338
#30 do_basic_setup () at init/main.c:1358
#31 kernel_init_freeable () at init/main.c:1560
#32 0xffffffff81b97a69 in kernel_init (unused=<optimized out>) at init/main.c:1447
#33 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#34 0x0000000000000000 in ?? ()
```
trace 的代码显示，在 i8042_setup_kbd 的时候，I8042_KBD_IRQ 已经设置为 2 了, 使用 gdb 调试，i8042_setup_kbd 可以正常返回

但是，还是无法解释，为什么 apic_update_vector 没有被执行啊

使用 gdb 仔细的调试，问题出现在 `__irq_domain_activate_irq`:
```txt
>>> p irqd
$7 = (struct irq_data *) 0xffff888100060c28
>>> p irqd->domain
$8 = (struct irq_domain *) 0x0 <fixed_percpu_data>
```
而在 request_threaded_irq 分别测试修改 acpi 前后的表格，发现 2 号对应的 domain 是空的，而 1 号初始化号了

```txt
>>> p desc
$1 = (struct irq_desc *) 0xffff888100060a00
>>> p desc->irq_data
$2 = {
  mask = 0,
  irq = 1,
  hwirq = 1,
  common = 0xffff888100060a00,
  chip = 0xffffffff8267c6a0 <ioapic_chip>,
  domain = 0xffff8881000fe000,
  parent_data = 0xffff88810004e3c0,
  chip_data = 0xffff88810005c2c0
}
```
也就是，这个 irq 在很早的时候就已经初始化好了。

在 i8042_pnp_kbd_probe 中会初始化 i8042_pnp_kbd_probe



#### serial8250
```c
/*
#0  apic_update_vector (irqd=irqd@entry=0xffff88810004e540, newvec=newvec@entry=42, newcpu=0) at arch/x86/kernel/apic/vector.c:134
#1  0xffffffff810478f9 in assign_vector_locked (irqd=irqd@entry=0xffff88810004e540, dest=dest@entry=0xffffffff82c97540 <vector_searchmask>) at arch/x86/kernel/apic/vect
or.c:252
#2  0xffffffff81047b0b in assign_irq_vector_any_locked (irqd=0xffff88810004e540) at arch/x86/kernel/apic/vector.c:279
#3  0xffffffff81047f17 in activate_reserved (irqd=0xffff88810004e540) at arch/x86/kernel/apic/vector.c:393
#4  x86_vector_activate (dom=<optimized out>, irqd=0xffff88810004e540, reserve=<optimized out>) at arch/x86/kernel/apic/vector.c:462
#5  0xffffffff810c07ee in __irq_domain_activate_irq (irqd=0xffff88810004e540, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1718
#6  0xffffffff810c07cd in __irq_domain_activate_irq (irqd=irqd@entry=0xffff8881000f3028, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1715
#7  0xffffffff810c2350 in irq_domain_activate_irq (irq_data=irq_data@entry=0xffff8881000f3028, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1741
#8  0xffffffff810bf68b in irq_activate (desc=desc@entry=0xffff8881000f3000) at kernel/irq/chip.c:291
#9  0xffffffff810bcfd5 in __setup_irq (irq=irq@entry=4, desc=desc@entry=0xffff8881000f3000, new=new@entry=0xffff8881013e1300) at kernel/irq/manage.c:1678
#10 0xffffffff810bd4e7 in request_threaded_irq (irq=4, handler=handler@entry=0xffffffff814e6250 <serial8250_interrupt>, thread_fn=thread_fn@entry=0x0 <fixed_percpu_data
>, irqflags=<optimized out>, devname=0xffff8881003924f8 "ttyS0", dev_id=dev_id@entry=0xffff88810137e560) at kernel/irq/manage.c:2137
#11 0xffffffff814e74f8 in request_irq (dev=0xffff88810137e560, name=<optimized out>, flags=<optimized out>, handler=0xffffffff814e6250 <serial8250_interrupt>, irq=<opti
mized out>) at ./include/linux/interrupt.h:164
#12 serial_link_irq_chain (up=0xffffffff82d03320 <serial8250_ports>) at drivers/tty/serial/8250/8250_core.c:212
#13 univ8250_setup_irq (up=0xffffffff82d03320 <serial8250_ports>) at drivers/tty/serial/8250/8250_core.c:337
#14 0xffffffff814e964b in serial8250_do_startup (port=0xffffffff82d03320 <serial8250_ports>) at drivers/tty/serial/8250/8250_port.c:2312
#15 0xffffffff814e53e6 in uart_port_startup (init_hw=0, state=0xffff888101010000, tty=0xffff88810152d400) at drivers/tty/serial/serial_core.c:221
#16 uart_startup (tty=0xffff88810152d400, state=state@entry=0xffff888101010000, init_hw=init_hw@entry=0) at drivers/tty/serial/serial_core.c:260
#17 0xffffffff814e5566 in uart_startup (init_hw=<optimized out>, state=<optimized out>, tty=<optimized out>) at drivers/tty/serial/serial_core.c:1785
#18 uart_port_activate (tty=<optimized out>, port=0xffff888101010000) at drivers/tty/serial/serial_core.c:1798
#19 uart_port_activate (port=0xffff888101010000, tty=<optimized out>) at drivers/tty/serial/serial_core.c:1785
#20 0xffffffff814cfc48 in tty_port_open (port=0xffff888101010000, tty=0xffff88810152d400, filp=0xffff8881013ef500) at drivers/tty/tty_port.c:690
#21 0xffffffff814e2325 in uart_open (tty=<optimized out>, filp=<optimized out>) at drivers/tty/serial/serial_core.c:1778
#22 0xffffffff814c8ade in tty_open (inode=0xffff888100a82470, filp=0xffff8881013ef500) at drivers/tty/tty_io.c:2167
#23 0xffffffff811e7378 in chrdev_open (inode=inode@entry=0xffff888100a82470, filp=filp@entry=0xffff8881013ef500) at fs/char_dev.c:414
#24 0xffffffff811de5c6 in do_dentry_open (f=f@entry=0xffff8881013ef500, inode=0xffff888100a82470, open=0xffffffff811e72e0 <chrdev_open>, open@entry=0x0 <fixed_percpu_da
ta>) at fs/open.c:826
#25 0xffffffff811dffb9 in vfs_open (path=path@entry=0xffffc90000013db0, file=file@entry=0xffff8881013ef500) at ./include/linux/dcache.h:555
#26 0xffffffff811f2c41 in do_open (op=0xffffc90000013ed4, op=0xffffc90000013ed4, file=0xffff8881013ef500, nd=0xffffc90000013db0) at fs/namei.c:3361
#27 path_openat (nd=nd@entry=0xffffc90000013db0, op=op@entry=0xffffc90000013ed4, flags=flags@entry=65) at fs/namei.c:3494
#28 0xffffffff811f461d in do_filp_open (dfd=dfd@entry=-100, pathname=pathname@entry=0xffff8881001f0000, op=op@entry=0xffffc90000013ed4) at fs/namei.c:3521
#29 0xffffffff811e029c in file_open_name (name=name@entry=0xffff8881001f0000, flags=flags@entry=2, mode=mode@entry=0) at fs/open.c:1132
#30 0xffffffff811e0397 in filp_open (filename=filename@entry=0xffffffff8220afd0 "/dev/console", flags=flags@entry=2, mode=mode@entry=0) at fs/open.c:1152
#31 0xffffffff82b4c094 in console_on_rootfs () at init/main.c:1513
#32 0xffffffff82b4c288 in kernel_init_freeable () at init/main.c:1565
#33 0xffffffff81b979d9 in kernel_init (unused=<optimized out>) at init/main.c:1447
#34 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#35 0x0000000000000000 in ?? ()
```
在 acpi 中间配置的 4，不会这么巧吧?

阅读 serial_isa_realizefn 的函数，修改 isa_serial_irq 数组中第一个数值从 4 到 5，在 guest 的 /proc/interrupts 中可以看到 serial 数值的确发生改变了

在 serial_pnp_probe 中，初始化了这个中断号:


但是 i8042 的 acpi 配置不能修改这些数值，否则键盘直接无法响应。
- 仔细想想，这应该是 qemu 的原因吧, 因为是从 QEMU 中注入的中断号是一个错误的。
  - 修改 aml 会导致键盘中断根本不会被注册

- [ ] 让人尴尬的问题，实际上，serial8250_interrupt 也就是开始会被调用，然后就永久的沉默了，搞不清楚都是 serial 的使用过程。
  - 怀疑这都是内核手动触发的, 比如在 serial8250_do_startup

#### e1000
```c
/*
>>> bt
#0  apic_update_vector (irqd=irqd@entry=0xffff88810004e8c0, newvec=newvec@entry=43, newcpu=0) at arch/x86/kernel/apic/vector.c:134
#1  0xffffffff810478f9 in assign_vector_locked (irqd=irqd@entry=0xffff88810004e8c0, dest=dest@entry=0xffffffff82c97540 <vector_searchmask>) at arch/x86/kernel/apic/vect
or.c:252
#2  0xffffffff81047b0b in assign_irq_vector_any_locked (irqd=0xffff88810004e8c0) at arch/x86/kernel/apic/vector.c:279
#3  0xffffffff81047f17 in activate_reserved (irqd=0xffff88810004e8c0) at arch/x86/kernel/apic/vector.c:393
#4  x86_vector_activate (dom=<optimized out>, irqd=0xffff88810004e8c0, reserve=<optimized out>) at arch/x86/kernel/apic/vector.c:462
#5  0xffffffff810c07ee in __irq_domain_activate_irq (irqd=0xffff88810004e8c0, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1718
#6  0xffffffff810c07cd in __irq_domain_activate_irq (irqd=irqd@entry=0xffff8881000f3e28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1715
#7  0xffffffff810c2350 in irq_domain_activate_irq (irq_data=irq_data@entry=0xffff8881000f3e28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1741
#8  0xffffffff810bf68b in irq_activate (desc=desc@entry=0xffff8881000f3e00) at kernel/irq/chip.c:291
#9  0xffffffff810bcfd5 in __setup_irq (irq=irq@entry=11, desc=desc@entry=0xffff8881000f3e00, new=new@entry=0xffff8881013e1600) at kernel/irq/manage.c:1678
#10 0xffffffff810bd4e7 in request_threaded_irq (irq=11, handler=handler@entry=0xffffffff8172fd70 <e1000_intr>, thread_fn=thread_fn@entry=0x0 <fixed_percpu_data>, irqfla
gs=irqflags@entry=128, devname=0xffff8881001fa000 "eth0", dev_id=<optimized out>) at kernel/irq/manage.c:2137
#11 0xffffffff8172fea2 in request_irq (dev=<optimized out>, name=<optimized out>, flags=128, handler=0xffffffff8172fd70 <e1000_intr>, irq=<optimized out>) at ./include/
linux/interrupt.h:164
#12 e1000_request_irq (adapter=adapter@entry=0xffff8881001fa8c0) at drivers/net/ethernet/intel/e1000/e1000_main.c:260
#13 0xffffffff8173664e in e1000_open (netdev=0xffff8881001fa000) at drivers/net/ethernet/intel/e1000/e1000_main.c:1391
#14 0xffffffff818c2115 in __dev_open (dev=dev@entry=0xffff8881001fa000, extack=extack@entry=0x0 <fixed_percpu_data>) at net/core/dev.c:1609
#15 0xffffffff818c250f in __dev_change_flags (dev=dev@entry=0xffff8881001fa000, flags=4099, extack=extack@entry=0x0 <fixed_percpu_data>) at net/core/dev.c:8741
#16 0xffffffff818c258c in dev_change_flags (dev=dev@entry=0xffff8881001fa000, flags=<optimized out>, extack=extack@entry=0x0 <fixed_percpu_data>) at net/core/dev.c:8812
#17 0xffffffff8198b4e1 in devinet_ioctl (net=net@entry=0xffffffff825fa940 <init_net>, cmd=cmd@entry=35092, ifr=ifr@entry=0xffffc90000497d80) at net/ipv4/devinet.c:1142
#18 0xffffffff8198d25b in inet_ioctl (sock=<optimized out>, cmd=35092, arg=140729080145584) at net/ipv4/af_inet.c:971
#19 0xffffffff81897c42 in sock_do_ioctl (net=net@entry=0xffffffff825fa940 <init_net>, sock=<optimized out>, cmd=cmd@entry=35092, arg=arg@entry=140729080145584) at net/s
ocket.c:1039
#20 0xffffffff81897fc7 in sock_ioctl (file=<optimized out>, cmd=35092, arg=140729080145584) at net/socket.c:1179
#21 0xffffffff811f7fee in vfs_ioctl (arg=140729080145584, cmd=<optimized out>, filp=0xffff888101623700) at fs/ioctl.c:51
#22 __do_sys_ioctl (arg=140729080145584, cmd=35092, fd=6) at fs/ioctl.c:1069
#23 __se_sys_ioctl (arg=140729080145584, cmd=35092, fd=6) at fs/ioctl.c:1055
#24 __x64_sys_ioctl (regs=<optimized out>) at fs/ioctl.c:1055
#25 0xffffffff81b94500 in do_syscall_64 (nr=<optimized out>, regs=0xffffc90000497f58) at arch/x86/entry/common.c:47
#26 0xffffffff81c0007c in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:112
#27 0x0000000000000000 in ?? ()
```

## pnp
从这里面，其实可以非常清楚的 trace 在 acpi table 中 page walk 的时候，将这种资源添加进去的，其中包括中断号资源
在 serial_pnp_probe 中，将这个中断号告知设备驱动，之后再去注册。
```c
/*
>>> bt
#0  pnp_add_resource (dev=dev@entry=0xffff888100322400, res=res@entry=0xffffc90000013c78) at drivers/pnp/resource.c:514
#1  0xffffffff814af932 in pnpacpi_allocated_resource (res=0xffff88810033da40, data=0xffff888100322400) at drivers/pnp/pnpacpi/rsparser.c:177
#2  0xffffffff81497cb0 in acpi_walk_resource_buffer (buffer=buffer@entry=0xffffc90000013d18, user_function=user_function@entry=0xffffffff814af8e0 <pnpacpi_allocated_res
ource>, context=context@entry=0xffff888100322400) at drivers/acpi/acpica/rsxface.c:547
#3  0xffffffff814980ce in acpi_walk_resources (context=0xffff888100322400, user_function=0xffffffff814af8e0 <pnpacpi_allocated_resource>, name=0xffffffff8225bf9f "_CRS"
, device_handle=0xffff888100322400) at drivers/acpi/acpica/rsxface.c:623
#4  acpi_walk_resources (device_handle=device_handle@entry=0xffff8881000f2780, name=name@entry=0xffffffff8225bf9f "_CRS", user_function=user_function@entry=0xffffffff81
4af8e0 <pnpacpi_allocated_resource>, context=context@entry=0xffff888100322400) at drivers/acpi/acpica/rsxface.c:594
#5  0xffffffff814afb90 in pnpacpi_parse_allocated_resource (dev=dev@entry=0xffff888100322400) at drivers/pnp/pnpacpi/rsparser.c:280
#6  0xffffffff82b80ea4 in pnpacpi_add_device (device=0xffff8881001eb800) at drivers/pnp/pnpacpi/core.c:258
#7  pnpacpi_add_device_handler (handle=<optimized out>, lvl=<optimized out>, context=<optimized out>, rv=<optimized out>) at drivers/pnp/pnpacpi/core.c:295
#8  0xffffffff81493756 in acpi_ns_get_device_callback (return_value=0x0 <fixed_percpu_data>, context=0xffffc90000013e80, nesting_level=4, obj_handle=0xffff8881000f2780)
 at drivers/acpi/acpica/nsxfeval.c:740
#9  acpi_ns_get_device_callback (obj_handle=obj_handle@entry=0xffff8881000f2780, nesting_level=nesting_level@entry=4, context=context@entry=0xffffc90000013e80, return_v
alue=return_value@entry=0x0 <fixed_percpu_data>) at drivers/acpi/acpica/nsxfeval.c:635
#10 0xffffffff81492fe5 in acpi_ns_walk_namespace (type=type@entry=6, start_node=<optimized out>, start_node@entry=0xffffffffffffffff, max_depth=max_depth@entry=42949672
95, flags=flags@entry=1, descending_callback=descending_callback@entry=0xffffffff814935e0 <acpi_ns_get_device_callback>, ascending_callback=ascending_callback@entry=0x0
 <fixed_percpu_data>, context=0xffffc90000013e80, return_value=0x0 <fixed_percpu_data>) at drivers/acpi/acpica/nswalk.c:229
#11 0xffffffff81493152 in acpi_get_devices (HID=HID@entry=0x0 <fixed_percpu_data>, user_function=user_function@entry=0xffffffff82b80d07 <pnpacpi_add_device_handler>, co
ntext=context@entry=0x0 <fixed_percpu_data>, return_value=return_value@entry=0x0 <fixed_percpu_data>) at drivers/acpi/acpica/nsxfeval.c:805
#12 0xffffffff82b80ce8 in pnpacpi_init () at drivers/pnp/pnpacpi/core.c:308
#13 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b80ca2 <pnpacpi_init>) at init/main.c:1249
#14 0xffffffff82b4c26b in do_initcall_level (command_line=0xffff888100123400 "root", level=5) at ./include/linux/compiler.h:234
#15 do_initcalls () at init/main.c:1338
#16 do_basic_setup () at init/main.c:1358
#17 kernel_init_freeable () at init/main.c:1560
#18 0xffffffff81b97a69 in kernel_init (unused=<optimized out>) at init/main.c:1447
#19 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#20 0x0000000000000000 in ?? ()
```

键盘的中断号也是通过 pnp 来进行设置的：
1. i8042_pnp_kbd_probe 会设置 i8042_pnp_kbd_irq
2. i8042_pnp_init 用 i8042_pnp_kbd_probe 来设置 i8042_kbd_irq，然后会拿着这个数值去 request_irq

## [ ] irq_desc_tree 和 vector_irq 的关系是什么

#### irq domain 的结构
1. 在 arch_early_irq_init 会初始化 16 个 irq_desc

紧接着创建变量
```c
x86_vector_domain = irq_domain_create_tree(fn, &x86_vector_domain_ops, NULL);
```

- [ ] 前 16 是这个时候创建，之后的是怎么创建的?

2. vector 在上，而 mp_ioapic_irqdomain_ops

```c
static const struct irq_domain_ops x86_vector_domain_ops = {
  .select   = x86_vector_select,
  .alloc    = x86_vector_alloc_irqs,
  .free   = x86_vector_free_irqs,
  .activate = x86_vector_activate,
  .deactivate = x86_vector_deactivate,
#ifdef CONFIG_GENERIC_IRQ_DEBUGFS
  .debug_show = x86_vector_debug_show,
#endif
};

const struct irq_domain_ops mp_ioapic_irqdomain_ops = {
  .alloc    = mp_irqdomain_alloc,
  .free   = mp_irqdomain_free,
  .activate = mp_irqdomain_activate,
  .deactivate = mp_irqdomain_deactivate,
};
```
x86_vector_domain_ops 和 mp_ioapic_irqdomain_ops 其实就是对应 lapic 和 ioapic:

- mp_ioapic_irqdomain_ops
  - mp_register_handler : 注册了 `__irq_set_handler`

- [ ] 不知道为什么 mp_ioapic_irqdomain_ops 正好不会注册 2 号?
  - [ ] 虽然 0 号 pin 没有连接上

从 mp_register_handler 到达的：
```c
/*
#0  x86_vector_alloc_irqs (domain=0xffff88810004d180, virq=1, nr_irqs=1, arg=0xffffffff82403df0) at arch/x86/kernel/apic/vector.c:533
#1  0xffffffff8104a2f1 in mp_irqdomain_alloc (domain=0xffff8881000fe000, virq=1, nr_irqs=1, arg=0xffffffff82403df0) at arch/x86/kernel/apic/io_apic.c:3020
#2  0xffffffff810c1c35 in irq_domain_alloc_irqs_hierarchy (arg=0xffffffff82403df0, nr_irqs=1, irq_base=1, domain=0xffff8881000fe000) at kernel/irq/irqdomain.c:1383
#3  __irq_domain_alloc_irqs (domain=domain@entry=0xffff8881000fe000, irq_base=irq_base@entry=1, nr_irqs=nr_irqs@entry=1, node=node@entry=-1, arg=arg@entry=0xffffffff824
03df0, realloc=realloc@entry=true, affinity=0x0 <fixed_percpu_data>) at kernel/irq/irqdomain.c:1439
#4  0xffffffff8104922a in alloc_isa_irq_from_domain (domain=domain@entry=0xffff8881000fe000, irq=irq@entry=1, ioapic=ioapic@entry=0, info=info@entry=0xffffffff82403df0,
 pin=1) at arch/x86/kernel/apic/io_apic.c:1008
#5  0xffffffff81049f2a in mp_map_pin_to_irq (gsi=1, idx=<optimized out>, ioapic=<optimized out>, pin=pin@entry=1, flags=1, info=info@entry=0x0 <fixed_percpu_data>) at a
rch/x86/kernel/apic/io_apic.c:1057
#6  0xffffffff8104a0c9 in pin_2_irq (idx=<optimized out>, ioapic=ioapic@entry=0, pin=pin@entry=1, flags=flags@entry=1) at arch/x86/kernel/apic/io_apic.c:1103
#7  0xffffffff82b5f326 in setup_IO_APIC_irqs () at arch/x86/kernel/apic/io_apic.c:1219
#8  setup_IO_APIC () at arch/x86/kernel/apic/io_apic.c:2416
#9  0xffffffff82b52ce4 in x86_late_time_init () at arch/x86/kernel/time.c:100
#10 0xffffffff82b4bfcb in start_kernel () at init/main.c:1051
#11 0xffffffff81000107 in secondary_startup_64 () at arch/x86/kernel/head_64.S:283
#12 0x0000000000000000 in ?? ()
*/
```

从 msi_domain_alloc 到达的 :
```c
/*
#0  x86_vector_alloc_irqs (domain=0xffff88810004d180, virq=24, nr_irqs=1, arg=0xffffc90000043c18) at arch/x86/kernel/apic/vector.c:533
#1  0xffffffff810c3982 in msi_domain_alloc (domain=0xffff88810004df00, virq=24, nr_irqs=1, arg=0xffffc90000043c18) at kernel/irq/msi.c:150
#2  0xffffffff810c1c35 in irq_domain_alloc_irqs_hierarchy (arg=0xffffc90000043c18, nr_irqs=1, irq_base=24, domain=0xffff88810004df00) at kernel/irq/irqdomain.c:1383
#3  __irq_domain_alloc_irqs (domain=domain@entry=0xffff88810004df00, irq_base=irq_base@entry=-1, nr_irqs=1, node=<optimized out>, arg=arg@entry=0xffffc90000043c18, real
loc=realloc@entry=false, affinity=0x0 <fixed_percpu_data>) at kernel/irq/irqdomain.c:1439
#4  0xffffffff810c3f88 in __msi_domain_alloc_irqs (domain=0xffff88810004df00, dev=0xffff8881002e08c0, nvec=<optimized out>) at ./include/linux/device.h:636
#5  0xffffffff81448c44 in msix_capability_init (affd=<optimized out>, nvec=<optimized out>, entries=0x0 <fixed_percpu_data>, dev=0xffff8881002e0800) at drivers/pci/msi.
c:788
#6  __pci_enable_msix (flags=<optimized out>, affd=<optimized out>, nvec=<optimized out>, entries=0x0 <fixed_percpu_data>, dev=0xffff8881002e0800) at drivers/pci/msi.c:
1003
#7  __pci_enable_msix_range (flags=<optimized out>, affd=<optimized out>, maxvec=<optimized out>, minvec=<optimized out>, entries=<optimized out>, dev=<optimized out>)
at drivers/pci/msi.c:1137
#8  __pci_enable_msix_range (dev=0xffff8881002e0800, entries=0x0 <fixed_percpu_data>, minvec=<optimized out>, maxvec=<optimized out>, affd=<optimized out>, flags=<optim
ized out>) at drivers/pci/msi.c:1117
#9  0xffffffff81448e80 in pci_alloc_irq_vectors_affinity (dev=dev@entry=0xffff8881002e0800, min_vecs=min_vecs@entry=1, max_vecs=max_vecs@entry=1, flags=flags@entry=7, a
ffd=affd@entry=0x0 <fixed_percpu_data>) at drivers/pci/msi.c:1206
#10 0xffffffff816e2b5a in pci_alloc_irq_vectors (flags=7, max_vecs=1, min_vecs=1, dev=0xffff8881002e0800) at ./include/linux/pci.h:1824
#11 nvme_pci_enable (dev=0xffff8881003f4000) at drivers/nvme/host/pci.c:2381
#12 nvme_reset_work (work=0xffff8881003f46d0) at drivers/nvme/host/pci.c:2605
#13 0xffffffff8107f9ef in process_one_work (worker=0xffff88810004d9c0, work=0xffff8881003f46d0) at kernel/workqueue.c:2275
#14 0xffffffff8107fbc5 in worker_thread (__worker=0xffff88810004d9c0) at kernel/workqueue.c:2421
#15 0xffffffff81086109 in kthread (_create=0xffff888100123040) at kernel/kthread.c:313
#16 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
*/
```
