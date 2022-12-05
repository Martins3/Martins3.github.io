# hotplug

# [kernel doc](https://www.kernel.org/doc/html/latest/firmware-guide/acpi/index.html)

- 使用 make menuconfig 大致分析一下一共都存在什么功能吧

## acpi 中断的过程

- 在 memory hotplug 总是出现如下的 backtrace ，所以中断哪里是:

```txt
#15 0xffffffff816cfa05 in acpi_hotplug_work_fn (work=0xffff8880093142c0) at drivers/acpi/osl.c:1162
#16 0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff888003ab83c0, work=0xffff8880093142c0) at kernel/workqueue.c:2289
#17 0xffffffff811232c8 in worker_thread (__worker=0xffff888003ab83c0) at kernel/workqueue.c:2436
#18 0xffffffff81129c73 in kthread (_create=0xffff888003aaa400) at kernel/kthread.c:376
#19 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

cat /proc/interrupt 中可以看到这一行:
```txt
9:          2   IO-APIC   9-fasteoi   acpi
```

进一步的检查 docs/qemu/interrupt.md 的文档，找到 acpi 调用 `acpi_os_install_interrupt_handler` 将 `acpi_irq` 注册为中断。

真的是对于 workqueue 使用的行云流水啊:
```txt
#0  acpi_hotplug_schedule (adev=adev@entry=0xffff888003b64800, src=src@entry=1) at include/linux/slab.h:600
#1  0xffffffff816d5c9f in acpi_bus_notify (handle=0xffff888003af0420, type=1, data=<optimized out>) at drivers/acpi/bus.c:531
#2  0xffffffff816edc4f in acpi_ev_notify_dispatch (context=0xffff8881001e8410) at drivers/acpi/acpica/evmisc.c:171
#3  0xffffffff816cf941 in acpi_os_execute_deferred (work=0xffff88810016e750) at drivers/acpi/osl.c:850
#4  0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff888008151cc0, work=0xffff88810016e750) at kernel/workqueue.c:2289
#5  0xffffffff811232c8 in worker_thread (__worker=0xffff888008151cc0) at kernel/workqueue.c:2436
#6  0xffffffff81129c73 in kthread (_create=0xffff88810016e640) at kernel/kthread.c:376
#7  0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#8  0x0000000000000000 in ?? ()
```

```txt
#0  acpi_os_execute (type=type@entry=OSL_GPE_HANDLER, function=function@entry=0xffffffff816ec615 <acpi_ev_asynch_execute_gpe_method>, context=context@entry=0xffff888003afb048) at drivers/acpi/osl.c:1074
#1  0xffffffff816ec7fd in acpi_ev_gpe_dispatch (gpe_device=gpe_device@entry=0xffff8880039170c0, gpe_event_info=gpe_event_info@entry=0xffff888003afb048, gpe_number=gpe_number@entry=3) at drivers/acpi/acpica/evgpe.c:823
#2  0xffffffff816ec96d in acpi_ev_detect_gpe (gpe_device=gpe_device@entry=0xffff8880039170c0, gpe_event_info=gpe_event_info@entry=0xffff888003afb048, gpe_number=gpe_number@entry=3) at drivers/acpi/acpica/evgpe.c:723
#3  0xffffffff816eca49 in acpi_ev_gpe_detect (gpe_xrupt_list=gpe_xrupt_list@entry=0xffff888003aacac0) at drivers/acpi/acpica/evgpe.c:424
#4  0xffffffff816eebb9 in acpi_ev_sci_xrupt_handler (context=0xffff888003aacac0) at drivers/acpi/acpica/evsci.c:98
#5  0xffffffff816cfa23 in acpi_irq (irq=<optimized out>, dev_id=<optimized out>) at drivers/acpi/osl.c:549
#6  0xffffffff811658d1 in __handle_irq_event_percpu (desc=desc@entry=0xffff888003918a00) at kernel/irq/handle.c:158
#7  0xffffffff81165a7f in handle_irq_event_percpu (desc=0xffff888003918a00) at kernel/irq/handle.c:193
#8  handle_irq_event (desc=desc@entry=0xffff888003918a00) at kernel/irq/handle.c:210
#9  0xffffffff81169c1b in handle_fasteoi_irq (desc=0xffff888003918a00) at kernel/irq/chip.c:714
#10 0xffffffff810b9ad4 in generic_handle_irq_desc (desc=0xffff888003918a00) at include/linux/irqdesc.h:158
#11 handle_irq (regs=<optimized out>, desc=0xffff888003918a00) at arch/x86/kernel/irq.c:231
#12 __common_interrupt (regs=<optimized out>, vector=33) at arch/x86/kernel/irq.c:250
#13 0xffffffff81ef93f3 in common_interrupt (regs=0xffffffff82a03de8, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
Backtrace stopped: Cannot access memory at address 0xffffc90000004018
```

我不是很懂这里的设计，在 acpi_os_execute 中使用 workqueue 一直执行到 acpi_hotplug_schedule ，然后再次使用
执行到 acpi_hotplug_work_fn ，也许是因为不在乎效率吧。

## 其他
- [ ] https://unix.stackexchange.com/questions/242013/disable-gpe-acpi-interrupts-on-boot


## [ ] 在物理机上 hotplug usb 的 backtrace 是什么样子的
