# QOM

## TODO
- [ ] QOM 不是存在一个标准的教学吗?

- [ ] 到底存在那几个关键概念
  - Type
  - Class
  - ?

- [ ] 一个 ObjectProperty 和普通的函数有什么区别啊 ?
  - 为什么还有普通的指针啊 ?

- [ ] object_class_property_init_all 函数是不是说明，其实还存在 class property

关于 QOM 的进一步参考
- [ ] http://juniorprincewang.github.io/categories/QEMU/
- [ ] https://qemu.readthedocs.io/en/latest/devel/qom.html

## Type

```c
/**
>>> bt
#0  type_new (info=0x55555625b1a0 <cirrus_vga_info>) at ../qom/object.c:103
#1  0x0000555555c9be72 in type_register_internal (info=<optimized out>) at ../qom/object.c:150
#2  type_register (info=<optimized out>) at ../qom/object.c:150
#3  0x0000555555d28b62 in module_call_init (type=type@entry=MODULE_INIT_QOM) at ../util/module.c:106
#4  0x0000555555ae9928 in qemu_init_subsystems () at ../softmmu/runstate.c:761
#5  0x0000555555bae7b0 in qemu_init (argc=14, argv=0x7fffffffd968, envp=<optimized out>) at ../softmmu/vl.c:2666
#6  0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
*/
```
似乎各种 Type 注册函数都只是调用一下 type_register_static，最后就是 type 加入到 hash table 中，并且进行初始化


```c
void qemu_init_subsystems(void)
{
    Error *err;

    os_set_line_buffering();

    module_call_init(MODULE_INIT_TRACE);

    qemu_init_cpu_list();
    qemu_init_cpu_loop();
    qemu_mutex_lock_iothread();

    atexit(qemu_run_exit_notifiers);

    module_call_init(MODULE_INIT_QOM);
    module_call_init(MODULE_INIT_MIGRATION);

    runstate_init();
    precopy_infrastructure_init();
    postcopy_infrastructure_init();
    monitor_init_globals();

    if (qcrypto_init(&err) < 0) {
        error_reportf_err(err, "cannot initialize crypto: ");
        exit(1);
    }

    os_setup_early_signal_handling();

    bdrv_init_with_whitelist();
    socket_init();
}
```

然后调用 `module_call_init`，参数是 MODULE_INIT_QOM

module_call_init 会调用这个 module 注册的所有的 init，函数的注册位置: `register_module_init`

加入对应模块的队列中间:
```c
void register_module_init(void (*fn)(void), module_init_type type)
{
    ModuleEntry *e;
    ModuleTypeList *l;

    e = g_malloc0(sizeof(*e));
    e->init = fn;
    e->type = type;

    l = find_type(type);

    QTAILQ_INSERT_TAIL(l, e, node);
}
```
这个函数存在 500 多个调用的位置


```c
struct TypeInfo
{
    const char *name;
    const char *parent;

    size_t instance_size;
    size_t instance_align;
    void (*instance_init)(Object *obj);
    void (*instance_post_init)(Object *obj);
    void (*instance_finalize)(Object *obj);

    bool abstract;
    size_t class_size;

    void (*class_init)(ObjectClass *klass, void *data);
    void (*class_base_init)(ObjectClass *klass, void *data);
    void *class_data;

    InterfaceInfo *interfaces;
};
```

TypeInfo 的 instance_init / class_init 是如何初始化的? 使用 nvme 作为例子:

- 非常的离谱，调用 object_class_get_list 的时候居然会初始化所有的 type, 进而初始化 class 

```c
/*
>>> bt
#0  nvme_class_init (oc=0x55555674a270, data=0x0) at ../hw/block/nvme.c:6312
#1  0x0000555555c9c29f in type_initialize (ti=0x555556588e30) at ../qom/object.c:1079
#2  object_class_foreach_tramp (key=<optimized out>, value=0x555556588e30, opaque=0x7fffffffd600) at ../qom/object.c:1079
#3  0x00007ffff6ed01b8 in g_hash_table_foreach () at /lib/x86_64-linux-gnu/libglib-2.0.so.0
#4  0x0000555555c9c8dc in object_class_foreach (fn=fn@entry=0x555555c9af70 <object_class_get_list_tramp>, implements_type=implements_type@entry=0x55555612a2d0 "machine" , include_abstract=include_abstract@entry=false, opaque=opaque@entry=0x7fffffffd640) at ../qom/object.c:85
#5  0x0000555555c9c986 in object_class_get_list (implements_type=implements_type@entry=0x55555612a2d0 "machine", include_abstract=include_abstract@entry=false) at ../qo m/object.c:1158
#6  0x0000555555baec35 in select_machine () at ../softmmu/vl.c:3545
#7  qemu_init (argc=<optimized out>, argv=0x7fffffffd8d8, envp=<optimized out>) at ../softmmu/vl.c:3545
#8  0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49

#0  nvme_instance_init (obj=0x555557c93920) at ../hw/block/nvme.c:6328
#1  0x0000555555c9ce2c in object_initialize_with_type (obj=obj@entry=0x555557c93920, size=size@entry=18496, type=type@entry=0x555556588e30) at ../qom/object.c:527
#2  0x0000555555c9cf79 in object_new_with_type (type=0x555556588e30) at ../qom/object.c:742
#3  0x0000555555cac8fa in qdev_new (name=name@entry=0x5555565d01e0 "nvme") at ../hw/core/qdev.c:153
#4  0x0000555555858844 in qdev_device_add (opts=0x5555565d02d0, errp=errp@entry=0x5555564e2e30 <error_fatal>) at ../softmmu/qdev-monitor.c:650
#5  0x0000555555babb53 in device_init_func (opaque=<optimized out>, opts=<optimized out>, errp=0x5555564e2e30 <error_fatal>) at ../softmmu/vl.c:1211
#6  0x0000555555d20fb2 in qemu_opts_foreach (list=<optimized out>, func=func@entry=0x555555babb40 <device_init_func>, opaque=opaque@entry=0x0, errp=errp@entry=0x5555564 e2e30 <error_fatal>) at ../util/qemu-option.c:1167
#7  0x0000555555bae255 in qemu_create_cli_devices () at ../softmmu/vl.c:2541
#8  qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2589
#9  0x0000555555bb1e02 in qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3611
#10 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
*/
```


## object_property : pci_e1000_realize

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
*/
```

使用 -kernel 作为例子:
```c
    object_class_property_add_str(oc, "kernel", machine_get_kernel, machine_set_kernel);
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

- [ ] 关于 object_property 的唯一问题在于，总是从 parent 中间搜索的，之后才到 child 中间找


## 类型转换

PIIX3_PCI_DEVICE 是如何将一个 parent 类型 pci_dev 转化为 piix3 的
```c
pci_dev = pci_create_simple_multifunction(pci_bus, -1, true, TYPE_PIIX3_DEVICE);
piix3 = PIIX3_PCI_DEVICE(pci_dev);
```

在创建 pci_create_simple_multifunction 的参数, TYPE_PIIX3_DEVICE 可以保证创建出来的对象就是一个 piix3

而 PIIX3_PCI_DEVICE 应该只是进行在 child 到 parent 之间的转换使用上了检查吧


## isa 作为例子

1. isa_create_simple 的一个参数 name，可以找到对应的类，然后将类的初始化函数进行调用
2. 这是因为 type 定义的时候会将 name 加入到 hashtable 中，然后就可以找到函数定义

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

## qom-tree

```c
/*
(qemu) info qom-tree
/machine (pc-i440fx-6.0-machine)
  /fw_cfg (fw_cfg_io)
    /\x2from@etc\x2facpi\x2frsdp[0] (memory-region)
    /\x2from@etc\x2facpi\x2ftables[0] (memory-region)
    /\x2from@etc\x2ftable-loader[0] (memory-region)
    /fwcfg.dma[0] (memory-region)
    /fwcfg[0] (memory-region)
  /i440fx (i440FX-pcihost)
    /ioapic (kvm-ioapic)
      /kvm-ioapic[0] (memory-region)
      /unnamed-gpio-in[0] (irq)
      /unnamed-gpio-in[10] (irq)
      /unnamed-gpio-in[11] (irq)
      /unnamed-gpio-in[12] (irq)
      /unnamed-gpio-in[13] (irq)
      /unnamed-gpio-in[14] (irq)
      /unnamed-gpio-in[15] (irq)
      /unnamed-gpio-in[16] (irq)
      /unnamed-gpio-in[17] (irq)
      /unnamed-gpio-in[18] (irq)
      /unnamed-gpio-in[19] (irq)
      /unnamed-gpio-in[1] (irq)
      /unnamed-gpio-in[20] (irq)
      /unnamed-gpio-in[21] (irq)
      /unnamed-gpio-in[22] (irq)
      /unnamed-gpio-in[23] (irq)
      /unnamed-gpio-in[2] (irq)
      /unnamed-gpio-in[3] (irq)
      /unnamed-gpio-in[4] (irq)
      /unnamed-gpio-in[5] (irq)
      /unnamed-gpio-in[6] (irq)
      /unnamed-gpio-in[7] (irq)
      /unnamed-gpio-in[8] (irq)
      /unnamed-gpio-in[9] (irq)
    /pam-pci[0] (memory-region)
    /pam-pci[10] (memory-region)
    /pam-pci[11] (memory-region)
    /pam-pci[12] (memory-region)
    /pam-pci[13] (memory-region)
    /pam-pci[14] (memory-region)
    /pam-pci[15] (memory-region)
    /pam-pci[16] (memory-region)
    /pam-pci[17] (memory-region)
    /pam-pci[18] (memory-region)
    /pam-pci[19] (memory-region)
    /pam-pci[1] (memory-region)
    /pam-pci[20] (memory-region)
    /pam-pci[21] (memory-region)
    /pam-pci[22] (memory-region)
    /pam-pci[23] (memory-region)
    /pam-pci[24] (memory-region)
    /pam-pci[25] (memory-region)
    /pam-pci[2] (memory-region)
    /pam-pci[3] (memory-region)
    /pam-pci[4] (memory-region)
    /pam-pci[5] (memory-region)
    /pam-pci[6] (memory-region)
    /pam-pci[7] (memory-region)
    /pam-pci[8] (memory-region)
    /pam-pci[9] (memory-region)
    /pam-ram[0] (memory-region)
    /pam-ram[10] (memory-region)
    /pam-ram[11] (memory-region)
    /pam-ram[12] (memory-region)
    /pam-ram[1] (memory-region)
    /pam-ram[2] (memory-region)
    /pam-ram[3] (memory-region)
    /pam-ram[4] (memory-region)
    /pam-ram[5] (memory-region)
    /pam-ram[6] (memory-region)
    /pam-ram[7] (memory-region)
    /pam-ram[8] (memory-region)
    /pam-ram[9] (memory-region)
    /pam-rom[0] (memory-region)
    /pam-rom[10] (memory-region)
    /pam-rom[11] (memory-region)
    /pam-rom[12] (memory-region)
    /pam-rom[1] (memory-region)
    /pam-rom[2] (memory-region)
    /pam-rom[3] (memory-region)
    /pam-rom[4] (memory-region)
    /pam-rom[5] (memory-region)
    /pam-rom[6] (memory-region)
    /pam-rom[7] (memory-region)
    /pam-rom[8] (memory-region)
    /pam-rom[9] (memory-region)
    /pci-conf-data[0] (memory-region)
    /pci-conf-idx[0] (memory-region)
    /pci.0 (PCI)
  /peripheral (container)
  /peripheral-anon (container)
    /device[0] (isa-debugcon)
      /isa-debugcon[0] (memory-region)
    /device[1] (nvme)                            // 实际上，nvme 和 e1000 根本不是对称的
      /bus master container[0] (memory-region)
      /bus master[0] (memory-region)
      /msix-pba[0] (memory-region)
      /msix-table[0] (memory-region)
      /nvme-bar0[0] (memory-region)
      /nvme-bus.0 (nvme-bus)
      /nvme[0] (memory-region)
  /unattached (container)                         // 所以，什么是 unattached 的 
    /device[0] (host-x86_64-cpu)
      /lapic (kvm-apic)
        /kvm-apic-msi[0] (memory-region)
    /device[10] (kvm-pit)
      /kvm-pit[0] (memory-region)
      /unnamed-gpio-in[0] (irq)
    /device[11] (isa-pcspk)
      /pcspk[0] (memory-region)
    /device[12] (i8257)
      /dma-chan[0] (memory-region)
      /dma-cont[0] (memory-region)
      /dma-page[0] (memory-region)
      /dma-page[1] (memory-region)
    /device[13] (i8257)
      /dma-chan[0] (memory-region)
      /dma-cont[0] (memory-region)
      /dma-page[0] (memory-region)
      /dma-page[1] (memory-region)
    /device[14] (isa-serial)
      /serial (serial)
      /serial[0] (memory-region)
    /device[15] (isa-parallel)
      /parallel[0] (memory-region)
    /device[16] (isa-fdc)
      /fdc[0] (memory-region)
      /fdc[1] (memory-region)
      /floppy-bus.0 (floppy-bus)
    /device[17] (floppy)
    /device[18] (i8042)
      /i8042-cmd[0] (memory-region)
      /i8042-data[0] (memory-region)
    /device[19] (vmport)
      /vmport[0] (memory-region)
    /device[1] (kvmvapic)
      /kvmvapic-rom[0] (memory-region)
      /kvmvapic[0] (memory-region)
    /device[20] (vmmouse)
    /device[21] (port92)
      /port92[0] (memory-region)
    /device[22] (e1000)
      /bus master container[0] (memory-region)
      /bus master[0] (memory-region)
      /e1000-io[0] (memory-region)
      /e1000-mmio[0] (memory-region)
      /e1000.rom[0] (memory-region)
    /device[23] (piix3-ide)
      /bmdma[0] (memory-region)
      /bmdma[1] (memory-region)
      /bus master container[0] (memory-region)
      /bus master[0] (memory-region)
      /ide.0 (IDE)
      /ide.1 (IDE)
      /piix-bmdma-container[0] (memory-region)
      /piix-bmdma[0] (memory-region)
      /piix-bmdma[1] (memory-region)
    /device[24] (ide-hd)
    /device[25] (ide-cd)
    /device[26] (PIIX4_PM)
      /acpi-cnt[0] (memory-region)
      /acpi-cpu-hotplug[0] (memory-region)
      /acpi-cpu-hotplug[1] (memory-region)
      /acpi-evt[0] (memory-region)
      /acpi-gpe0[0] (memory-region)
      /acpi-pci-hotplug[0] (memory-region)
      /acpi-tmr[0] (memory-region)
      /apm-io[0] (memory-region)
      /bus master container[0] (memory-region)
      /bus master[0] (memory-region)
      /i2c (i2c-bus)
      /piix4-pm[0] (memory-region)
      /pm-smbus[0] (memory-region)
    /device[27] (smbus-eeprom)
    /device[28] (smbus-eeprom)
    /device[29] (smbus-eeprom)
    /device[2] (kvmclock)
    /device[30] (smbus-eeprom)
    /device[31] (smbus-eeprom)
    /device[32] (smbus-eeprom)
    /device[33] (smbus-eeprom)
    /device[34] (smbus-eeprom)
    /device[3] (i440FX)
      /bus master container[0] (memory-region)
      /bus master[0] (memory-region)
      /smram-low[0] (memory-region)
      /smram-region[0] (memory-region)
      /smram[0] (memory-region)
    /device[4] (PIIX3)
      /bus master container[0] (memory-region)
      /bus master[0] (memory-region)
      /isa.0 (ISA)
      /piix3-reset-control[0] (memory-region)
    /device[5] (kvm-i8259)
      /kvm-elcr[0] (memory-region)
      /kvm-pic[0] (memory-region)
    /device[6] (kvm-i8259)
      /kvm-elcr[0] (memory-region)
      /kvm-pic[0] (memory-region)
    /device[7] (virtio-vga)
      /bochs dispi interface[0] (memory-region)
      /bus master container[0] (memory-region)
      /bus master[0] (memory-region)
      /msix-pba[0] (memory-region)
      /msix-table[0] (memory-region)
      /qemu extended regs[0] (memory-region)
      /vbe[0] (memory-region)
      /vga ioports remapped[0] (memory-region)
      /vga-lowmem[0] (memory-region)
      /vga.vram[0] (memory-region)
      /vga[0] (memory-region)
      /vga[1] (memory-region)
      /vga[2] (memory-region)
      /vga[3] (memory-region)
      /vga[4] (memory-region)
      /virtio-backend (virtio-gpu-device)
      /virtio-bus (virtio-pci-bus)
      /virtio-pci-common-virtio-gpu[0] (memory-region)
      /virtio-pci-device-virtio-gpu[0] (memory-region)
      /virtio-pci-isr-virtio-gpu[0] (memory-region)
      /virtio-pci-notify-pio-virtio-gpu[0] (memory-region)
      /virtio-pci-notify-virtio-gpu[0] (memory-region)
      /virtio-pci[0] (memory-region)
      /virtio-vga-msix[0] (memory-region)
      /virtio-vga.rom[0] (memory-region)
    /device[8] (hpet)
      /hpet[0] (memory-region)
      /unnamed-gpio-in[0] (irq)
      /unnamed-gpio-in[1] (irq)
    /device[9] (mc146818rtc)
      /rtc-index[0] (memory-region)
      /rtc[0] (memory-region)
    /ide[0] (memory-region)
    /ide[1] (memory-region)
    /ide[2] (memory-region)
    /ide[3] (memory-region)
    /io[0] (memory-region)
    /ioport80[0] (memory-region)
    /ioportF0[0] (memory-region)
    /isa-bios[0] (memory-region)
    /non-qdev-gpio[0] (irq)
    /non-qdev-gpio[10] (irq)
    /non-qdev-gpio[11] (irq)
    /non-qdev-gpio[12] (irq)
    /non-qdev-gpio[13] (irq)
    /non-qdev-gpio[14] (irq)
    /non-qdev-gpio[15] (irq)
    /non-qdev-gpio[16] (irq)
    /non-qdev-gpio[17] (irq)
    /non-qdev-gpio[18] (irq)
    /non-qdev-gpio[19] (irq)
    /non-qdev-gpio[1] (irq)
    /non-qdev-gpio[20] (irq)
    /non-qdev-gpio[21] (irq)
    /non-qdev-gpio[22] (irq)
    /non-qdev-gpio[23] (irq)
    /non-qdev-gpio[24] (irq)
    /non-qdev-gpio[25] (irq)
    /non-qdev-gpio[2] (irq)
    /non-qdev-gpio[3] (irq)
    /non-qdev-gpio[4] (irq)
    /non-qdev-gpio[5] (irq)
    /non-qdev-gpio[6] (irq)
    /non-qdev-gpio[7] (irq)
    /non-qdev-gpio[8] (irq)
    /non-qdev-gpio[9] (irq)
    /pc.bios[0] (memory-region)
    /pc.rom[0] (memory-region)
    /pci[0] (memory-region)
    /ram-above-4g[0] (memory-region)
    /ram-below-4g[0] (memory-region)
    /sysbus (System)
    /system[0] (memory-region)
(qemu)
```

