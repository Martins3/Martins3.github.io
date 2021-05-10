# QOM

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

// 检测 mmio 的地址是确定的，果然如此
#0  huxueshi () at ../softmmu/memory.c:1188
#1  0x0000555555b928fa in memory_region_add_subregion_common (subregion=0x5555579fcf80, offset=4273733632, mr=0x55555681a700) at ../softmmu/memory.c:2478
#2  memory_region_add_subregion_overlap (mr=0x55555681a700, offset=4273733632, subregion=0x5555579fcf80, priority=<optimized out>) at ../softmmu/memory.c:2500
#3  0x000055555596b5a2 in pci_update_mappings (d=d@entry=0x5555579fa660) at ../hw/pci/pci.c:1371
#4  0x000055555596bd1c in pci_default_write_config (d=d@entry=0x5555579fa660, addr=addr@entry=4, val_in=val_in@entry=259, l=l@entry=2) at ../hw/pci/pci.c:1431
#5  0x0000555555909d68 in e1000_write_config (pci_dev=0x5555579fa660, address=4, val=259, len=2) at ../hw/net/e1000.c:1674
#6  0x0000555555877f1b in pci_host_config_write_common (pci_dev=0x5555579fa660, addr=4, limit=<optimized out>, val=259, len=2) at ../hw/pci/pci_host.c:83
#7  0x0000555555b8d7c0 in memory_region_write_accessor (mr=0x5555569dbd90, addr=0, value=<optimized out>, size=2, shift=<optimized out>, mask=<optimized out>, attrs=...
) at ../softmmu/memory.c:491
#8  0x0000555555b8c0ee in access_with_adjusted_size (addr=0, value=0x7fffd9ff90a8, size=2, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=0
x555555b8d730 <memory_region_write_accessor>, mr=0x5555569dbd90, attrs=...) at ../softmmu/memory.c:552
#9  0x0000555555b8fce7 in memory_region_dispatch_write (mr=mr@entry=0x5555569dbd90, addr=0, data=<optimized out>, op=<optimized out>, attrs=attrs@entry=...) at ../softm
mu/memory.c:1533
#10 0x0000555555bd7190 in flatview_write_continue (fv=fv@entry=0x7ffdcc1490a0, addr=addr@entry=3324, attrs=..., ptr=ptr@entry=0x7fffeb121000, len=len@entry=2, addr1=<op
timized out>, l=<optimized out>, mr=0x5555569dbd90) at /home/maritns3/core/kvmqemu/include/qemu/host-utils.h:164
#11 0x0000555555bd73a6 in flatview_write (fv=0x7ffdcc1490a0, addr=addr@entry=3324, attrs=attrs@entry=..., buf=buf@entry=0x7fffeb121000, len=len@entry=2) at ../softmmu/p
hysmem.c:2786
#12 0x0000555555bda066 in address_space_write (as=0x5555564e0ba0 <address_space_io>, addr=addr@entry=3324, attrs=..., buf=0x7fffeb121000, len=len@entry=2) at ../softmmu
/physmem.c:2878
#13 0x0000555555bda0fe in address_space_rw (as=<optimized out>, addr=addr@entry=3324, attrs=..., attrs@entry=..., buf=<optimized out>, len=len@entry=2, is_write=is_writ
e@entry=true) at ../softmmu/physmem.c:2888
#14 0x0000555555bb8939 in kvm_handle_io (count=1, size=2, direction=<optimized out>, data=<optimized out>, attrs=..., port=3324) at ../accel/kvm/kvm-all.c:2256
#15 kvm_cpu_exec (cpu=cpu@entry=0x5555569b9530) at ../accel/kvm/kvm-all.c:2507
#16 0x0000555555b77185 in kvm_vcpu_thread_fn (arg=arg@entry=0x5555569b9530) at ../accel/kvm/kvm-accel-ops.c:49
#17 0x0000555555d24983 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:521
#18 0x00007ffff6235609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#19 0x00007ffff615a293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95

*/
```
