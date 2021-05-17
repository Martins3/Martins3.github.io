# acpi 

- [MADT](https://wiki.osdev.org/MADT)
- [RSDT](https://wiki.osdev.org/RSDT)


- https://github.com/rust-osdev/about : 这个组织提供一堆可以用于 os dev 的工具，包括 uefi bootloader acpi
- https://github.com/acpica/acpica : acpi 框架的源代码 


acpi 的解析[^2]

原来kernel中最终是通过acpi_evaluate_object 来调用bios中在asl中定义好的函数啊 [^1]

uefi 提供了 ACPI 的文档

acpi 是内核上的设计


在 acpi_pci_root_add 中，调用 pci_acpi_scan_root
```
#0  acpi_pci_root_add (device=0xffff888100217000, not_used=0xffffffff82063c60 <root_device_ids>) at drivers/acpi/pci_root.c:517
#1  0xffffffff8146a1c3 in acpi_scan_attach_handler (device=0xffff888100217000) at drivers/acpi/scan.c:2038
#2  acpi_bus_attach (device=device@entry=0xffff888100217000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2086
#3  0xffffffff8146a110 in acpi_bus_attach (device=device@entry=0xffff888100216800, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#4  0xffffffff8146a110 in acpi_bus_attach (device=0xffff888100216000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#5  0xffffffff8146bf1f in acpi_bus_scan (handle=handle@entry=0xffffffffffffffff) at drivers/acpi/scan.c:2167
#6  0xffffffff82b699a2 in acpi_scan_init () at drivers/acpi/scan.c:2342
#7  0xffffffff82b696d3 in acpi_init () at drivers/acpi/bus.c:1341
#8  0xffffffff81000def in do_one_initcall (fn=0xffffffff82b692e1 <acpi_init>) at init/main.c:1249
#9  0xffffffff82b3726b in do_initcall_level (command_line=0xffff888100122400 "root", level=4) at ./include/linux/compiler.h:234
#10 do_initcalls () at init/main.c:1338
#11 do_basic_setup () at init/main.c:1358
#12 kernel_init_freeable () at init/main.c:1560
#13 0xffffffff81b6f3b9 in kernel_init (unused=<optimized out>) at init/main.c:1447
#14 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#15 0x0000000000000000 in ?? ()
```

pcibios_add_bus 就是 acpi_pci_add_bus，最后调用到 bios 的处理函数，利用 bios 的功能实现 pci host bridge 的查找:
```
#0  acpi_evaluate_object (handle=handle@entry=0xffff8881000efde0, pathname=pathname@entry=0xffffffff822a918e "_DSM", external_params=external_params@entry=0xffffc900000
13a80, return_buffer=return_buffer@entry=0xffffc90000013a70) at drivers/acpi/acpica/nsxfeval.c:167
#1  0xffffffff81464717 in acpi_evaluate_dsm (handle=handle@entry=0xffff8881000efde0, guid=guid@entry=0xffffffff8205f340 <pci_acpi_dsm_guid>, rev=rev@entry=3, func=func@
entry=8, argv4=argv4@entry=0x0 <fixed_percpu_data>) at drivers/acpi/utils.c:664
#2  0xffffffff81448751 in acpi_pci_add_bus (bus=0xffff888100259c00) at ./include/linux/acpi.h:40
#3  0xffffffff81430ee2 in pci_register_host_bridge (bridge=bridge@entry=0xffff888100259800) at drivers/pci/probe.c:944
#4  0xffffffff81431014 in pci_create_root_bus (parent=parent@entry=0x0 <fixed_percpu_data>, bus=bus@entry=0, ops=0xffffffff8263f640 <pci_root_ops>, sysdata=sysdata@entr
y=0xffff888100254338, resources=resources@entry=0xffff888100254318) at drivers/pci/probe.c:2979
#5  0xffffffff8147151e in acpi_pci_root_create (root=root@entry=0xffff88810021fa00, ops=ops@entry=0xffffffff8263f580 <acpi_pci_root_ops>, info=info@entry=0xffff88810025
4300, sysdata=sysdata@entry=0xffff888100254338) at drivers/acpi/pci_root.c:895
#6  0xffffffff81b179cd in pci_acpi_scan_root (root=root@entry=0xffff88810021fa00) at arch/x86/pci/acpi.c:368
#7  0xffffffff81b3542c in acpi_pci_root_add (device=0xffff888100217000, not_used=<optimized out>) at drivers/acpi/pci_root.c:597
#8  0xffffffff8146a1c3 in acpi_scan_attach_handler (device=0xffff888100217000) at drivers/acpi/scan.c:2038
#9  acpi_bus_attach (device=device@entry=0xffff888100217000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2086
#10 0xffffffff8146a110 in acpi_bus_attach (device=device@entry=0xffff888100216800, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#11 0xffffffff8146a110 in acpi_bus_attach (device=0xffff888100216000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#12 0xffffffff8146bf1f in acpi_bus_scan (handle=handle@entry=0xffffffffffffffff) at drivers/acpi/scan.c:2167
#13 0xffffffff82b699a2 in acpi_scan_init () at drivers/acpi/scan.c:2342
#14 0xffffffff82b696d3 in acpi_init () at drivers/acpi/bus.c:1341
#15 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b692e1 <acpi_init>) at init/main.c:1249
#16 0xffffffff82b3726b in do_initcall_level (command_line=0xffff888100122400 "root", level=4) at ./include/linux/compiler.h:234
#17 do_initcalls () at init/main.c:1338
#18 do_basic_setup () at init/main.c:1358
#19 kernel_init_freeable () at init/main.c:1560
#20 0xffffffff81b6f3b9 in kernel_init (unused=<optimized out>) at init/main.c:1447
#21 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#22 0x0000000000000000 in ?? ()
```

[^1]: https://blog.csdn.net/tiantao2012/article/details/73775993
[^2]: https://blog.csdn.net/woai110120130/article/details/93318611
[^3]: https://uefi.org/sites/default/files/resources/ACPI%206_2_A_Sept29.pdf
[^4]: https://www.kernel.org/doc/ols/2005/ols2005v1-pages-59-76.pdf
