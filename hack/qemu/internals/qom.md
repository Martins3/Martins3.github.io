# QOM

## TODO
- [ ] QOM 不是存在一个标准的教学吗?

## material
- [ ] 从一个普通的调用变为 pci_e1000_realize
```c
/**
// 检测这个 memory region 是如何被加入的, 这个时候的大小是确定的
#0  huxueshi () at ../softmmu/memory.c:1188
#1  0x0000555555b8eeb5 in memory_region_init (mr=0x5555579fa5c0, owner=0x5555579f7ca0, name=<optimized out>, size=131072) at ../softmmu/memory.c:1198
#2  0x0000555555b8ef5c in memory_region_init_io (mr=mr@entry=0x5555579fa5c0, owner=owner@entry=0x5555579f7ca0, ops=ops@entry=0x5555562d9520 <e1000_mmio_ops>, opaque=opa que@entry=0x5555579f7ca0, name=name@entry=0x555555dc1976 "e1000-mmio", size=size@entry=131072) at ../softmmu/memory.c:1532
#3  0x000055555590a682 in e1000_mmio_setup (d=0x5555579f7ca0) at ../hw/net/e1000.c:1640
#4  pci_e1000_realize (pci_dev=0x5555579f7ca0, errp=<optimized out>) at ../hw/net/e1000.c:1698
#5  0x000055555596eb91 in pci_qdev_realize (qdev=0x5555579f7ca0, errp=<optimized out>) at ../hw/pci/pci.c:2117
#6  0x0000555555cadec7 in device_set_realized (obj=<optimized out>, value=true, errp=0x7fffffffd300) at ../hw/core/qdev.c:761
#7  0x0000555555c9b21a in property_set_bool (obj=0x5555579f7ca0, v=<optimized out>, name=<optimized out>, opaque=0x5555565d1db0, errp=0x7fffffffd300) at ../qom/object.c :2257
#8  0x0000555555c9d72c in object_property_set (obj=obj@entry=0x5555579f7ca0, name=name@entry=0x555555ed7f96 "realized", v=v@entry=0x555556dbfe10, errp=errp@entry=0x5555 564e2e30 <error_fatal>) at ../qom/object.c:1402
#9  0x0000555555c9fa64 in object_property_set_qobject (obj=obj@entry=0x5555579f7ca0, name=name@entry=0x555555ed7f96 "realized", value=value@entry=0x555556d9d790, errp=e rrp@entry=0x5555564e2e30 <error_fatal>) at ../qom/qom-qobject.c:28
#10 0x0000555555c9d979 in object_property_set_bool (obj=0x5555579f7ca0, name=0x555555ed7f96 "realized", value=<optimized out>, errp=0x5555564e2e30 <error_fatal>) at ../ qom/object.c:1472
#11 0x0000555555cacd93 in qdev_realize_and_unref (dev=dev@entry=0x5555579f7ca0, bus=bus@entry=0x555556e0f800, errp=<optimized out>) at ../hw/core/qdev.c:396
#12 0x000055555596d209 in pci_realize_and_unref (dev=dev@entry=0x5555579f7ca0, bus=bus@entry=0x555556e0f800, errp=<optimized out>) at ../hw/pci/pci.c:2182
#13 0x000055555596d437 in pci_nic_init_nofail (nd=nd@entry=0x5555564c4300 <nd_table>, rootbus=rootbus@entry=0x555556e0f800, default_model=default_model@entry=0x555555d7 227c "e1000", default_devaddr=default_devaddr@entry=0x0) at ../hw/pci/pci.c:1957
#14 0x0000555555a62b20 in pc_nic_init (pcmc=pcmc@entry=0x5555567978b0, isa_bus=0x5555568b4980, pci_bus=pci_bus@entry=0x555556e0f800) at ../hw/i386/pc.c:1189
#15 0x0000555555a65bed in pc_init1 (machine=0x5555566c0000, pci_type=0x555555dbe5ad "i440FX", host_type=0x555555d80e54 "i440FX-pcihost") at ../hw/i386/pc_piix.c:244
#16 0x00005555558ff1ae in machine_run_board_init (machine=machine@entry=0x5555566c0000) at ../hw/core/machine.c:1232
#17 0x0000555555bae25e in qemu_init_board () at ../softmmu/vl.c:2514
#18 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
#19 0x0000555555bb1ea2 in qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3611
#20 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```

添加 str 的方法有点别致
```c
    object_class_property_add_str(oc, "kernel",
        machine_get_kernel, machine_set_kernel);
```

相当于创建了 option 和 method 之间的方法:
```c
/**
#0  machine_set_kernel (obj=0x5555566c0000, value=0x5555569562a0 "/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage", errp=0x7fffffffd610) at ../hw/core/machine.c: 244
#1  0x0000555555c9b0f7 in property_set_str (obj=0x5555566c0000, v=<optimized out>, name=<optimized out>, opaque=0x5555565ee8a0, errp=0x7fffffffd610) at ../qom/object.c: 2180
#2  0x0000555555c9d70c in object_property_set (obj=0x5555566c0000, name=0x5555565cfff0 "kernel", v=0x5555568e25c0, errp=0x5555564e2e30 <error_fatal>) at ../qom/object.c :1402
#3  0x0000555555c9df84 in object_property_parse (obj=obj@entry=0x5555566c0000, name=name@entry=0x5555565cfff0 "kernel", string=string@entry=0x5555565d00c0 "/home/maritn s3/core/ubuntu-linux/arch/x86/boot/bzImage", errp=errp@entry=0x5555564e2e30 <error_fatal>) at ../qom/object.c:1642
#4  0x0000555555bacf5f in object_parse_property_opt (skip=0x555555e104d6 "type", errp=0x5555564e2e30 <error_fatal>, value=0x5555565d00c0 "/home/maritns3/core/ubuntu-lin ux/arch/x86/boot/bzImage", name=0x5555565cfff0 "kernel", obj=0x5555566c0000) at ../softmmu/vl.c:1651
#5  object_parse_property_opt (errp=0x5555564e2e30 <error_fatal>, skip=0x555555e104d6 "type", value=0x5555565d00c0 "/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzIma ge", name=0x5555565cfff0 "kernel", obj=0x5555566c0000) at ../softmmu/vl.c:1643
#6  machine_set_property (opaque=0x5555566c0000, name=0x5555565cfff0 "kernel", value=0x5555565d00c0 "/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage", errp=0x555 5564e2e30 <error_fatal>) at ../softmmu/vl.c:1693
#7  0x0000555555d2041d in qemu_opt_foreach (opts=opts@entry=0x5555565d0010, func=func@entry=0x555555bace80 <machine_set_property>, opaque=0x5555566c0000, errp=errp@entr y=0x5555564e2e30 <error_fatal>) at ../util/qemu-option.c:593
#8  0x0000555555bb11a9 in qemu_apply_machine_options () at ../softmmu/vl.c:1812
#9  qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3554
#10 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
*/
```

各种 realized 函数, 最后被调用的方式为:

```c
return object_property_set_bool(OBJECT(dev), "realized", true, errp);
```

