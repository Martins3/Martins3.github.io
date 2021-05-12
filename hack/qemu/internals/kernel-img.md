# kernel image 是如何被加载的

```
>>> bt
#0  x86_load_linux (x86ms=x86ms@entry=0x5555566c0000, fw_cfg=fw_cfg@entry=0x555556a5b000, acpi_data_size=163840, pvh_enabled=true, linuxboot_dma_enabled=true) at ../hw/
i386/x86.c:766
#1  0x0000555555a620a2 in pc_memory_init (pcms=pcms@entry=0x5555566c0000, system_memory=system_memory@entry=0x5555566f6400, rom_memory=rom_memory@entry=0x5555568328d0,
ram_memory=ram_memory@entry=0x7fffffffd540) at ../hw/i386/pc.c:981
#2  0x0000555555a65ab1 in pc_init1 (machine=0x5555566c0000, pci_type=0x555555dbe5ad "i440FX", host_type=0x555555d80e54 "i440FX-pcihost") at ../hw/i386/pc_piix.c:187
#3  0x00005555558ff1ae in machine_run_board_init (machine=machine@entry=0x5555566c0000) at ../hw/core/machine.c:1232
#4  0x0000555555bae22e in qemu_init_board () at ../softmmu/vl.c:2514
#5  qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
#6  0x0000555555bb1e82 in qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3612
#7  0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```

- [ ] 最后，这个东西是怎么被 hardware 检测到的 e820_add_entry

- [ ] /home/maritns3/core/kvmqemu/hw/i386/kvmvapic.c 为什么提供一个 ROM
```c
option_rom kvmvapic.bin -1
option_rom linuxboot_dma.bin 0
```

似乎最开始运行的是 BIOS

这里，是选择默认的 firmware ，然后在 qemu_create_machine 中间初始化:
```
#0  pc_i440fx_machine_options (m=<optimized out>) at ../hw/i386/pc_piix.c:406
#1  pc_i440fx_6_0_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:421
#2  0x0000555555a63fd0 in pc_i440fx_5_2_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:432
#3  0x0000555555a640eb in pc_i440fx_5_1_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:446
#4  0x0000555555a641f0 in pc_i440fx_5_0_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:460
#5  0x0000555555a642f0 in pc_i440fx_4_2_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:474
#6  0x0000555555a64450 in pc_i440fx_4_1_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:486
#7  0x0000555555a64565 in pc_i440fx_4_0_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:499
#8  0x0000555555a64685 in pc_i440fx_3_1_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:514
#9  0x0000555555a64810 in pc_i440fx_3_0_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:529
#10 0x0000555555a648f0 in pc_i440fx_2_12_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:539
#11 0x0000555555a649d0 in pc_i440fx_2_11_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:549
#12 0x0000555555a64ab0 in pc_i440fx_2_10_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:559
#13 0x0000555555a64ba0 in pc_i440fx_2_9_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:570
#14 0x0000555555a64c80 in pc_i440fx_2_8_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:580
#15 0x0000555555a64d60 in pc_i440fx_2_7_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:590
#16 0x0000555555a64e65 in pc_i440fx_2_6_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:602
#17 0x0000555555a64f75 in pc_i440fx_2_5_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:616
#18 0x0000555555a65095 in pc_i440fx_2_4_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:630
#19 0x0000555555a65190 in pc_i440fx_2_3_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:642
#20 0x0000555555a652ab in pc_i440fx_2_2_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:655
#21 0x0000555555a653eb in pc_i440fx_2_1_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:670
#22 0x0000555555a65525 in pc_i440fx_2_0_machine_options (m=0x555556606170) at ../hw/i386/pc_piix.c:686
#23 0x0000555555a655cf in pc_machine_v2_0_class_init (oc=<optimized out>, data=<optimized out>) at ../hw/i386/pc_piix.c:711
#24 0x0000555555c9c32f in type_initialize (ti=0x55555659fda0) at ../qom/object.c:1069
#25 object_class_foreach_tramp (key=<optimized out>, value=0x55555659fda0, opaque=0x7fffffffd690) at ../qom/object.c:1069
#26 0x00007ffff6ed01b8 in g_hash_table_foreach () at /lib/x86_64-linux-gnu/libglib-2.0.so.0
#27 0x0000555555c9c96c in object_class_foreach (fn=fn@entry=0x555555c9b000 <object_class_get_list_tramp>, implements_type=implements_type@entry=0x55555612a330 "machine", include_abstract=include_abstract@entry=false, opaque=opaq ue@entry=0x7fffffffd6d0) at ../qom/object.c:85
#28 0x0000555555c9ca16 in object_class_get_list (implements_type=implements_type@entry=0x55555612a330 "machine", include_abstract=include_abstract@entry=false) at ../qom/object.c:1148
#29 0x0000555555baecb5 in select_machine () at ../softmmu/vl.c:3546
#30 qemu_init (argc=<optimized out>, argv=0x7fffffffd968, envp=<optimized out>) at ../softmmu/vl.c:3546
#31 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```

参数内核( -kernel ) 也是通过这种方法进行设置的:
```
>>> bt
#0  machine_set_firmware (obj=0x5555566c0000, value=0x555556945330 "bios-256k.bin", errp=0x7fffffffd610) at ../hw/core/machine.c:428
#1  0x0000555555c9b137 in property_set_str (obj=0x5555566c0000, v=<optimized out>, name=<optimized out>, opaque=0x5555565ef400, errp=0x7fffffffd610) at ../qom/object.c: 2180
#2  0x0000555555c9d74c in object_property_set (obj=0x5555566c0000, name=0x5555567e6af0 "firmware", v=0x555556979660, errp=0x5555564e2e30 <error_fatal>) at ../qom/object .c:1402
#3  0x0000555555c9dfc4 in object_property_parse (obj=obj@entry=0x5555566c0000, name=name@entry=0x5555567e6af0 "firmware", string=string@entry=0x5555567e6ab0 "bios-256k. bin", errp=errp@entry=0x5555564e2e30 <error_fatal>) at ../qom/object.c:1642
#4  0x0000555555bacf9f in object_parse_property_opt (skip=0x555555e104d6 "type", errp=0x5555564e2e30 <error_fatal>, value=0x5555567e6ab0 "bios-256k.bin", name=0x5555567 e6af0 "firmware", obj=0x5555566c0000) at ../softmmu/vl.c:1651
#5  object_parse_property_opt (errp=0x5555564e2e30 <error_fatal>, skip=0x555555e104d6 "type", value=0x5555567e6ab0 "bios-256k.bin", name=0x5555567e6af0 "firmware", obj= 0x5555566c0000) at ../softmmu/vl.c:1643
#6  machine_set_property (opaque=0x5555566c0000, name=0x5555567e6af0 "firmware", value=0x5555567e6ab0 "bios-256k.bin", errp=0x5555564e2e30 <error_fatal>) at ../softmmu/ vl.c:1693
#7  0x0000555555d2045d in qemu_opt_foreach (opts=opts@entry=0x5555565d0010, func=func@entry=0x555555bacec0 <machine_set_property>, opaque=0x5555566c0000, errp=errp@entr y=0x5555564e2e30 <error_fatal>) at ../util/qemu-option.c:593
#8  0x0000555555bb11e9 in qemu_apply_machine_options () at ../softmmu/vl.c:1812
#9  qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3554
#10 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```

从 info mtree 中看到，pc.bios 占据了 4G-256k 到 256k 之间的位置。

在 rom_reset 中间检测，实际上，ROM 的数量超乎想想:
```
[rom : kvmvapic.bin]
[rom : linuxboot_dma.bin]
[rom : bios-256k.bin]
[rom : etc/acpi/tables]
[rom : etc/table-loader]
[rom : etc/acpi/rsdp]
```

CPU 的状态初始化，很早的时候就开始了:
```
#0  x86_cpu_reset (dev=0x5555569c1d50) at ../target/i386/cpu.c:6109
#1  0x0000555555cae7c9 in resettable_phase_hold (obj=obj@entry=0x5555569c1d50, opaque=opaque@entry=0x0, type=type@entry=RESET_TYPE_COLD) at ../hw/core/resettable.c:182
#2  0x0000555555caef69 in resettable_assert_reset (obj=0x5555569c1d50, type=<optimized out>) at ../hw/core/resettable.c:60
#3  0x0000555555caf2fd in resettable_reset (obj=0x5555569c1d50, type=type@entry=RESET_TYPE_COLD) at ../hw/core/resettable.c:45
#4  0x0000555555cacc1b in device_cold_reset (dev=<optimized out>) at ../hw/core/qdev.c:345
#5  0x0000555555934b97 in cpu_reset (cpu=0x5555569c1d50) at /home/maritns3/core/kvmqemu/include/hw/qdev-core.h:17
#6  0x0000555555ad4a0c in x86_cpu_realizefn (dev=0x5555569c1d50, errp=0x7fffffffd380) at ../target/i386/cpu.c:6924
#7  0x0000555555cade57 in device_set_realized (obj=<optimized out>, value=true, errp=0x7fffffffd400) at ../hw/core/qdev.c:761
#8  0x0000555555c9b1aa in property_set_bool (obj=0x5555569c1d50, v=<optimized out>, name=<optimized out>, opaque=0x5555565d1db0, errp=0x7fffffffd400) at ../qom/object.c :2257
#9  0x0000555555c9d6bc in object_property_set (obj=obj@entry=0x5555569c1d50, name=name@entry=0x555555ed7fb6 "realized", v=v@entry=0x555556979410, errp=errp@entry=0x5555
564e2e30 <error_fatal>) at ../qom/object.c:1402
#10 0x0000555555c9f9f4 in object_property_set_qobject (obj=obj@entry=0x5555569c1d50, name=name@entry=0x555555ed7fb6 "realized", value=value@entry=0x555556944850, errp=e
rrp@entry=0x5555564e2e30 <error_fatal>) at ../qom/qom-qobject.c:28
#11 0x0000555555c9d909 in object_property_set_bool (obj=0x5555569c1d50, name=name@entry=0x555555ed7fb6 "realized", value=value@entry=true, errp=errp@entry=0x5555564e2e3
0 <error_fatal>) at ../qom/object.c:1472
#12 0x0000555555cacc82 in qdev_realize (dev=<optimized out>, bus=bus@entry=0x0, errp=errp@entry=0x5555564e2e30 <error_fatal>) at ../hw/core/qdev.c:389
#13 0x0000555555a79815 in x86_cpu_new (x86ms=x86ms@entry=0x5555566c0000, apic_id=0, errp=errp@entry=0x5555564e2e30 <error_fatal>) at /home/maritns3/core/kvmqemu/include
/hw/qdev-core.h:17
#14 0x0000555555a798fe in x86_cpus_init (x86ms=x86ms@entry=0x5555566c0000, default_cpu_version=<optimized out>) at ../hw/i386/x86.c:138
#15 0x0000555555a65a73 in pc_init1 (machine=0x5555566c0000, pci_type=0x555555dbe5ad "i440FX", host_type=0x555555d80e54 "i440FX-pcihost") at ../hw/i386/pc_piix.c:159
#16 0x00005555558ff1ae in machine_run_board_init (machine=machine@entry=0x5555566c0000) at ../hw/core/machine.c:1232
#17 0x0000555555bae1de in qemu_init_board () at ../softmmu/vl.c:2514
#18 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
#19 0x0000555555bb1e32 in qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3612
#20 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```
