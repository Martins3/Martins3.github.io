**TODO** 将所有的 pci.md 都整理到这个位置吧

https://wiki.osdev.org/PCI_Express : 通过 ACPI 的配置，可以让 PCI 配置空间的访问使用 mmio 的方式

https://66ring.github.io/2021/09/10/universe/qemu/qemu_bus_simulate/ : 写的非常不错的文档

- [ ] 从 pci_register_bar 中介绍一下 wmask 的作用
    - 其实很简单，就是 pci_dev->config 之前，首先和 wmask 一下，但是读直接读取 pci_dev->config

## dma

在 QEMU 中的部分:
```c
static uint32_t
e1000e_txdesc_writeback(E1000ECore *core, dma_addr_t base,
                        struct e1000_tx_desc *dp, bool *ide, int queue_idx)
{
    uint32_t txd_upper, txd_lower = le32_to_cpu(dp->lower.data);

    if (!(txd_lower & E1000_TXD_CMD_RS) &&
        !(core->mac[IVAR] & E1000_IVAR_TX_INT_EVERY_WB)) {
        return 0;
    }

    *ide = (txd_lower & E1000_TXD_CMD_IDE) ? true : false;

    txd_upper = le32_to_cpu(dp->upper.data) | E1000_TXD_STAT_DD;

    dp->upper.data = cpu_to_le32(txd_upper);
    pci_dma_write(core->owner, base + ((char *)&dp->upper - (char *)dp),
                  &dp->upper, sizeof(dp->upper));
    return e1000e_tx_wb_interrupt_cause(core, queue_idx);
}
```

在内核中的 e1000_clean_tx_irq 中对应位置检查数值的。

# QEMU 如何模拟 PCI 设备
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

```txt
#0  i440fx_realize (dev=0x5555568e2e00, errp=0x7fffffffcb30) at /home/maritns3/core/xqm/hw/pci-host/i440fx.c:264
#1  0x0000555555abaf2b in pci_qdev_realize (qdev=0x5555568e2e00, errp=<optimized out>) at /home/maritns3/core/xqm/hw/pci/pci.c:2099
#2  0x0000555555a25435 in device_set_realized (obj=<optimized out>, value=<optimized out>, errp=0x7fffffffccc0) at /home/maritns3/core/xqm/hw/core/qdev.c:876
#3  0x0000555555bb1e1b in property_set_bool (obj=0x5555568e2e00, v=<optimized out>, name=<optimized out>, opaque=0x555556c1db90, errp=0x7fffffffccc0) at /home/maritns3/core/xqm/qom/object.c:2078
#4  0x0000555555bb6604 in object_property_set_qobject (obj=obj@entry=0x5555568e2e00, value=value@entry=0x555556c11370, name=name@entry=0x555555db1285 "realized", errp=errp@entry=0x7fffffffccc0) at /home/maritns3/core/xqm/qom/qom-qobject.c:26
#5  0x0000555555bb3e3a in object_property_set_bool (obj=0x5555568e2e00, value=<optimized out>, name=0x555555db1285 "realized", errp=0x7fffffffccc0) at /home/maritns3/core/xqm/qom/object.c:1336
#6  0x0000555555a24276 in qdev_init_nofail (dev=dev@entry=0x5555568e2e00) at /home/maritns3/core/xqm/hw/core/qdev.c:363
#7  0x0000555555ab99ab in pci_create_simple_multifunction (name=<optimized out>, multifunction=false, devfn=<optimized out>, bus=<optimized out>) at /home/maritns3/core/xqm/hw/pci/pci.c:2168
#8  pci_create_simple (bus=<optimized out>, devfn=<optimized out>, name=<optimized out>) at /home/maritns3/core/xqm/hw/pci/pci.c:2179
#9  0x0000555555ab4add in i440fx_init (host_type=host_type@entry=0x555555d74f1b "i440FX-pcihost", pci_type=pci_type@entry=0x555555d75e48 "i440FX", pi440fx_state=pi440fx_state@entry=0x7fffffffcd90, address_space_mem=address_space_mem@entry=0x555556551700, address_space_io=address_space_io@entry=0x55555653a300, ram_size=6442450944, below_4g_mem_size=3221225472, above_4g_mem_size=3221225472, pci_address_space=0x555556535300, ram_memory=0x555556506700) at /home/maritns3/core/xqm/hw/pci-host/i440fx.c:298
#10 0x0000555555913b3d in pc_init1 (machine=0x55555659a000, pci_type=0x555555d75e48 "i440FX", host_type=0x555555d74f1b "i440FX-pcihost") at /home/maritns3/core/xqm/hw/i386/pc_piix.c:196
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

## bar

```c
int pci_bar(PCIDevice *d, int reg)
{
    uint8_t type;

    if (reg != PCI_ROM_SLOT)
        return PCI_BASE_ADDRESS_0 + reg * 4;

    type = d->config[PCI_HEADER_TYPE] & ~PCI_HEADER_TYPE_MULTI_FUNCTION;
    return type == PCI_HEADER_TYPE_BRIDGE ? PCI_ROM_ADDRESS1 : PCI_ROM_ADDRESS;
}
```
一共存在 6 + 1 的映射空间:
- 6 指的是:
- 1 指的是:

每当

## 挑战
- [ ] 中断的分配规则是什么
-  在 seabios 初始化的过程中, 可以检测到很多向 PCI 空间中写 PCI_BAR_UNMAPPED

## 记录
- pci_ls7a_config 是没有作用的,删掉也无所谓

## 在内核启动的过程中会调用 e1000e 吗
```c
      00000000febc0000-00000000febdffff (prio 1, i/o): e1000-mmio
```
- [ ] 将这些发生对于 e1000e-mmio 写的位置找到?
- [ ] 成功初始化的标志是什么?
- [ ] 为什么 pci 空间必须从 fe0000000 开始

## [x] 分析 bar 空间的赋值是如何确定的
- pci_default_write_config 中调用 pci_update_mappings 会告诉那个 device 的空间在哪里

## [x] 理解一下 PCI_COMMAND 中的含义
在 pci_bar_address 中
```c
    uint16_t cmd = pci_get_word(d->config + PCI_COMMAND);

    if (!(cmd & PCI_COMMAND_MEMORY)) {
        return PCI_BAR_UNMAPPED;
    }

```
如果打开了 PCI_COMMAND_MEMORY , 才会存在 memory address space 的.

## network_init
- analzy how pci works in qemu

### [ ] ls7a_pci_conf

配置空间:
```c
    0000000020000000-0000000027ffffff (prio 0, i/o): pcie-mmcfg-mmio
```
- pcie_mmcfg_data_write
- pcie_mmcfg_data_read

- ls7a_init
  - dev = qdev_create(NULL, TYPE_LS7A_PCIE_HOST_BRIDGE);
  - pci_bus = pci_ls7a_init(machine, dev, pic);
  - qdev_init_nofail(dev);
  - pcid = pci_create(pci_bus, PCI_DEVFN(0, 0), TYPE_PCIE_LS7A);

## 尝试直通一下 pci config spsace

```c
=== PCI bus & bridge init ===
PCI: pci_bios_init_bus_rec bus = 0x0
=== PCI device probing ===
Found 2 PCI devices (max PCI bus is 00)
=== PCI new allocation pass #1 ===
PCI: check devices
=== PCI new allocation pass #2 ===
PCI: IO: c000 - bfff
PCI: 32: 0000000080000000 - 00000000fec00000
PCI: init bdf=00:00.0 id=8086:1237
disable i440FX memory mappings
PCI: init bdf=00:01.0 id=8086:7000
PIIX3/PIIX4 init: elcr=00 0c
disable PIIX3 memory mappings
PCI: No VGA devices found
```

- pci_host_config_write
  - 记录地址
- pci_host_data_write
  - pci_data_write :
    - pci_dev_find_by_addr : 找到 PCIDevice
    - pci_host_config_write_common
      - 调用具体 pci device 的函数

在 do_pci_register_device 会给所有的 pci device 默认的

但是,在 piix3_class_init 中还是可以修改的!
```c
static void piix3_class_init(PCIDeviceClass *k) {
  k->config_write = piix3_write_config;
}
```


## 为什么地址空间是从 80000900 开始的
```c
[huxueshi:pci_data_read:127] 80000900
[huxueshi:pci_data_read:127] 80000a00
[huxueshi:pci_data_read:127] 80000b00
[huxueshi:pci_data_read:127] 80000c00
[huxueshi:pci_data_read:127] 80000d00
[huxueshi:pci_data_read:127] 80000e00
[huxueshi:pci_data_read:127] 80000f00
[huxueshi:pci_data_read:127] 80001000
[huxueshi:pci_data_read:127] 80009800
// ....
[huxueshi:pci_data_read:127] 8000a000
[huxueshi:pci_data_read:127] 8000a800
[huxueshi:pci_data_read:127] 8000b000
[huxueshi:pci_data_read:127] 8000b800
[huxueshi:pci_data_read:127] 8000c000
[huxueshi:pci_data_read:127] 8000c800
[huxueshi:pci_data_read:127] 8000d000
[huxueshi:pci_data_read:127] 8000d800
[huxueshi:pci_data_read:127] 8000e000
[huxueshi:pci_data_read:127] 8000e800
[huxueshi:pci_data_read:127] 8000f000
[huxueshi:pci_data_read:127] 8000f800
```

```c
// Check if PCI is available at all
int
pci_probe_host(void)
{
    outl(0x80000000, PORT_PCI_CMD);
    if (inl(PORT_PCI_CMD) != 0x80000000) {
        dprintf(1, "Detected non-PCI system\n");
        return -1;
    }
    return 0;
}
```

在 seabios 写的:
```c
static u32 ioconfig_cmd(u16 bdf, u32 addr)
{
    return 0x80000000 | (bdf << 8) | (addr & 0xfc);
}
```

## [ ] 最后一个问题, loongarch 的 pci mmio 空间是随便分配的吗
- 从那个位置开始的

> BAR 根据 mmio 和 pio 有两种不同的布局，其中 bit0 用于指示是内存是 mmio 还是 pio
> https://66ring.github.io/2021/09/10/universe/qemu/qemu_bus_simulate/

没有给每一个 pci bar 构建 addr space 了,反正都是都是记录在 host 机器中

- [x] 似乎 ROM addr 不在 bar 中?
  - 应该只是一个 enable 操作而已
```c
/* Header type 0 (normal devices) */
#define PCI_CARDBUS_CIS   0x28
#define PCI_SUBSYSTEM_VENDOR_ID 0x2c
#define PCI_SUBSYSTEM_ID  0x2e
#define PCI_ROM_ADDRESS   0x30  /* Bits 31..11 are address, 10..1 reserved */
#define  PCI_ROM_ADDRESS_ENABLE 0x01
#define PCI_ROM_ADDRESS_MASK  (~0x7ffU)
```

在 pci_register_bar 中:
```c
    if (!(r->type & PCI_BASE_ADDRESS_SPACE_IO) &&
        r->type & PCI_BASE_ADDRESS_MEM_TYPE_64) {
        pci_set_quad(pci_dev->wmask + addr, wmask);
        pci_set_quad(pci_dev->cmask + addr, ~0ULL);
    } else {
        pci_set_long(pci_dev->wmask + addr, wmask & 0xffffffff);
        pci_set_long(pci_dev->cmask + addr, 0xffffffff);
    }
```

## PCI_BASE_ADDRESS_MEM_TYPE_64

pci_register_bar 中, 只是一个普通的 PCI_BASE_ADDRESS_MEM_TYPE_64 type 信息, 之后会放到 config 中的:
```c
    pci_set_long(pci_dev->config + addr, type);
```

```c
    if (type & PCI_BASE_ADDRESS_MEM_TYPE_64) {
        new_addr = pci_get_quad(d->config + bar);
    } else {
        new_addr = pci_get_long(d->config + bar);
    }
```

bar addr 的开始位置,从 PCI_BASE_ADDRESS_0 每一个 bar 大小为 4 * byte
```c
int pci_bar(PCIDevice *d, int reg)
{
    uint8_t type;

    if (reg != PCI_ROM_SLOT)
        return PCI_BASE_ADDRESS_0 + reg * 4;

    type = d->config[PCI_HEADER_TYPE] & ~PCI_HEADER_TYPE_MULTI_FUNCTION;
    return type == PCI_HEADER_TYPE_BRIDGE ? PCI_ROM_ADDRESS1 : PCI_ROM_ADDRESS;
}
```

## [ ] 本来以为 pci_update_mappings 中除了 reset 导致这个判断成功

```c
        /* This bar isn't changed */
        if (new_addr == r->addr){
            continue;
        }
```
但是,实际上, ide 这个设备会导致:
```c
$1 = "piix3-ide", '\000' <repeats 54 times>
```
这个东西配置一次不就好了!

```c
>>> disass 0xeb0b7
Dump of assembler code for function pci_config_writel:
   0x000eb0b7 <+0>:     push   %ebx
   0x000eb0b8 <+1>:     mov    0xf5558,%ebx
   0x000eb0be <+7>:     test   %ebx,%ebx
   0x000eb0c0 <+9>:     movzwl %ax,%eax
   0x000eb0c3 <+12>:    je     0xeb0d0 <pci_config_writel+25>
   0x000eb0c5 <+14>:    shl    $0xc,%eax
   0x000eb0c8 <+17>:    add    %ebx,%edx
   0x000eb0ca <+19>:    add    %edx,%eax
   0x000eb0cc <+21>:    mov    %ecx,(%eax)
   0x000eb0ce <+23>:    jmp    0xeb0ee <pci_config_writel+55>
   0x000eb0d0 <+25>:    shl    $0x8,%eax
   0x000eb0d3 <+28>:    and    $0xfc,%edx
   0x000eb0d9 <+34>:    or     %edx,%eax
   0x000eb0db <+36>:    or     $0x80000000,%eax
   0x000eb0e0 <+41>:    mov    $0xcf8,%edx
   0x000eb0e5 <+46>:    out    %eax,(%dx)
   0x000eb0e6 <+47>:    mov    $0xcfc,%edx
   0x000eb0eb <+52>:    mov    %ecx,%eax
   0x000eb0ed <+54>:    out    %eax,(%dx)
   0x000eb0ee <+55>:    pop    %ebx
   0x000eb0ef <+56>:    ret
End of assembler dump.
```
看来是在 seabios 中搞的.

## [x] 分析一下, x86 的 mmio 空间分配的规则是什么

```c
guest ip : 7faf709
failed in [memory dispatch] with offset=[feb40000]
memory_region_look_up:106: g_assert_not_reached!!! bmbt never call exit !!!
```
还是在 seabios 中的,但是通过 gdb 无法获取这个地址.

在 seabios 中的 log :
```c
=== PCI new allocation pass #2 ===
PCI: IO: c000 - c02f
PCI: 32: 0000000080000000 - 00000000fec00000
PCI: map device bdf=00:03.0  bar 2, addr 0000c000, size 00000020 [io]
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
PCI: map device bdf=00:01.1  bar 4, addr 0000c020, size 00000010 [io]
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
PCI: map device bdf=00:03.0  bar 6, addr feb40000, size 00040000 [mem]
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
PCI: map device bdf=00:03.0  bar 0, addr feb80000, size 00020000 [mem]
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
PCI: map device bdf=00:03.0  bar 1, addr feba0000, size 00020000 [mem]
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
PCI: map device bdf=00:02.0  bar 6, addr febc0000, size 00010000 [mem]
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
PCI: map device bdf=00:03.0  bar 3, addr febd0000, size 00004000 [mem]
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
PCI: map device bdf=00:02.0  bar 2, addr febd4000, size 00001000 [mem]
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
PCI: map device bdf=00:02.0  bar 0, addr fd000000, size 01000000 [prefmem]
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
PCI: init bdf=00:00.0 id=8086:1237
PCI: init bdf=00:01.0 id=8086:7000
PIIX3/PIIX4 init: elcr=00 0c
PCI: init bdf=00:01.1 id=8086:7010
[huxueshi:pci_update_mappings:1357] c020 -> ffffffffffffffff
PCI: init bdf=00:01.3 id=8086:7113
Using pmtimer, ioport 0x608
PCI: init bdf=00:02.0 id=1234:1111
[huxueshi:pci_update_mappings:1357] fd000000 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1357] febd4000 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
PCI: init bdf=00:03.0 id=8086:10d3
[huxueshi:pci_update_mappings:1357] feb80000 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1357] feba0000 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1357] c000 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1357] febd0000 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1354] ffffffffffffffff
```

- [x] 似乎我们不知道 size 是多少啊
  - 这种事情本来就是设备配置
    - 在 QEMU 中设置的

只要一个固定的偏移就可以了,其他的事情不用考虑.

在 seabios 中:
```c
static int pci_bios_init_root_regions_mem(struct pci_bus *bus)
{
    struct pci_region *r_end = &bus->r[PCI_REGION_TYPE_PREFMEM];
    struct pci_region *r_start = &bus->r[PCI_REGION_TYPE_MEM];

    if (pci_region_align(r_start) < pci_region_align(r_end)) {
        // Swap regions to improve alignment.
        r_end = r_start;
        r_start = &bus->r[PCI_REGION_TYPE_PREFMEM];
    }
    u64 sum = pci_region_sum(r_end);
    u64 align = pci_region_align(r_end);
    r_end->base = ALIGN_DOWN((pcimem_end - sum), align);
    sum = pci_region_sum(r_start);
    align = pci_region_align(r_start);
    r_start->base = ALIGN_DOWN((r_end->base - sum), align);

    if ((r_start->base < pcimem_start) ||
         (r_start->base > pcimem_end))
        // Memory range requested is larger than available.
        return -1;
    return 0;
}
```

```c
u64 pcimem_end     = BUILD_PCIMEM_END;
#define BUILD_PCIMEM_END          0xfec00000    /* IOAPIC is mapped at */
#define BUILD_PCIMEM_START        0xe0000000
```
根据结尾的位置计算的出来 mmio 空间的开始位置的.

这边就是从这里开始的.

## [x] 分析一下, loongarch 的 mmio 空间分配的规则是什么
```c
    0000000040000000-0000000041ffffff (prio 1, i/o): cirrus-pci-bar0
      0000000040000000-00000000403fffff (prio 1, ram): vga.vram
      0000000040000000-00000000403fffff (prio 0, i/o): cirrus-linear-io
      0000000041000000-00000000413fffff (prio 0, i/o): cirrus-bitblt-mmio
    0000000040000000-000000007fffffff (prio 0, i/o): isa-mem
    0000000042040000-000000004205ffff (prio 1, i/o): e1000e-mmio
    0000000042060000-000000004207ffff (prio 1, i/o): e1000e-flash
    0000000042080000-0000000042083fff (prio 1, i/o): e1000e-msix
      0000000042080000-000000004208004f (prio 0, i/o): msix-table
      0000000042082000-0000000042082007 (prio 0, i/o): msix-pba
    0000000042084000-0000000042087fff (prio 1, i/o): virtio-pci
      0000000042084000-0000000042084fff (prio 0, i/o): virtio-pci-common
      0000000042085000-0000000042085fff (prio 0, i/o): virtio-pci-isr
      0000000042086000-0000000042086fff (prio 0, i/o): virtio-pci-device
      0000000042087000-0000000042087fff (prio 0, i/o): virtio-pci-notify
    0000000042088000-0000000042088fff (prio 1, i/o): cirrus-mmio
    0000000042089000-0000000042089fff (prio 1, i/o): virtio-blk-pci-msix
      0000000042089000-000000004208901f (prio 0, i/o): msix-table
      0000000042089800-0000000042089807 (prio 0, i/o): msix-pba
```
- [x] 从代码中验证一下的确是 0x42000000 的
  - 是 guest 写的,无法从 QEMU 中获取到
  - 从 arch/loongarch/include/asm/mach-la64/pci.h

- [ ] 如果 loongarch bios 初始化过一次每一个 pci 的 bar 空间,在 bmbt 中初始化,不会出现问题吗?
  - 之前分析 x86 的时候, seabios 中初始化一次,然后内核似乎会重新初始化一次

如果让 guest 直接写,就可以得到这个,符合预期:
```c
    00000000feb40000-00000000feb7ffff (prio 1, rom): e1000e.rom
    00000000feb80000-00000000feb9ffff (prio 1, i/o): e1000e-mmio
    00000000feba0000-00000000febbffff (prio 1, i/o): e1000e-flash
    00000000febc0000-00000000febc3fff (prio 1, i/o): e1000e-msix
```

## cmask 和 wmask
在 pci_config_alloc 中初始化

- [x] 所以 wmask 如何保证 write 的时候的

似乎 get_pci_config_device 中只是使用的这个, load 指的是 vmstate 的 load 的过程.

## 参考资料 QEMU pci type 0 和 type 1 的操作
```c
/* Header type 0 (normal devices) */
#define PCI_CARDBUS_CIS   0x28
#define PCI_SUBSYSTEM_VENDOR_ID 0x2c
#define PCI_SUBSYSTEM_ID  0x2e
#define PCI_ROM_ADDRESS   0x30  /* Bits 31..11 are address, 10..1 reserved */
#define  PCI_ROM_ADDRESS_ENABLE 0x01
#define PCI_ROM_ADDRESS_MASK  (~0x7ffU)

#define PCI_CAPABILITY_LIST 0x34  /* Offset of first capability list entry */

/* 0x35-0x3b are reserved */
#define PCI_INTERRUPT_LINE  0x3c  /* 8 bits */
#define PCI_INTERRUPT_PIN 0x3d  /* 8 bits */
#define PCI_MIN_GNT   0x3e  /* 8 bits */
#define PCI_MAX_LAT   0x3f  /* 8 bits */

/* Header type 1 (PCI-to-PCI bridges) */
#define PCI_PRIMARY_BUS   0x18  /* Primary bus number */
#define PCI_SECONDARY_BUS 0x19  /* Secondary bus number */
#define PCI_SUBORDINATE_BUS 0x1a  /* Highest bus number behind the bridge */
#define PCI_SEC_LATENCY_TIMER 0x1b  /* Latency timer for secondary interface */
#define PCI_IO_BASE   0x1c  /* I/O range behind the bridge */
#define PCI_IO_LIMIT    0x1d
#define  PCI_IO_RANGE_TYPE_MASK 0x0fUL  /* I/O bridging type */
#define  PCI_IO_RANGE_TYPE_16 0x00
#define  PCI_IO_RANGE_TYPE_32 0x01
#define  PCI_IO_RANGE_MASK  (~0x0fUL) /* Standard 4K I/O windows */
#define  PCI_IO_1K_RANGE_MASK (~0x03UL) /* Intel 1K I/O windows */
#define PCI_SEC_STATUS    0x1e  /* Secondary status register, only bit 14 used */
#define PCI_MEMORY_BASE   0x20  /* Memory range behind */
#define PCI_MEMORY_LIMIT  0x22
#define  PCI_MEMORY_RANGE_TYPE_MASK 0x0fUL
#define  PCI_MEMORY_RANGE_MASK  (~0x0fUL)
#define PCI_PREF_MEMORY_BASE  0x24  /* Prefetchable memory range behind */
#define PCI_PREF_MEMORY_LIMIT 0x26
#define  PCI_PREF_RANGE_TYPE_MASK 0x0fUL
#define  PCI_PREF_RANGE_TYPE_32 0x00
#define  PCI_PREF_RANGE_TYPE_64 0x01
#define  PCI_PREF_RANGE_MASK  (~0x0fUL)
#define PCI_PREF_BASE_UPPER32 0x28  /* Upper half of prefetchable memory range */
#define PCI_PREF_LIMIT_UPPER32  0x2c
#define PCI_IO_BASE_UPPER16 0x30  /* Upper half of I/O addresses */
#define PCI_IO_LIMIT_UPPER16  0x32
```

## [ ] 实际上是存在 pio 的,哭泣吧
似乎是 sata 搞的,暂时不怕
```txt
[    6.921728] pci_bus 0000:00: root bus resource [bus 00-ff]
[huxueshi:pci_update_mappings:1361] ffffffffffffffff -> c020
huxueshi
huxueshi
[huxueshi:pci_update_mappings:1361] c020 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1361] ffffffffffffffff -> c020
huxueshi
huxueshi
[huxueshi:pci_update_mappings:1361] c020 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1361] ffffffffffffffff -> c020
huxueshi
huxueshi
[huxueshi:pci_update_mappings:1361] c020 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1361] ffffffffffffffff -> c020
huxueshi
huxueshi
[huxueshi:pci_update_mappings:1361] c020 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1361] ffffffffffffffff -> c020
huxueshi
huxueshi
[huxueshi:pci_update_mappings:1361] c020 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1361] ffffffffffffffff -> c020
huxueshi
huxueshi
[huxueshi:pci_update_mappings:1361] c020 -> ffffffffffffffff
[huxueshi:pci_update_mappings:1361] ffffffffffffffff -> c020
```

```c
[    8.108393] ata1: PATA max MWDMA2 cmd 0x1f0 ctl 0x3f6 bmdma 0xc020 irq 14
```

难道 e1000e 真的制作了 pio 的吗?

## 实现的方法
- pci_register_bar 中会制作的时候, wmask 会将低位全部清空的.
- PCIDevice::config 中就是设备的
- bar 的类型实际上不可写的,因为在 QEMU 中就已经初始化好了
- pci_do_device_reset

## 如何处理中断
- [ ] pci_default_write_config 调用了 pci_update_irq_disabled 是做啥的

## 找到 x86 的 PCI io 空间在哪里

mtree info 看到的:
```c
  000000000000c000-000000000000c03f (prio 1, i/o): e1000-io
  000000000000c040-000000000000c05f (prio 1, i/o): virtio-pci
  000000000000c060-000000000000c063 (prio 0, i/o): piix-bmdma
  000000000000c064-000000000000c067 (prio 0, i/o): bmdma
  000000000000c068-000000000000c06b (prio 0, i/o): piix-bmdma
  000000000000c06c-000000000000c06f (prio 0, i/o): bmdma
  000000000000c070-000000000000ffff (prio 0, i/o): io @000000000000c070
```

从 seabios 的 pci_bios_init_root_regions_io 是 0xc000

## option rom 的特殊的地方在哪里

## [ ] 中断的这种错误没有吧
```c
[    5.909127] e1000e 0000:00:02.0: Interrupt Throttling Rate (ints/sec) set to dynamic conservative mode
[    5.915877] e1000e 0000:00:02.0 0000:00:02.0 (uninitialized): Failed to initialize MSI-X interrupts.  Falling back to MSI interrupts.
[    5.919983] e1000e 0000:00:02.0 0000:00:02.0 (uninitialized): Failed to initialize MSI interrupts.  Falling back to legacy interrupts.
```
