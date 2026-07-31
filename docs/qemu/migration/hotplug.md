## 基本讨论 : codex
核心思路是：QEMU 在热迁移开始时冻结设备拓扑；迁移设备的运行状态，但通常不迁移“创
建这个设备的操作”。

可以把它分成两层：

设备拓扑/配置
    管理层在目的端重新创建
    ↓
设备运行状态 VMState
    QEMU 通过迁移流复制

### 1. 迁移前已经热插入的设备

设备在 realize 时会把自己的 VMStateDescription 注册进迁移框架，因此启动时创建和后
来热插入的设备，在迁移层面没有本质区别：

- 设备 realize
- 注册 VMState section
- 如果包含 RAM，再注册对应 RAMBlock
- 开始迁移时自动进入迁移流

相关代码见 hw/core/qdev.c:536 和 migration/savevm.c:924。

热插内存的 RAMBlock 也会被标记成可迁移，见 migration/savevm.c:3564。

### 2. 目的端不会根据迁移流自动执行 device_add

目的端通常必须预先存在对应设备，实际由 libvirt 等管理层根据当前 live XML 创建，要
求：

- 相同 machine type
- 相同设备类型
- 相同 bus/slot/function
- 相同设备标识和可迁移属性
- 对应的 memory-backend、netdev、block backend 已准备好

随后 QEMU 根据 VMState 的路径、名称和 instance ID 找到目标设备并加载状态。如果目
标端缺少源端的热插设备，会报：

> Make sure that your current VM setup matches your saved VM setup, including
> any hotplugged devices

见 migration/savevm.c:2703。

所以，迁移流主要携带的是寄存器、队列、feature、控制器状态等，不是完整的
device_add 参数。

### 3. 迁移进行中禁止普通热插拔

从迁移进入 setup 开始，migration_is_running() 就返回 true，覆盖 setup、active、
postcopy、device、cancelling 等状态，见 migration/migration.c:1003。

此时：

- device_add 一律拒绝：

  system/qdev-monitor.c:704

- device_del 默认也拒绝：

  system/qdev-monitor.c:925

这样避免迁移线程已经枚举完设备/RAMBlock 后，拓扑又发生变化。并发的 QMP 请求通过状
态检查和 BQL 串行化：要么设备先完整加入并参与迁移，要么迁移先进入 setup，设备操作
失败。

### 4. 热拔出尤其麻烦

很多热拔出是异步的：

device_del
  → QEMU 通知 guest
  → guest 卸载驱动
  → guest 确认 eject
  → QEMU 才真正 unrealize 设备

真正 unrealize 时，设备的 VMState 才会注销，见 hw/core/qdev.c:590。

因此，运维上应当在迁移前等待 DEVICE_DELETED，不能只看 device_del 命令成功。QEMU
通用迁移框架不会等待所有任意设备的异步拔出完成。

### 5. 特例：virtio-net failover

这是明确允许“为了迁移而热拔出”的特例：

VFIO primary + virtio-net standby
       ↓ 开始迁移
通知 guest 拔掉 VFIO primary
       ↓
迁移进入 WAIT_UNPLUG
       ↓ guest 完成拔出
通过 virtio-net standby 迁移
       ↓
目的端重新插入 primary

带 failover_pair_id 的 PCI primary 会设置 allow_unplug_during_migration=true，见
hw/pci/pci.c:2369。

迁移在 WAIT_UNPLUG 状态等待 guest 完成拔出，见 migration/migration.c:3627。
virtio-net 通过 dev_unplug_pending 报告拔出状态，见 hw/net/virtio-net.c:4164。

这是专门设计的协议，不代表普通 PCI 设备也能在迁移中随意拔出。

### 6. 内存热插拔的额外限制

DIMM 设备在迁移前插入没有问题，但迁移中不能增加、删除或改变 RAMBlock 大小。因为
precopy setup 已经发送了 RAMBlock 的布局和大小；如果运行期间发生 resize，QEMU 会
直接取消迁移，见 migration/ram.c:4725。

virtio-mem 的 guest 内部 plug/unplug 也会在迁移期间返回 busy，以避免源、目的端对
哪些页面有效产生不同理解，见 hw/virtio/virtio-mem.c:196。

### 7. 最终一致性

Precopy 阶段 guest 仍在运行，主要反复复制 RAM。到 switchover 时：

1. 停止 VM；
2. 进入 DEVICE 状态；
3. 完成最后一轮 RAM；
4. 序列化所有非迭代设备状态；
5. 目的端加载状态并恢复运行。

停止 VM 和保存最终设备状态的路径见 migration/migration.c:2860。

简而言之：

> 热插拔发生在迁移前，就作为当前设备拓扑的一部分迁移；迁移开始后，QEMU原则上冻结
> 拓扑。目的端由管理层重建相同设备，QEMU迁移其运行状态。只有 virtio-net failover
> 等专门设计的机制允许迁移期间拔插。

## 热插拔的工作是 libvirt 的工作 : kimi

两份源码都看完了。结论：目标端 QEMU 的命令行在"设备层面"会包含热插的设备（比如
热插的内存会以 -object memory-backend-* + -device pc-dimm 出现），但并不是和源
端逐字节一模一样。两边分工如下：

QEMU 侧：一致性不靠 QEMU 保证，靠管理层；不一致就拒绝迁移

目标端是一个全新进程，先按命令行把整台机器建好（qemu_init_board 完成后才进入
incoming，system/vl.c:2821-2848），收到迁移流后只做字符串匹配，不会从流里重建
任何设备：

• 每个设备的 VMState section 带 idstr（设备 qom 路径）+ instance_id，目标端
  find_se() 匹配不上直接报错（migration/savevm.c:2704-2711）：
  │ Unknown section or instance ... Make sure that your current VM setup
  │ matches your saved VM setup, **including any hotplugged devices**
• pc-dimm 本身没有 VMState（hw/mem/pc-dimm.c 的 class init 没设 dc->vmsd，历史
  上只迁 addr 的 vmstate_pcdimm 已从上游移除）。也就是说 dimm 的
  addr/slot/node/size/memdev 全靠目标端命令行重建，迁移流里只有两样相关的东西
  ：
    • RAMBlock（idstr = 设备 qom 路径 + MR 名），目标端找不到就报 Unknown
      ramblock ... cannot accept migration（migration/ram.c:4281-4288）；
    • ACPI 热插状态寄存器
      （vmstate_memory_hotplug，hw/acpi/memory_hotplug.c:308）。
• 前提还得有 -M maxmem=，否则目标端连 device-memory 区域都没有。

所以：如果目标端命令行漏了热插的内存，迁移不是"静默丢状态"，而是直接失败（落在
ramblock 匹配上）。

libvirt 侧：目标端命令行由源端 live XML 重建，自动包含热插设备

• 热插成功后设备立即插入 live def（内存走
  virDomainMemoryInsert，src/qemu/qemu_hotplug.c:2398），DIMM 的 slot/base 地
  址在热插时由 qemuDomainAssignMemoryDeviceSlot 分配并记录，之后还会从 QEMU 查
  实际地址回写。
• 迁移 Begin 阶段，源端把 live def 序列化为 migratable
  XML（qemuMigrationSrcBeginXML，qemu_migration.c:2678）传给目标端；目标端
  qemuMigrationDstPrepareFresh 用这份 XML 建 domain 对象，再
  qemuBuildCommandLine 生成 argv（qemu_process.c:8094）。热插的内存在
  qemuBuildMemoryDeviceCommandLine（qemu_command.c:7904）里变成完整的 -object
  memory-backend-* + -device pc-dimm，slot/alias/PCI 地址与源端一致。
• 迁移 cookie 不传设备配置，只有 persistent XML、CPU、QEMU caps 等，外加
  MEMORY_HOTPLUG/CPU_HOTPLUG 强制特性标志（防止旧 libvirt 不认识热插配置时静默
  出错）。

和源端命令行的刻意差异

虽然由同一份 XML 构建，但目标端会：

• 多一个 -incoming defer；
• monitor socket、pidfile、chardev/logfile 等运行时路径按目标端主机重新生成；
• 内存后端文件路径按目标端 qemu.conf（hugetlbfs 挂载点、memory_backing_dir）重
  算；
• TLS creds、NBD server 等迁移相关对象不走命令行，启动后经 QMP 添加。

一句话总结：设备构成（含热插的 dimm）一致，但参数不是一模一样——QEMU 要求的是"
配置可匹配"（idstr/RAMBlock/地址对得上），逐字节相同从来不是要求，也不可能（两
端路径本就不同）。

## 测试效果

8G 热插 4G ，然后对 target 端启动的结果:
```txt
	-m size=8388608k,slots=255,maxmem=4194304000k
	-object memory-backend-ram,id=memdimm0,size=4294967296
	-device pc-dimm,node=0,memdev=memdimm0,id=dimm0,slot=0,addr=9663676416
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
