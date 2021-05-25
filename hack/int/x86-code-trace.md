# Trace x86 interrupt

## /proc/interrupts
```
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


## trigger
```c
/*
[   36.794135] Call Trace:
[   36.794140]  <IRQ>
[   36.794143]  dump_stack+0x64/0x7c
[   36.794150]  pollwake+0x2a/0x90
[   36.794157]  ? check_preempt_curr+0x3a/0x70
[   36.794162]  ? ttwu_do_wakeup.isra.0+0xd/0xd0
[   36.794167]  __wake_up_common+0x75/0x140
[   36.794172]  __wake_up_common_lock+0x77/0xb0
[   36.794178]  evdev_events+0x7c/0xa0
[   36.794184]  input_to_handler+0x90/0xf0
[   36.794190]  input_pass_values.part.0+0x119/0x140
[   36.794196]  input_handle_event+0x20e/0x5f0
[   36.794203]  input_event+0x4a/0x70
[   36.794208]  atkbd_interrupt+0x47f/0x640
[   36.794213]  serio_interrupt+0x42/0x90
[   36.794218]  i8042_interrupt+0x146/0x250
[   36.794223]  __handle_irq_event_percpu+0x38/0x150
[   36.794229]  handle_irq_event_percpu+0x2c/0x80
[   36.794234]  handle_irq_event+0x23/0x50
[   36.794252]  handle_edge_irq+0x79/0x190
[   36.794257]  __common_interrupt+0x39/0x90
[   36.794277]  common_interrupt+0x76/0xa0
[   36.794283]  </IRQ>
[   36.794285]  asm_common_interrupt+0x1e/0x40
```

```c
/*
#0  nvme_irq (irq=24, data=0xffff888101150e00) at drivers/nvme/host/pci.c:1066
#1  0xffffffff810bb448 in __handle_irq_event_percpu (desc=desc@entry=0xffff888101163200, flags=flags@entry=0xffffc90000003f84) at kernel/irq/handle.c:156
#2  0xffffffff810bb58c in handle_irq_event_percpu (desc=desc@entry=0xffff888101163200) at kernel/irq/handle.c:196
#3  0xffffffff810bb603 in handle_irq_event (desc=desc@entry=0xffff888101163200) at kernel/irq/handle.c:213
#4  0xffffffff810bf4c9 in handle_edge_irq (desc=0xffff888101163200) at kernel/irq/chip.c:819
#5  0xffffffff81021c19 in generic_handle_irq_desc (desc=0xffff888101163200) at ./include/linux/irqdesc.h:158
#6  handle_irq (regs=<optimized out>, desc=0xffff888101163200) at arch/x86/kernel/irq.c:231
#7  __common_interrupt (regs=<optimized out>, vector=37) at arch/x86/kernel/irq.c:250
#8  0xffffffff81b94c46 in common_interrupt (regs=0xffffc90000013868, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
Backtrace stopped: Cannot access memory at address 0xffffc90000004010
```

可以很容易的找到 common_interrupt 的入口, 此时可以从硬件哪里拿到 error_code 来索引 vector_irq 获取 irqdesc

```c
DEFINE_PER_CPU(vector_irq_t, vector_irq) = {
	[0 ... NR_VECTORS - 1] = VECTOR_UNUSED,
};
```

## setup
- [x] i8042 之类如何设置的
- [x] nvme 如何设置的

- [ ] 看看 pci 是怎么分配的吧！

1. acpi 注册过程
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
2. floppy
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
3. ata_piix
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

4. nvme
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


5. i8042
```c
/*
>>> bt
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
```

6. serial8250
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

7. e1000
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

