# qemu 热插拔和热迁移
<!-- 74ba90da-89de-47fb-a27b-eac52b14d31e -->

## 热迁移前进行了热插
1. 迁移前已经热插入的设备

设备在 realize 时会把自己的 VMStateDescription 注册进迁移框架，因此启动时创建和后来热插入的设备，在迁移层面没有本质区别：
- 设备 realize
- 注册 VMState section
- 如果包含 RAM，再注册对应 RAMBlock
- 开始迁移时自动进入迁移流

### 热插拔主要逻辑在 libvirt 中

目标端是一个全新进程，先按命令行把整台机器建好，收到迁移流后只做字符串匹配，不会从流里重建
任何设备：

每个设备的 VMState section 带 idstr（设备 qom 路径）+ instance_id，目标端 find_se() 匹配不上直接报错（migration/savevm.c)
```txt
Unknown section or instance ... Make sure that your current VM setup
matches your saved VM setup, **including any hotplugged devices**
```

8G 热插 4G ，然后对 target 端启动的结果:
```txt
	-m size=8388608k,slots=255,maxmem=4194304000k
	-object memory-backend-ram,id=memdimm0,size=4294967296
	-device pc-dimm,node=0,memdev=memdimm0,id=dimm0,slot=0,addr=9663676416
```

## 热迁移和热插的同步问题

1. 热插和热迁移是顺序的，而热插是一个同步的操作

2. qmp_migrate() 调用 migrate_prepare()，最终由 migrate_init() 把状态从 NONE 改成 SETUP：
```txt

migrate_set_state(&s->state,
                  MIGRATION_STATUS_NONE,
                  MIGRATION_STATUS_SETUP);
```

见 migration/migration.c:1709。

```c
if (migration_is_running()) {
    error_setg(errp, "device_add not allowed while migrating");
    return NULL;
}
```

所以，就不用考虑这个情况了。

## 热迁移和热拔的同步问题

基本上可以按照 qdev_unplug 中，来做划分:
```c
    /* If device supports async unplug just request it to be done,
     * otherwise just remove it synchronously */
    hdc = HOTPLUG_HANDLER_GET_CLASS(hotplug_ctrl);
    if (hdc->unplug_request) {
        hotplug_handler_unplug_request(hotplug_ctrl, dev, &local_err);
    } else {
        hotplug_handler_unplug(hotplug_ctrl, dev, &local_err);
        if (!local_err) {
            object_unparent(OBJECT(dev));
        }
    }
```

2026-08-25 : 不过，我大致知道了，unplug 机制还是有点复杂的，会出现 async 的情况，先就这样了。

### 特殊设备
典型特例是 virtio-net failover :
迁移会进入 WAIT_UNPLUG，通过 qemu_savevm_wait_unplug() 等待主 VFIO 网卡完全拔除

### virtio-scsi

hotplug_handler_unplug(...);
object_unparent(...);

virtio_scsi_hotunplug() 内部立即调用 qdev_simple_device_unplug_cb()，见 hw/scsi/virtio-scsi.c:1182。

### virtio-blk-pci

这里要分两个阶段理解。

第一阶段，qdev_unplug() 走 if：

```txt
if (hdc->unplug_request) {
    hotplug_handler_unplug_request(...);
}
```

因为 img 是 PCI 设备，挂在 i440fx/PIIX4 的 PCI root bus 上，hotplug handler 是 PIIX4 ACPI 控制器。PIIX4 同时注册了：

```txt
hc->unplug_request = piix4_device_unplug_request_cb;
hc->unplug = piix4_device_unplug_cb;
```

请求路径：

qdev_unplug
  → hotplug_handler_unplug_request
  → piix4_device_unplug_request_cb
  → acpi_pcihp_device_unplug_request_cb
      → pending_deleted_event = true
      → 设置 ACPI down bit
      → 向 Guest 发 ACPI event

第二阶段，Guest ACPI 驱动响应 eject 请求，QEMU 再走：

acpi_pcihp_eject_slot
  → hotplug_handler_unplug
  → piix4_device_unplug_cb
  → qdev_unrealize
  → object_unparent

所以 GDB 随后又看到了：

TRACE ELSE/completion hotplug_handler_unplug id=img

但这不是重新进入 qdev_unplug() 的 else，而是 Guest 确认后的异步完成路径直接调用 hotplug_handler_unplug()，
见 hw/acpi/pcihp.c:178。

Guest 日志也确认了 PCI 异步 eject：

```txt
ACPI: \_SB_.PCI0.S20_: Eject request in hotplug_event()
virtio-pci 0000:00:04.0: device released
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
