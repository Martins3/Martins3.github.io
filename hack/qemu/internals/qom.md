# QOM

## TODO
- [ ] QOM 不是存在一个标准的教学吗?

- [ ] 到底存在那几个关键概念
  - Type
  - Class
  - ?

- [ ] 一个 ObjectProperty 和普通的函数有什么区别啊 ?
  - 为什么还有普通的指针啊 ?

```c
struct ObjectProperty
{
    char *name;
    char *type;
    char *description;
    ObjectPropertyAccessor *get;
    ObjectPropertyAccessor *set;
    ObjectPropertyResolve *resolve;
    ObjectPropertyRelease *release;
    ObjectPropertyInit *init;
    void *opaque;
    QObject *defval;
};
```
  

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

qdev_realize_and_unref
```c
/**
 * DeviceState:
 * @realized: Indicates whether the device has been fully constructed.
 *            When accessed outside big qemu lock, must be accessed with
 *            qatomic_load_acquire()
 * @reset: ResettableState for the device; handled by Resettable interface.
 *
 * This structure should not be accessed directly.  We declare it here
 * so that it can be embedded in individual device state structures.
 */
struct DeviceState {
    /*< private >*/
    Object parent_obj;
    /*< public >*/

    const char *id;
    char *canonical_path;
    bool realized;
    bool pending_deleted_event;
    QemuOpts *opts;
    int hotplugged;
    bool allow_unplug_during_migration;
    BusState *parent_bus;
    QLIST_HEAD(, NamedGPIOList) gpios;
    QLIST_HEAD(, NamedClockList) clocks;
    QLIST_HEAD(, BusState) child_bus;
    int num_child_bus;
    int instance_id_alias;
    int alias_required_for_version;
    ResettableState reset;
};

/**
 * BusState:
 * @hotplug_handler: link to a hotplug handler associated with bus.
 * @reset: ResettableState for the bus; handled by Resettable interface.
 */
struct BusState {
    Object obj;
    DeviceState *parent;
    char *name;
    HotplugHandler *hotplug_handler;
    int max_index;
    bool realized;
    int num_children;

    /*
     * children is a RCU QTAILQ, thus readers must use RCU to access it,
     * and writers must hold the big qemu lock
     */

    QTAILQ_HEAD(, BusChild) children;
    QLIST_ENTRY(BusState) sibling;
    ResettableState reset;
};
```
实际上，qdev_realize_and_unref 都是各种总线模块的调用，比如 pci, 也就是说，代码设计上，就是设备和总线关联起来的。

- qdev_realize_and_unref
  - qdev_realize
    - qdev_set_parent_bus : bus 和 dev 的关系确定
    - object_property_set_bool(OBJECT(dev), "realized", true, errp);
      - object_property_set_qobject
        - qobject_input_visitor_new : *TODO* 真 NM 离谱, 将 Qbool 作为参数，创建 Visitor
        - object_property_set
           - object_property_find_err
              - object_property_find 
                  - object_get_class
                  - object_class_property_find : 递归的查找这个 property
                  - g_hash_table_lookup
  - object_unref

```c
/*
>>> bt
#0  object_property_try_add (obj=0x5555566c0000, name=0x5555567dc8c0 "peripheral", type=0x5555567dc990 "child<container>", get=0x555555c9eac0 <object_get_child_property
>, set=0x0, release=0x555555c9cc70 <object_finalize_child_property>, opaque=0x5555567dc900, errp=0x5555564e2e38 <error_abort>) at ../qom/object.c:1196
#1  0x0000555555c9e401 in object_property_try_add_child (obj=0x5555566c0000, name=0x5555567dc8c0 "peripheral", child=0x5555567dc900, errp=0x5555564e2e38 <error_abort>)
at ../qom/object.c:1744
#2  0x0000555555c99e25 in container_get (root=root@entry=0x5555566c0000, path=path@entry=0x555555d72051 "/peripheral") at ../qom/container.c:41
#3  0x00005555558ff749 in machine_initfn (obj=0x5555566c0000) at ../hw/core/machine.c:923
#4  0x0000555555c9b7e6 in object_init_with_type (obj=0x5555566c0000, ti=0x5555565a1d70) at ../qom/object.c:371
#5  0x0000555555c9b7e6 in object_init_with_type (obj=0x5555566c0000, ti=0x55555659d180) at ../qom/object.c:371
#6  0x0000555555c9b7e6 in object_init_with_type (obj=0x5555566c0000, ti=0x55555659d8e0) at ../qom/object.c:371
#7  0x0000555555c9ce2c in object_initialize_with_type (obj=0x5555566c0000, size=<optimized out>, type=0x55555659d8e0) at ../qom/object.c:517
#8  0x0000555555c9cf79 in object_new_with_type (type=0x55555659d8e0) at ../qom/object.c:732
#9  0x0000555555baf103 in qemu_create_machine (machine_class=0x5555567978b0) at ../softmmu/vl.c:2067
#10 qemu_init (argc=<optimized out>, argv=0x7fffffffd968, envp=<optimized out>) at ../softmmu/vl.c:3545
#11 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```



## 实现方法


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

PIIX3_PCI_DEVICE 是如何将一个 parent 类型 pci_dev 转化为 piix3 的
```c
pci_dev = pci_create_simple_multifunction(pci_bus, -1, true,
                                          TYPE_PIIX3_DEVICE);
piix3 = PIIX3_PCI_DEVICE(pci_dev);
```


这个例子就是简直有毒了
1. isa_create_simple 的一个参数 name，可以找到对应的类，然后将类的初始化函数进行调用

```c
>>> bt
#0  i8042_realizefn (dev=0x5555569fb420, errp=0x7fffffffd310) at ../hw/input/pckbd.c:547
#1  0x0000555555cade27 in device_set_realized (obj=<optimized out>, value=true, errp=0x7fffffffd390) at ../hw/core/qdev.c:761
#2  0x0000555555c9b17a in property_set_bool (obj=0x5555569fb420, v=<optimized out>, name=<optimized out>, opaque=0x5555565d1db0, errp=0x7fffffffd390) at ../qom/object.c
:2257
#3  0x0000555555c9d68c in object_property_set (obj=obj@entry=0x5555569fb420, name=name@entry=0x555555ed7f76 "realized", v=v@entry=0x5555568bc7a0, errp=errp@entry=0x5555
564e2e30 <error_fatal>) at ../qom/object.c:1402
#4  0x0000555555c9f9c4 in object_property_set_qobject (obj=obj@entry=0x5555569fb420, name=name@entry=0x555555ed7f76 "realized", value=value@entry=0x555556a06fa0, errp=e
rrp@entry=0x5555564e2e30 <error_fatal>) at ../qom/qom-qobject.c:28
#5  0x0000555555c9d8d9 in object_property_set_bool (obj=0x5555569fb420, name=0x555555ed7f76 "realized", value=<optimized out>, errp=0x5555564e2e30 <error_fatal>) at ../
qom/object.c:1472
#6  0x0000555555caccf3 in qdev_realize_and_unref (dev=dev@entry=0x5555569fb420, bus=bus@entry=0x5555568a28a0, errp=<optimized out>) at ../hw/core/qdev.c:396
#7  0x000055555590c2e9 in isa_realize_and_unref (dev=dev@entry=0x5555569fb420, bus=bus@entry=0x5555568a28a0, errp=<optimized out>) at ../hw/isa/isa-bus.c:179
#8  0x000055555590c31b in isa_create_simple (bus=bus@entry=0x5555568a28a0, name=name@entry=0x555555d76226 "i8042") at ../hw/isa/isa-bus.c:173
#9  0x0000555555a62751 in pc_superio_init (no_vmport=false, create_fdctrl=<optimized out>, isa_bus=0x5555568a28a0) at ../hw/i386/pc.c:1079
#10 pc_basic_device_init (pcms=pcms@entry=0x5555566c0000, isa_bus=0x5555568a28a0, gsi=<optimized out>, rtc_state=rtc_state@entry=0x7fffffffd538, create_fdctrl=create_fd
ctrl@entry=true, hpet_irqs=hpet_irqs@entry=4) at ../hw/i386/pc.c:1174
#11 0x0000555555a65bdd in pc_init1 (machine=0x5555566c0000, pci_type=0x555555dbe5ad "i440FX", host_type=0x555555d80e54 "i440FX-pcihost") at ../hw/i386/pc_piix.c:241
#12 0x00005555558ff1ae in machine_run_board_init (machine=machine@entry=0x5555566c0000) at ../hw/core/machine.c:1232
#13 0x0000555555bae1be in qemu_init_board () at ../softmmu/vl.c:2514
#14 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
#15 0x0000555555bb1e02 in qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3611
#16 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```

然后这些东西其实都是靠这个初始化的:
```c
static void i8042_class_initfn(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    ISADeviceClass *isa = ISA_DEVICE_CLASS(klass);

    dc->realize = i8042_realizefn;
    dc->vmsd = &vmstate_kbd_isa;
    isa->build_aml = i8042_build_aml;
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static const TypeInfo i8042_info = {
    .name          = TYPE_I8042,
    .parent        = TYPE_ISA_DEVICE,
    .instance_size = sizeof(ISAKBDState),
    .instance_init = i8042_initfn,
    .class_init    = i8042_class_initfn,
};

static void i8042_register_types(void)
{
    type_register_static(&i8042_info);
}
```

