# PCIDevice::net_failover 与热迁移

`PCIDevice::failover_pair_id` 是 QEMU 里 **virtio-net failover** 机制在 PCI
设备侧的配对标记，和热迁移关系密切。

## 1. 基本语义

在 `include/hw/pci/pci_device.h:178` 里：

```c
/* ID of standby device in net_failover pair */
char *failover_pair_id;
```

它表示：**当前 PCI 设备（primary，通常是 vfio-pci 直通网卡）所属的 failover 对的
standby 设备 ID**。 对应 standby 设备是 `virtio-net-pci`，启动时带 `failover=on`。

典型配置：

```bash
# standby：virtio-net，开启 STANDBY feature
-device virtio-net-pci,netdev=hostnet1,id=net1,mac=52:54:00:6f:55:cc,failover=on

# primary：vfio-pci 直通网卡，用 failover_pair_id 指向 standby
-device vfio-pci,host=5e:00.2,id=hostdev0,failover_pair_id=net1
```

guest 内核里 `net_failover` 模块则通过 **相同 MAC 地址**
把两个网卡再绑定成一对。

## 2. 为什么需要 failover

vfio-pci/SR-IOV 这类直通设备性能高，但通常
**不能热迁移**（设备状态绑定物理硬件）。\
failover 的设计是：

- 平时：流量走 fast primary（vfio-pci）。
- 迁移时：把 primary 热拔除，流量切到 virtio-net standby。
- 目标端：迁移完成后，再把 primary 热插回去，流量恢复走硬件。

这样就在不中断网络的前提下完成 live migration。

## 3. 热迁移中的状态流转

QEMU 为 failover 增加了一个迁移状态 `wait-unplug`（见
`docs/system/virtio-net-failover.rst:66`）：

1. 发起迁移时，如果配置里存在 failover primary 设备，迁移进入 `wait-unplug` 状态。
2. QEMU 通过 PCIe hotplug 通知 guest 拔除 primary 设备。
3. guest 的 `net_failover` 把流量切到 virtio-net standby。
4. 拔除完成后，迁移进入 active 状态，真正开始传输 VM 状态。
5. 目标端恢复后，若 virtio-net 已协商 `VIRTIO_NET_F_STANDBY`，自动把 primary vfio-pci 热插回去。
6. guest 重新识别 primary，流量切回硬件。

## 4. 关键点总结

| 概念               | 含义                                                              |
| ------------------ | ----------------------------------------------------------------- |
| `failover_pair_id` | primary PCI 设备指向 standby virtio-net 的 ID，仅 QEMU 内部配对用 |
| primary            | vfio-pci 等高性能直通网卡                                         |
| standby            | 带 `failover=on` 的 virtio-net                                    |
| guest 内配对       | `net_failover` 模块按相同 MAC 地址匹配                            |
| 迁移状态           | `wait-unplug`：等 guest 完成 primary 拔除后再真正迁移             |

所以 `PCIDevice::failover_pair_id` 的核心作用就是：
**让 QEMU 在迁移前后知道该拔哪张 standby 对应的 primary
网卡、又该在目标端插回哪一对设备**。

### MIGRATION_STATUS_WAIT_UNPLUG 状态的含义

> QEMU 正在等待 guest OS 完成设备热拔除，然后才能正式开始迁移。

典型场景是 virtio-net-failover：

- VM 同时拥有一个可迁移的 virtio standby 网卡和一个不可直接迁移的 primary 设备，例如 VFIO 直通网卡。
- 迁移开始时，QEMU请求 guest 热拔除 primary 设备。
- guest 驱动处理请求并返回确认。
- 确认完成后，只迁移 virtio standby 设备，目的端随后可以重新接入 primary 设备。

正常状态转换：

SETUP
  |
  | 存在待完成的 guest unplug
  v
WAIT_UNPLUG
  |
  | guest 确认热拔除
  v
ACTIVE

如果没有设备需要拔除，则直接：

SETUP -> ACTIVE

几个关键点：

- 它主要是源端状态。
- 它发生在正式 RAM precopy 迭代之前。
- 此时 guest 通常还在运行，以便 guest OS 和驱动处理热拔除请求。
- 它与 PRE_SWITCHOVER 不同：WAIT_UNPLUG 等 guest 驱动，PRE_SWITCHOVER 等管理端执行 migrate-continue。
- 正常等待没有固定超时。如果 guest 驱动不支持、设备配置错误或热拔除事件丢失，迁移可能长期停在这里。
- 如果此时执行取消，会转入 CANCELLING。QEMU还会最多等待约 30 秒让已经开始的 unplug 完成，以便安全恢复设备；超时会打印 partially unplugged device 警告。

可以把它理解成迁移前的“卸下不可携带设备”步骤：

准备迁移
  -> 通知 guest 卸下直通设备
  -> WAIT_UNPLUG
  -> guest 完成卸载
  -> 开始正式迁移

核心逻辑在 migration/migration.c:qemu_savevm_wait_unplug()。

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
