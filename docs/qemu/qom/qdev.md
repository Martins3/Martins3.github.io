## qdev

hw/core/qdev.c
hw/core/qdev-hotplug.c

```txt
static const TypeInfo device_type_info = {
    .name = TYPE_DEVICE,
    .parent = TYPE_OBJECT,
    .instance_size = sizeof(DeviceState),
    .instance_init = device_initfn,
    .instance_post_init = device_post_init,
    .instance_finalize = device_finalize,
    .class_base_init = device_class_base_init,
    .class_init = device_class_init,
    .abstract = true,
    .class_size = sizeof(DeviceClass),
    .interfaces = (const InterfaceInfo[]) {
        { TYPE_VMSTATE_IF },
        { TYPE_RESETTABLE_INTERFACE },
        { }
    }
};
```

## info qtree 的结构是很简单的


```txt
(qemu) info qtree
bus: main-system-bus
  type System
  dev: ps2-mouse, id ""
    gpio-out "" 1
  dev: ps2-kbd, id ""
    gpio-out "" 1
  dev: kvm-ioapic, id ""
    gpio-in "" 24
    gsi_base = 0 (0x0)
    mmio 00000000fec00000/0000000000001000
  dev: fw_cfg_io, id ""
    dma_enabled = true
    x-file-slots = 32 (0x20)
    acpi-mr-restore = true
  dev: i440FX-pcihost, id ""
    pci-hole64-size = 2147483648 (2 GiB)
    below-4g-mem-size = 3221225472 (3 GiB)
    above-4g-mem-size = 5368709120 (5 GiB)
    x-pci-hole64-fix = true
    pci-type = "i440FX"
    x-config-reg-migration-enabled = true
    bypass-iommu = false
    bus: pci.0
      type PCI
      dev: virtio-serial-pci, id ""
      dev: virtio-gpu-pci, id ""
      dev: virtio-blk-pci, id "virt-blk1"
      dev: vhost-user-blk-pci, id "blk1"
      dev: vhost-user-fs-pci, id ""
      dev: virtio-net-pci, id ""
      dev: virtio-net-pci, id ""
      dev: virtio-net-pci, id ""
      dev: virtio-scsi-pci, id "scsi4"
      dev: PIIX4_PM, id ""
      dev: piix3-ide, id ""
      dev: PIIX3, id ""
      dev: i440FX, id ""
  dev: kvmclock, id ""
    x-mach-use-reliable-get-clock = true
  dev: kvmvapic, id ""
```

## 为什么 qemu 的初始化又是依赖 property 的?
```txt
    object_class_property_add_bool(class, "realized",
                                   device_get_realized, device_set_realized);
```

直接通过 device_type_info 来注册不可以么?
## 既然 qdev 都已经封装好了，为什么 iothread 不去直接复用?

qdev 需要描述和 bus 的关系吗?

- qdev_get_parent_bus
- qdev_get_child_bus

那么为什么 qobject 还是需要单独形成一套?

qdev_get_dev_path 这个打印依赖关系，还是继承关系

## qdev 如何通用的管理热迁移


```txt
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_bh_poll
                      - aio_bh_call
                        - virtio_net_tx_bh
                          - virtio_net_flush_tx
                            - virtio_irq
                              - virtio_notify_vector
                                - qdev_get_parent_bus
```

先需要找到设备的 parent 才可以发送中断:
```c
/* virtio device */
static void virtio_notify_vector(VirtIODevice *vdev, uint16_t vector)
{
    BusState *qbus = qdev_get_parent_bus(DEVICE(vdev));
    VirtioBusClass *k = VIRTIO_BUS_GET_CLASS(qbus);

    if (virtio_device_disabled(vdev)) {
        return;
    }

    if (k->notify) {
        k->notify(qbus->parent, vector);
    }
}
```

## qdev realize 真的很奇怪
- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_init_board
        - machine_run_board_init
          - pc_init1
            - sysbus_realize_and_unref
              - qdev_realize_and_unref
                - qdev_realize
                  - object_property_set_bool
                    - object_property_set_qobject
                      - object_property_set
                        - property_set_bool
                          - device_set_realized
                            - i440fx_pcihost_realize
                              - pci_create_simple
                                - pci_realize_and_unref
                                  - qdev_realize_and_unref
                                    - qdev_realize
                                      - object_property_set_bool
                                        - object_property_set_qobject
                                          - object_property_set
                                            - property_set_bool
                                              - device_set_realized
                                                - pci_qdev_realize


从 qdev_realize 到调用到 hook ，有必要搞这么远吗?

- qdev_realize
  - object_property_set_bool
    - object_property_set_qobject
      - object_property_set
        - property_set_bool
          - device_set_realized
            - i440fx_pcihost_realize


注意，class 的初始化的时间点是唯一的

- main
  - qemu_init
    - qemu_create_machine
      - select_machine
        - object_class_get_list
          - object_class_foreach
            - g_hash_table_foreach
              - object_class_foreach_tramp
                - type_initialize
                  - type_initialize
                    - type_initialize
                      - type_initialize
                        - type_initialize
                          - type_initialize
                            - device_class_init

所以，关键问题在于 device_initfn 的调用不够吗?

- qdev_device_add_from_qdict
  - qdev_new
    - object_new_with_type
      - object_initialize_with_type
        - object_init_with_type
          - object_init_with_type
            - virtio_blk_pci_instance_init
              - virtio_instance_init_common
                - object_initialize_child_with_props
                  - object_initialize_child_with_propsv
                    - object_initialize
                      - object_initialize_with_type
                        - object_init_with_type
                          - virtio_blk_instance_init


看看 virtio_blk_instance_init 和 virtio_blk_device_realize 调用的位置:


- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - monitor_read
                    - readline_handle_byte
                      - monitor_command_cb
                        - handle_hmp_command
                          - handle_hmp_command_exec
                            - handle_hmp_command_exec
                              - hmp_device_add
                                - qdev_device_add
                                  - qdev_device_add_from_qdict
                                    - qdev_new
                                      - object_new_with_type
                                        - object_initialize_with_type
                                          - object_init_with_type
                                            - object_init_with_type
                                              - virtio_blk_pci_instance_init
                                                - virtio_instance_init_common
                                                  - object_initialize_child_with_props
                                                    - object_initialize_child_with_propsv
                                                      - object_initialize
                                                        - object_initialize_with_type
                                                          - object_init_with_type
                                                            - virtio_blk_instance_init
                                  - qdev_device_add_from_qdict
                                    - qdev_realize
                                      - object_property_set_bool
                                        - object_property_set_qobject
                                          - object_property_set
                                            - property_set_bool
                                              - device_set_realized
                                                - pci_qdev_realize  (这个是注册到 DeviceClass 上的，所以需要)
                                                  - virtio_pci_realize (gdb 无法识别函数指针，所以手动补充)
                                                    - virtio_blk_pci_realize (同上，手动补充)
                                                      - object_property_set_bool (是一个新的 qdev )
                                                        - object_property_set_qobject
                                                          - object_property_set
                                                            - property_set_bool
                                                              - device_set_realized
                                                                - virtio_device_realize
                                                                  - virtio_blk_device_realize

1. qdev 的 realize 是如何实现类似构造函数的逐级调用的

qdev_realize(vdev, BUS(&vpci_dev->bus), errp); 只会调用 DeviceClass::realize的

但是注意，在 virtio_pci_class_init 中，virtio_pci_class_init 是注册到 PCIDeviceClass 上的
```txt
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);
        dc->realize = virtio_device_realize;
```

2. 这个例子引入了一些复杂性，这里是存在两个 qdev 的
```c
struct VirtIOBlkPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOBlock vdev;
};
```

## qdev 的还有一部分是 qbus

例如:
```txt
static void virtio_pci_bus_new(VirtioBusState *bus, size_t bus_size,
                               VirtIOPCIProxy *dev)
{
    DeviceState *qdev = DEVICE(dev);
    char virtio_bus_name[] = "virtio-bus";

    qbus_init(bus, bus_size, TYPE_VIRTIO_PCI_BUS, qdev, virtio_bus_name);
}
```

### 这个也看看  sysbus_create_simple
## 总是来说，qdev 是有历史遗留的
hotplug_disk 是中 device_add  和 object_add 共同使用


## 那些 -device 创建的就都是 qdev 吧


## 看看，为什么当时从 init 修改为使用 relize 是叫 QOM realize
commit 726887ef44d5 ("hpet: Use QOM realize for hpet")

## 去看看这个是如何实现的?

pci 设备都是可以配置自己的 address 的
```txt
	# 00:12.0 Non-Volatile memory controller [0108]: Red Hat, Inc. QEMU NVM Express Controller [1b36:0010] (rev 02)
	arg_nvme+=" -device nvme,drive=nvme_basic2,max_ioqpairs=14,serial=$(uuidgen),id=nvme_b2,bus=pci.0,addr=0x12 "
```

## qdev
qdev 出现的位置比 qom 要早，当 qom 出现之后，qdev 按照 qom 的模式重写过。

### realize
device_class_init 中注册了 realized 属性
```c
object_class_property_add_bool(class, "realized", device_get_realized, device_set_realized);
```

- qdev_realize
	- qdev_set_parent_bus : 将 dev 和 bus 联系起来，构建 qtree
	- object_property_set_bool
		- device_set_realized
			- DeviceClass::realized : 调用注册的 hook 函数，将两个函数

```txt
- x86_cpu_new
  - qdev_realize
    - object_property_set_bool
      - object_property_set_qobject
        - object_property_set
          - property_set_bool
            - device_set_realized
              - x86_cpu_realizefn
```

因为 `device_type_info` 实际上也是 qdev, 其初始化的时候自然也会调用**逐级** class_init 和 instance_init 的。
然后每个设备注册的自己的 realize。

[这里](http://people.redhat.com/~thuth/blog/qemu/2018/09/10/instance-init-realize.html) 进一步分析了 realize 和 class_init/instance_init 的区别。

### qdev realize
给大家再介绍一个 QEMU 处理 QOM 非常隐秘的一个点。

```c
PCIBus *i440fx_init(const char *host_type, const char *pci_type, // ...
{
    // ...
    dev = qdev_create(NULL, host_type);
    s = PCI_HOST_BRIDGE(dev);
    b = pci_root_bus_new(dev, NULL, pci_address_space,
                         address_space_io, 0, TYPE_PCI_BUS);
    s->bus = b;
    object_property_add_child(qdev_get_machine(), "i440fx", OBJECT(dev), NULL);
    qdev_init_nofail(dev);
```
review 这个代码，你会发现这里有点不顺眼的地方。

qdev_create 创建 dev 之后，先去调用 pci_root_bus_new ，然后去调用 qdev_init_nofail(dev)
为什么需要在 create 和 init 之间插入一个 pci_root_bus_new 的。

我尝试了一下调换顺序，但是虽然可以启动，但是 seabios 的启动会出现一个停滞。

检查 seabios 的 log 可以发现
```txt
Found 1 serial ports                  <- 首先会在这里卡住
WARNING - Timeout at nvme_wait:144!   <- 报错之后继续
```

最后在 @niugenen 和 @rrwhx 的帮助下，终于找到了下面的 backtrace
```txt
- main
  - machine_run_board_init
    - pc_init1
      - i440fx_init
        - pci_root_bus_new
          - qbus_create
            - qbus_realize
              - object_property_set_bool
                - object_property_set_qobject
                  - property_set_bool
                    - bus_set_realized
                      - pci_bus_realize
```

原来注册到 DeviceClass::realize 的并不是在 property_set_bool 中直接调用的，而是在调用 device_set_realized 中调用的
device_set_realized 除了调用 DeviceClass::realize 的这个 hook 之外，还会调用
- 处理 hotplug
- 将在这个设备上的所有的 child bus 全部 realize

所以，如果将 qdev_create 和 qdev_init_nofail 放到一起初始化，那么会导致 pci_bus_realize 没有被调用
最终导致 pci 设备的 mmio 空间没有被注册。

### qtree
和 qom-tree 非常类似，使用 info qtree 可以获取差不多下面的内容，全部的输出在 [这里](./res/qtree.txt)

```txt
bus: main-system-bus
  type System
  dev: i440FX-pcihost, id ""
    pci-hole64-size = 2147483648 (2 GiB)
    short_root_bus = 0 (0x0)
    x-pci-hole64-fix = true
    x-config-reg-migration-enabled = true
    bypass-iommu = false
    bus: pci.0
      type PCI
      dev: virtio-9p-pci, id ""
        disable-legacy = "off"
        disable-modern = false
        ioeventfd = true
        vectors = 2 (0x2)
        virtio-pci-bus-master-bug-migration = false
```

```c
struct BusState {

    QTAILQ_HEAD(, BusChild) children;
    QLIST_ENTRY(BusState) sibling;
```

```c
struct DeviceState {
    QLIST_HEAD(, BusState) child_bus;
```

- dev 和 bus 是互相交错放置的，这符合物理上设计，总线上挂载设备，总线和总线控制器交互。
  - 在 qbus_init 中间，创建的 bus 的时候，使用 BusState::sibling 将 BusState 挂到 DeviceState::child_bus 上
  - 在 bus_add_child 中，使用 DeviceState::sibling 将 DeviceState 挂到 BusState::children 上

在 qdev_realize -> qdev_set_parent_bus 将会 dev 添加到 bus 上，如果一个 dev 没有关联 bus，类似 hpet 那么就会添加到 main-system-bus 上。


## 如何理解 object_ref 的?

```c
void migration_connect(MigrationState *s, Error *error_in){
    // ...

    /*
     * Take a refcount to make sure the migration object won't get freed by
     * the main thread already in migration_shutdown().
     *
     * The refcount will be released at the end of the thread function.
     */
    object_ref(OBJECT(s));
```


## qom-list 和 info qom-tree 的关系是什么?

你的理解“接近正确”，但 qom-list 和 info qom-tree 展示的维度不同：

- qom-list PATH：列出这个对象的所有属性，只看一层。
- info qom-tree [PATH]：只沿 child<> 属性递归，展示 QOM 组合树。

例如：

qom-list /machine

输出中的：

device-memory[0] (child<memory-region>)
fw_cfg (child<fw_cfg_io>)
peripheral (child<container>)
i440fx (child<i440FX-pcihost>)

确实对应：

/machine
  /device-memory[0]
  /fw_cfg
  /i440fx
  /peripheral

也就是 /machine 的第一层子对象。

但下面这些不会成为树节点：

kernel (string)
graphics (bool)
smp (SMPConfiguration)
smram (link<memory-region>)
acpi-device (link<hotplug-handler>)

原因是它们不是 child<>：

 属性类型               含义                    出现在子树中
━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━
 child<T>               当前对象拥有的子对象    是
─────────────────────  ──────────────────────  ──────────────
 link<T>                指向其他对象的引用      否
─────────────────────  ──────────────────────  ──────────────
 bool/string/int/...    普通配置或状态属性      否

另一个容易误解的地方是：不带路径的 info qom-tree 并不是从全局 QOM 根 / 开
始，而是从 /machine 开始。源码明确使用了 qdev_get_machine()：

if (path) {
    obj = object_resolve_path(path, &ambiguous);
} else {
    obj = qdev_get_machine();
}

见 qom/qom-hmp-cmds.c:135。

因此：

(qemu) info qom-tree

等价于：

(qemu) info qom-tree /machine

如果想看真正的全局 QOM 根：

(qemu) qom-list /
(qemu) info qom-tree /

而：

(qemu) qom-list
/

并没有真正列出根对象属性。HMP 实现发现没有传路径后，仅打印 / 作为提示并返
回，见 qom/qom-hmp-cmds.c:21。

最后，qom-tree 是 info 子命令，所以：

qom-tree          # 错误
info qom-tree     # 正确

还要注意，QOM tree 表示的是对象所有权/组合关系，不等于 PCI、ISA、SCSI 等硬
件总线拓扑。要看设备与总线连接关系，应使用：

info qtree

一句话概括：qom-list /machine 是 /machine 的“属性表”；其中所有 child<>
项，才是 info qom-tree 中 /machine 的第一层节点。



<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
