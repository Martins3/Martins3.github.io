# CPR

## CPR: CheckPoint and Restart

核心代码位置：
- migration/cpr.c:24：CPR 核心状态管理，保存/恢复需要跨 QEMU 保留的 FD；主要入口是 cpr_state_save()、cpr_state_load()。
- migration/cpr-transfer.c:24：cpr-transfer 实现，通过 Unix socket 和 SCM_RIGHTS 传递 FD。
- migration/cpr-exec.c

  主流程入口：

  - migration/migration.c:2105：qmp_migrate() 调用 cpr_state_save()，然后启动主迁移。
  - migration/migration.c:1669：migrate_mode_is_cpr() 判断三种 CPR 模式。
  - migration/migration.c:2031：CPR 与 postcopy、background snapshot、COLO 等功能的冲突检查。
  - system/vl.c:3784：目标 QEMU 在创建设备前调用 cpr_state_load()，因为设备初始化可能需要恢复的 FD。
  - migration/channel.c:336：解析并检查 CPR migration channel。

VFIO CPR 支持主要在：
- hw/vfio/cpr.c:18：VFIO PCI 状态、IRQ/MSI FD 的保存恢复。
- hw/vfio/cpr-legacy.c:30：legacy VFIO container 和 DMA remap。
- hw/vfio/cpr-iommufd.c:20：IOMMUFD 的 IOAS/HWPT 状态。
- include/hw/vfio/vfio-cpr.h:20：VFIO CPR 接口和数据结构。

  文档和测试：

  QMP migrate
    → qmp_migrate()
    → cpr_state_save()
    → cpr-transfer socket / cpr-exec memfd
    → 普通 migration 保存 VM 与 RAM 状态
    → 新 QEMU 启动
    → cpr_state_load()（创建设备之前）
    → 普通 incoming migration 恢复 VM


## qemu cpr 基本理解
<!-- 12077840-e151-46d5-bafb-41c249f7fd30 -->

https://www.qemu.org/docs/master/devel/migration/CPR.html

```txt
CPR is the umbrella name for a set of migration modes in which the VM is migrated to a new QEMU instance on the same host. It is intended for use when the goal is to update host software components that run the VM, such as QEMU or even the host kernel. At this time, the cpr-reboot, cpr-transfer, and cpr-exec modes are available.
```

CPR (CheckPoint and Restart - 检查点与重启)

CPR 是 QEMU 中一组特殊迁移模式的总称，其核心特点是将虚拟机 (VM) 迁移到同一台宿主机上的新 QEMU
实例。其设计目标主要是为了在需要更新运行 VM 的宿主机软件组件（如 QEMU 本身或宿主机内核）时，能够平滑地恢复 VM 运行。

关键特点：

 * 同宿主机迁移: 利用同一宿主机的资源（特别是本地设备），使得在某些情况下 CPR 可行而普通跨宿主机迁移会被阻止。
 * 禁止修改块设备: 在旧 QEMU 退出和新 QEMU 启动之间，绝对不能修改客户机的块设备内容。
 * 无条件暂停: 在保存内存前会无条件停止 VM，因此不依赖脏页追踪机制。

三种 CPR 模式：

1. `cpr-reboot`:
	* 如果虚拟机运行的内存本来就是持久化内存，相对于 savevm 或者 mnigrate -d file:/tmp/vm.img 可以不用保存 ram 文件
	* 支持 VFIO 设备，但需先将客户机置于挂起状态（如使用 guest-suspend-ram 命令）。
	* 新旧 QEMU 实例不会同时运行。

2. `cpr-transfer`:
	* 通过在原地保留客户机 RAM（使用共享内存后端）和设备文件描述符（利用 SCM_RIGHTS 通过专用 CPR 通道传递，通常是 Unix 域套接字），实现极短暂停时间的迁移。
	* 需要预先启动新 QEMU（指定主迁移通道和 CPR 通道），然后触发旧 QEMU 的迁移。
	* 新旧 QEMU 实例会短暂共存。
	* 未来可能扩展支持 vhost、char 等设备的无缝传递。

3. `cpr-exec`:
	* 旧 QEMU 停止 VM 并保存状态，然后直接 exec 调用一个新的 QEMU 进程（替换自身，保持 PID 不变）来加载状态。
	* 实现了 QEMU 二进制文件的无缝更新，同时保留设备状态（如 VFIO/IOMMUFD）。
	* 通过 cpr-exec-command 参数指定新 QEMU 的启动命令。
	* 新旧 QEMU 实例不会同时运行。

共同要求与限制：

 * 通常都需要配置共享内存 (share=on) 和 aux-ram-share=on。
 * 均不能与 postcopy、background-snapshot 或 COLO 功能同时使用。
 * cpr-exec 模式还需要系统调用 exec 的权限，可能受沙箱设置影响。

1. 也就是说，cpr-transfer 和 cpr-exec 可以处理热迁移相关的操作? 可以测试一下
2. cpr-reboot 和 migrate 到文件的操作很像啊
3. 无条件暂停听上去是合理的
	- cpr-reboot 就是让虚拟机暂停，而另外的两个操作都很快
	- 所以 migration to file 还让虚拟机继续运行，就是方便调试而已了

- cpr-reboot mode may not be used with postcopy, background-snapshot, or COLO.
- Memory backend objects must have the share=on attribute. The VM must be started with the -machine aux-ram-share=on option.
	- 那么也就是 share=on 还可以

## 一共三个模式来保存
1. cpr reboot
3. migrate -d file:/tmp/vm.img
3. savevm / loadvm

## 三个东西
这里我们主要思考三个问题:
1. snapshot vm (userfault fd ？)
2. 虚拟机暂停恢复
3. QEMU 热升级
4. snapshot 可以用于哪里?
	- migration

- [ ] 思考一个问题，既然可以让 snapshot 和 migration 两个功能类似的功能放到一起，
那么是不是 upgrade 的功能可以类似的放到一起的。

似乎都是在  migration/savevm.c 中处理的?

1. 类似的场景需求
https://github.com/cloudflare/shellflip

1. cpr 的 trace 可以看到这些
热插的时候，可以看到这个日志
```txt
cpr_find_fd hp_mem0, id 0 returns -1
cpr_save_fd hp_mem0, id 0, fd 327
```
3. backends/hostmem-memfd.c 中的 cpr_save_fd(name, 0, fd); 是做什么的?

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
