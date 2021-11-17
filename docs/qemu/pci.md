# QEMU 如何模拟 PCI 设备

- [ ] i440fx pm 这个设备到底是如何管理 power 的
- [ ] 处理掉所有的 NEED_LATER
  - [ ] fw_cfg 之类的留到下次在处理吧
  - [ ] pc_machine_initfn 中的 `pmms->smm`

- i440fx_pcihost_initfn : 初始化最初的注册
- i440fx_pcihost_realize : 注册 0xcf8 和 0xcfc 两个端口，然后将这两个
- hw/pci/pci_host.c : 非常小的一个文件，定义了
  - pci_host_config_read
  - pci_host_data_write
    - pci_data_write
      - pci_dev_find_by_addr
        - pci_find_device
          - pci_find_bus_nr : 这个东西是在 do_pci_register_device 位置注册的
      - pci_host_config_write_common

## pci 设备的初始化基本过程
通过 pci_root_bus_new 可以创建出来 PCIBus 的，这里最后初始化到了 pci_bus_info 的呀

```c
/*
#0  i440fx_realize (dev=0x5555568e2e00, errp=0x7fffffffcb30) at /home/maritns3/core/xqm/hw/pci-host/i440fx.c:264
#1  0x0000555555abaf2b in pci_qdev_realize (qdev=0x5555568e2e00, errp=<optimized out>) at /home/maritns3/core/xqm/hw/pci/pci.c:2099
#2  0x0000555555a25435 in device_set_realized (obj=<optimized out>, value=<optimized out>, errp=0x7fffffffccc0) at /home/maritns3/core/xqm/hw/core/qdev.c:876
#3  0x0000555555bb1e1b in property_set_bool (obj=0x5555568e2e00, v=<optimized out>, name=<optimized out>, opaque=0x555556c1db90, errp=0x7fffffffccc0) at /home/maritns3
/core/xqm/qom/object.c:2078
#4  0x0000555555bb6604 in object_property_set_qobject (obj=obj@entry=0x5555568e2e00, value=value@entry=0x555556c11370, name=name@entry=0x555555db1285 "realized", errp=
errp@entry=0x7fffffffccc0) at /home/maritns3/core/xqm/qom/qom-qobject.c:26
#5  0x0000555555bb3e3a in object_property_set_bool (obj=0x5555568e2e00, value=<optimized out>, name=0x555555db1285 "realized", errp=0x7fffffffccc0) at /home/maritns3/c
ore/xqm/qom/object.c:1336
#6  0x0000555555a24276 in qdev_init_nofail (dev=dev@entry=0x5555568e2e00) at /home/maritns3/core/xqm/hw/core/qdev.c:363
#7  0x0000555555ab99ab in pci_create_simple_multifunction (name=<optimized out>, multifunction=false, devfn=<optimized out>, bus=<optimized out>) at /home/maritns3/cor
e/xqm/hw/pci/pci.c:2168
#8  pci_create_simple (bus=<optimized out>, devfn=<optimized out>, name=<optimized out>) at /home/maritns3/core/xqm/hw/pci/pci.c:2179
#9  0x0000555555ab4add in i440fx_init (host_type=host_type@entry=0x555555d74f1b "i440FX-pcihost", pci_type=pci_type@entry=0x555555d75e48 "i440FX", pi440fx_state=pi440f
x_state@entry=0x7fffffffcd90, address_space_mem=address_space_mem@entry=0x555556551700, address_space_io=address_space_io@entry=0x55555653a300, ram_size=6442450944, be
low_4g_mem_size=3221225472, above_4g_mem_size=3221225472, pci_address_space=0x555556535300, ram_memory=0x555556506700) at /home/maritns3/core/xqm/hw/pci-host/i440fx.c:
298
#10 0x0000555555913b3d in pc_init1 (machine=0x55555659a000, pci_type=0x555555d75e48 "i440FX", host_type=0x555555d74f1b "i440FX-pcihost") at /home/maritns3/core/xqm/hw/
i386/pc_piix.c:196
#11 0x0000555555a2c693 in machine_run_board_init (machine=0x55555659a000) at /home/maritns3/core/xqm/hw/core/machine.c:1143
#12 0x000055555582b0b8 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/xqm/vl.c:4348
```

## 分析一下 pci.c
- do_pci_register_device
  - do_pci_register_device

## 一些细节的挑战
- PCI_BUS qdev_get_parent_bus DEVICE 之类的装换
- PCIDeviceClass::realize 实际上，不能去掉的呀
  - 现在存在多个 PCI 的 devices 的 PCIDeviceClass::realize，可以是首先注册, 然后调用 pci_qdev_realize
    - pci_qdev_realize
      - do_pci_register_device : 选择 devfn 之类的事情了
      - pci_add_option_rom : 处理 option rom

- [x] piix3_pci_type_info 和 piix3_info 的关系是什么?
  - piix3_pci_type_info 是 parent，然后 piix3_info 是 child 的，用于修改 PCIDeviceClass::write_config 的

- 到现在为止，只是允许的 PCIDevice 的装换，其他的暂时就算了吧

- [ ] pci_default_write_config 这个要求 memory model 可以删除 memory region 的
- [ ] pci_default_write_config 中需要处理 msi 和 msix 的配置空间的书写
- [ ] pci_change_irq_level 的执行流程需要 review 一下
  - [ ] 注册的 map_irq 是在什么时候注册的

## i440fx_init 来看真正的实现细节
- [ ] i440fx_init 中同时处理了三个东西了呀
  - [ ] 除了自身的初始化之外，还是需要考虑到其 parent 的初始化，那么 parent 都是些什么东西呀

- [ ] 如果想要移除掉 pcibus 可以吗 ? 或者说只是提供一个空的勉强用用，有什么问题吗?
  - [ ] 恐怕不可以的哦

```txt
(qemu) huxueshi:i440fx_init pci_type=i440FX pci-host-type=i440FX-pcihost
```

- [ ] 好吧，我又忘记了 class_base_init 的作用，pci_device_type_info 是什么时候调用的

- qdev_create(NULL, host_type) : i440FX-pcihost
  - `pci_bus_class_init`
  - `i440fx_pcihost_initfn`
- pci_root_bus_new
  - qbus_create
    - bus = BUS(object_new(typename));
      - `qbus_initfn` : 设置 realize 属性 bus_set_realized
      - `bus_class_init` : 没啥东西
      - `pci_bus_class_init` : 注册了 BusClass::realize 和 BusClass::reset
  - pci_root_bus_init
    - 一些基本的初始化而已
    - pci_host_bus_register
- qdev_init_nofail(dev);
  - `i440fx_pcihost_realize`
- d = pci_create_simple(b, 0, pci_type);
  - pci_create_simple_multifunction : 应该将这个东西设计为
    - pci_create_multifunction
      - qdev_create
        - 并不存在对应的 initfn 之类，但是务必调用几个关键的 class init
          - pci_device_class_base_init : 检测 interface cast 的
          - pci_device_class_init
          - i440fx_class_init
    - qdev_init_nofail
      - pci_qdev_realize
        - pci_qdev_realize
          - do_pci_register_device
            - `i440fx_realize`

- 观察: 在 pci typeinfo 中注册的 class_init 比如 piix3_class_init 和 i440fx_class_init 其实主要都是在注册 PCIDeviceClass
  - 还是老办法，可以让 PCIDevice 中持有 PCIDeviceClass 的呀

### pci bus
```c
static const TypeInfo bus_info = {
    .name = TYPE_BUS,
    .parent = TYPE_OBJECT,
    .instance_size = sizeof(BusState),
    .abstract = true,
    .class_size = sizeof(BusClass),
    .instance_init = qbus_initfn, // hotplug_handler，设置 "realize" 属性设置 "realize" 属性
    .instance_finalize = qbus_finalize,
    .class_init = bus_class_init,
};

static const TypeInfo pci_bus_info = {
    .name = TYPE_PCI_BUS,
    .parent = TYPE_BUS,
    .instance_size = sizeof(PCIBus),
    .class_size = sizeof(PCIBusClass),
    .class_init = pci_bus_class_init,  // 注册 BusClass PCIBusClass 的操作
};
```
- 因为 bus 不是一个 device, 所以需要手动设置 realized 的事情
- 注意，在 pci_bus_class_init 中，注册的 DeviceClass 的 realize 的，而是 pci_bus_realize 的

### pci device
```c
static const TypeInfo pci_device_type_info = {
    .name = TYPE_PCI_DEVICE,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(PCIDevice),
    .abstract = true,
    .class_size = sizeof(PCIDeviceClass),
    .class_init = pci_device_class_init,           // 注册 PCIDevice::realize 为 pci_qdev_realize
    .class_base_init = pci_device_class_base_init,
};

static const TypeInfo i440fx_info = {
    .name          = TYPE_I440FX_PCI_DEVICE,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(PCII440FXState),
    .class_init    = i440fx_class_init,            // 注册的是 PCIDeviceClass::realize
    .interfaces = (InterfaceInfo[]) {
        { INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { },
    },
};
```
在 pci_qdev_realize 中调用这些 PCIDeviceClass::realize 的代码，但是这些

### pcihost

```c
static const TypeInfo sysbus_device_type_info = {
    .name = TYPE_SYS_BUS_DEVICE,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(SysBusDevice),
    .abstract = true,
    .class_size = sizeof(SysBusDeviceClass),
    .class_init = sysbus_device_class_init, // 几乎没有什么东西的，除了注册 sysbus_realize
};

static const TypeInfo pci_host_type_info = {
    .name = TYPE_PCI_HOST_BRIDGE,
    .parent = TYPE_SYS_BUS_DEVICE,
    .abstract = true,
    .class_size = sizeof(PCIHostBridgeClass),
    .instance_size = sizeof(PCIHostState),
};

static const TypeInfo i440fx_pcihost_info = {
    .name          = TYPE_I440FX_PCI_HOST_BRIDGE,
    .parent        = TYPE_PCI_HOST_BRIDGE,
    .instance_size = sizeof(I440FXState),
    .instance_init = i440fx_pcihost_initfn,     // 初始化 cf8 cfc 两个端口
    .class_init    = i440fx_pcihost_class_init, // 注册 realize 在 realize 中会初始化 cf8 cfc 两个端口
};
```

### reset
- [ ] 好家伙，现在 QOM 机制真的好难受啊

```c
#0  pcibus_reset (qbus=0x5555567be110) at /home/maritns3/core/xqm/hw/pci/pci.c:344
#1  0x0000555555a2377d in qbus_reset_one (bus=0x5555567be110, opaque=<optimized out>) at /home/maritns3/core/xqm/hw/core/qdev.c:310
#2  0x0000555555a24840 in qdev_walk_children (dev=0x5555566cc7d0, pre_devfn=0x0, pre_busfn=0x0, post_devfn=0x555555a24e70 <qdev_reset_one>, post_busfn=0x555555a23740 <
qbus_reset_one>, opaque=0x0) at /home/maritns3/core/xqm/hw/core/qdev.c:609
#3  0x0000555555a284c0 in qbus_walk_children (bus=0x5555565dd960, pre_devfn=0x0, pre_busfn=0x0, post_devfn=0x555555a24e70 <qdev_reset_one>, post_busfn=0x555555a23740 <
qbus_reset_one>, opaque=0x0) at /home/maritns3/core/xqm/hw/core/bus.c:53
#4  0x0000555555a28685 in qemu_devices_reset () at /home/maritns3/core/xqm/hw/core/reset.c:69
#5  0x000055555590e2df in pc_machine_reset (machine=<optimized out>) at /home/maritns3/core/xqm/hw/i386/pc.c:2140
#6  0x00005555559bc1e1 in qemu_system_reset (reason=SHUTDOWN_CAUSE_NONE) at /home/maritns3/core/xqm/vl.c:1551
#7  0x000055555582b262 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/xqm/vl.c:4438
```

## pci
```c
struct PCIBus {
    // ...
    QLIST_HEAD(, PCIBus) child; /* this will be replaced by qdev later */
    QLIST_ENTRY(PCIBus) sibling;/* this will be replaced by qdev later */
```
原来是通过这个东西来关联所有的 pci bus 的

## 如何面对 pcie 的挑战啊

```c
/* Implemented by devices that can be plugged on PCI Express buses */
#define INTERFACE_PCIE_DEVICE "pci-express-device"

/* Implemented by devices that can be plugged on Conventional PCI buses */
#define INTERFACE_CONVENTIONAL_PCI_DEVICE "conventional-pci-device"
```
- [ ] 虽然 INTERFACE_PCIE_DEVICE 的使用位置比 INTERFACE_CONVENTIONAL_PCI_DEVICE 少很多，但是 nvme 和 e1000e 都是这个
- [ ] 使用上 INTERFACE_PCIE_DEVICE 是不是存在一些关键的挑战啊
  - [ ] 暂时没有看到因为在 PCIDevice::cap_present 上插入 QEMU_PCI_CAP_EXPRESS 而导致具体的差别是什么

## hotplug
当 qdev_machine_creation_done 之后将会修正 qdev_hotplug 的数值。
但是之后再添加的设备在调用 qdev 的 hook 的时候，也就是在 device_initfn 中修正其属性 DeviceState::hotplugged

## PCIBridge
- [x] 之前分析 i440fx 的时候，和 PCIBridge 根本没有关系啊
  - 实际上，pci_bridge_initfn 从来都是不会被初始化的

## root bus
- 为什么需要初始化 pci_root_bus_init 中这些空间
- [ ] pci_root_bus_init 中初始化的内容真的需要吗?


- [ ] pci_get_bus 说明实际上，pcibus 并不可以被简单的删除的呀
  - [ ] pci_bus_is_root : 如何保证 root bus 就是 root 的呀
  - 既然现在只有 pci 设备和 pci bus，那么将 pci bus 定义为 global variable 不就完了

## pci_get_function_0
- [ ] 什么叫做 function 0 啊？

## interrupt
- [ ] pci_device_set_intx_routing_notifier 没有人调用过吗?
- [ ] pci_device_route_intx_to_irq 也是没有被调用，让人感觉到奇怪啊

- [ ] 好家伙: pci_update_mappings


## vga
在 pci_update_vga 中间，为什么总是保证 `pci_dev->has_vga` 的代码为 false

- [ ] pci_update_vga

## 如何模拟正常的 rom file
- [ ] pci devices 有可能需要 rom 的读去，能不能让物理机自己加载，而不是模拟

- [ ] isa bus 到底需要做什么?
  - [ ] 正常的返回它
