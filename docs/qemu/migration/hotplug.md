# 热插拔后，如何热迁移

1. 迁移前已经热插入的设备

设备在 realize 时会把自己的 VMStateDescription 注册进迁移框架，因此启动时创建和后
来热插入的设备，在迁移层面没有本质区别：

- 设备 realize
- 注册 VMState section
- 如果包含 RAM，再注册对应 RAMBlock
- 开始迁移时自动进入迁移流

## 热插拔的工作是 libvirt 的工作

两份源码都看完了。结论：目标端 QEMU 的命令行在"设备层面"会包含热插的设备（比如
热插的内存会以 -object memory-backend-* + -device pc-dimm 出现），但并不是和源
端逐字节一模一样。两边分工如下：

QEMU 侧：一致性不靠 QEMU 保证，靠管理层；不一致就拒绝迁移

目标端是一个全新进程，先按命令行把整台机器建好（qemu_init_board 完成后才进入
incoming，system/vl.c:2821-2848），收到迁移流后只做字符串匹配，不会从流里重建
任何设备：

- 每个设备的 VMState section 带 idstr（设备 qom 路径）+ instance_id，目标端 find_se() 匹配不上直接报错（migration/savevm.c)
```txt
Unknown section or instance ... Make sure that your current VM setup
matches your saved VM setup, **including any hotplugged devices**
  ```
- pc-dimm 本身没有 VMState（hw/mem/pc-dimm.c 的 class init 没设 dc->vmsd，历史
  上只迁 addr 的 vmstate_pcdimm 已从上游移除）。也就是说 dimm 的
  addr/slot/node/size/memdev 全靠目标端命令行重建，迁移流里只有两样相关的东西
    - RAMBlock（idstr = 设备 qom 路径 + MR 名），目标端找不到就报 Unknown
      ramblock ... cannot accept migration（migration/ram.c:4281-4288）；
    - ACPI 热插状态寄存器
      （vmstate_memory_hotplug，hw/acpi/memory_hotplug.c:308）。
- 前提还得有 -M maxmem=，否则目标端连 device-memory 区域都没有。

所以：如果目标端命令行漏了热插的内存，迁移不是"静默丢状态"，而是直接失败（落在
ramblock 匹配上）。

libvirt 侧：目标端命令行由源端 live XML 重建，自动包含热插设备

- 热插成功后设备立即插入 live def（内存走
  virDomainMemoryInsert，src/qemu/qemu_hotplug.c:2398），DIMM 的 slot/base 地
  址在热插时由 qemuDomainAssignMemoryDeviceSlot 分配并记录，之后还会从 QEMU 查
  实际地址回写。
- 迁移 Begin 阶段，源端把 live def 序列化为 migratable
  XML（qemuMigrationSrcBeginXML，qemu_migration.c:2678）传给目标端；目标端
  qemuMigrationDstPrepareFresh 用这份 XML 建 domain 对象，再
  qemuBuildCommandLine 生成 argv（qemu_process.c:8094）。热插的内存在
  qemuBuildMemoryDeviceCommandLine（qemu_command.c:7904）里变成完整的 -object
  memory-backend-* + -device pc-dimm，slot/alias/PCI 地址与源端一致。
- 迁移 cookie 不传设备配置，只有 persistent XML、CPU、QEMU caps 等，外加
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
