## snapshot-save 保存内存时复用了 migration/savevm 框架：

  snapshot-save
    -> save_snapshot()
    -> qemu_savevm_state()
    -> migrate_init()
    -> save_setup
    -> save_iterate
    -> complete_precopy

  这和热迁移使用同一套 RAM、设备状态序列化代码，见 /home/martins3/data/qemu/
  migration/savevm.c:1893。

  普通配置下也确实会自动打开 dirty page tracking：

  memory_global_dirty_log_start(GLOBAL_DIRTY_MIGRATION, errp);
  migration_bitmap_sync_precopy(false);

  完成后自动关闭，见 /home/martins3/data/qemu/migration/ram.c:2867 和 /home/
  martins3/data/qemu/migration/ram.c:2455。

  但它并不是通常意义上的“在线热迁移”：

  1. QEMU 先执行 vm_stop(RUN_STATE_SAVE_VM)。
  2. 然后 drain 所有 block I/O。
  3. 最后才进入 qemu_savevm_state() 保存内存。

  调用顺序见 /home/martins3/data/qemu/migration/savevm.c:3317。

  因此：

  - guest 停止后才保存全部内存。
  - 不会像正常热迁移那样，在 guest 运行时做多轮 pre-copy。
  - 初始 migration bitmap 基本把所有 RAM 页标成待保存。
  - 因为 vCPU 已停止，通常不会不断产生新脏页，所以基本是一轮完整扫描。
  - dirty tracking 主要是复用通用 migration RAM 保存框架的结果，并不是这个
    snapshot 算法实现低停机时间的关键。

  最终数据不是发送给目标 QEMU，而是通过 qemu_fopen_bdrv() 写进 vmstate 指定的
  qcow2 节点，见 /home/martins3/data/qemu/migration/savevm.c:3341。

  简化理解就是：

  普通热迁移：
  运行 guest + 开 dirty tracking
    -> 多轮复制脏页
    -> 短暂停机
    -> 复制剩余脏页和设备状态

  snapshot-save：
  先暂停 guest
    -> 开 dirty tracking/初始化 migration bitmap
    -> 完整保存 RAM 和设备状态
    -> 关闭 dirty tracking
    -> 恢复 guest

  另外，snapshot-save 不会自动启用 background-snapshot；如果明确使用 background
  snapshot 迁移能力，RAM 代码反而会跳过传统 dirty log。


## 哦，snapshot 和 savevm 就是相同的机制
  • 是。这里的 QMP snapshot-save/load 和 HMP savevm/loadvm 底层调用的是完全相同的核心函数：

  HMP savevm       ─┐
                    ├─> save_snapshot()
  QMP snapshot-save ┘

  HMP loadvm       ─┐
                    ├─> load_snapshot()
  QMP snapshot-load ┘

  主要区别在接口语义。

   特性                  HMP savevm/loadvm              QMP snapshot-save/load
  ━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━
   核心实现              save_snapshot/load_snapshot    相同
  ────────────────────  ─────────────────────────────  ──────────────────────────
   RAM/设备状态          保存                           保存
  ────────────────────  ─────────────────────────────  ──────────────────────────
   qcow2 内部磁盘快照    保存                           保存
  ────────────────────  ─────────────────────────────  ──────────────────────────
   磁盘选择              自动选择全部合适节点           显式指定 devices
  ────────────────────  ─────────────────────────────  ──────────────────────────
   VMState 节点          自动选择                       显式指定 vmstate
  ────────────────────  ─────────────────────────────  ──────────────────────────
   同名快照              savevm 自动覆盖                snapshot-save 报错
  ────────────────────  ─────────────────────────────  ──────────────────────────
   执行模型              同步 HMP 命令                  异步 QEMU Job
  ────────────────────  ─────────────────────────────  ──────────────────────────
   完成检查              命令返回                       query-jobs + job-dismiss

  HMP savevm 的调用是：

  save_snapshot(name, true, NULL, false, NULL, ...);

  其中：

  - overwrite=true
  - vmstate=NULL：自动选择
  - has_devices=false：自动遍历所有合适的磁盘

  见 /home/martins3/data/qemu/migration/migration-hmp-cmds.c:491。

  QMP snapshot-save 最终调用：

  save_snapshot(tag, false, vmstate, true, devices, ...);

  见 /home/martins3/data/qemu/migration/savevm.c:3633。

  所以现在 collei 使用：

  {
    "vmstate": "boot1",
    "devices": ["boot1"]
  }

  它基本等价于“只对 boot1 执行 savevm”。如果 VM 还有 boot2、virtio-scsi_1 等磁盘，区别就很明显：

  - HMP savevm 通常会尝试给所有合适的磁盘创建同名内部快照。
  - collei 的 snapshot-save 只处理 boot1。

  另外不要与以下命令混淆：

  blockdev-snapshot
  blockdev-snapshot-sync
  blockdev-snapshot-internal-sync

  这些是纯磁盘快照，不保存 RAM 和设备状态，与 savevm 不是同一种机制。

## 那么 snapshot 机制为什么没有和 backgroup-snapshot 机制配合使用?

首先，snapshot 本来就是直接暂停虚拟机运行的，根本就不需要这么花里胡哨的功能。

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
