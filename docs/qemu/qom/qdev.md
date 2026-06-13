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
## 既然 qdev 都已经封装好了，为什么 iothread 不去直接服用?

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
