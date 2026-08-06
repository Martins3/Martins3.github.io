# savevm
## 使用方法

hmp 命令:
```
savevm
```

disk 的 snapshot :
- https://wiki.qemu.org/Features/SnapshotsMultipleDevices

- https://superuser.com/questions/1768173/how-to-work-with-qemu-snapshots-savevm-in-a-way-that-is-predictable-like-virtual

## qemu savevm 基本原理
<!-- 3fcab9c4-a753-4b17-8e79-e5ce5c643b2d -->

首先，vmstate 是一定会保存的，此外，savevm 的时候，机器立刻停止下来，
将所有的内存全部都写入到盘中后，然后继续运行。

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
                      - monitor_command_cb,
                        - handle_hmp_command
                          - handle_hmp_command_exec,
                            - hmp_savevm
                              - save_snapshot,
                                - bdrv_all_create_snapshot,
                                  - bdrv_all_get_snapshot_devices,

保存使用的盘的逻辑参考 `bdrv_all_get_snapshot_devices()`

- `HMP savevm` 默认没有显式指定 `vmstate` 设备
- QEMU 会在默认候选块设备列表里，从前往后找
- 选中第一个“可写、已插入、支持 internal snapshot”的块设备
- 在这台机器当前的候选顺序里，`virtio-scsi_1` 是第一个满足条件的设备

使用 qemu-img snapshot -l "$d" 来查询，结果如下，可见，这个 qcow2 中把 vmstate 完整的保留了:
```txt
./virtio-scsi_1
Snapshot list:
ID        TAG               VM SIZE                DATE     VM CLOCK     ICOUNT
1         vm-20260112055339 1.56 GiB 2026-01-12 18:53:39 00:06:37.803
2         a                1.45 GiB 2026-01-23 22:00:42 00:04:30.948
3         mark             1.14 GiB 2026-01-23 22:04:44 00:00:49.611
4         abc              1.15 GiB 2026-01-23 22:09:20 00:01:37.400
5         vm-20260407172427 4.49 GiB 2026-04-07 17:24:27 00:06:40.465
```

如果想显式控制保存到哪块盘
```json
{
  "execute": "snapshot-save",
  "arguments": {
    "job-id": "snapsave0",
    "tag": "my-snap",
    "vmstate": "某个块节点名",
    "devices": ["盘1节点", "盘2节点"]
  }
}
```
这样可以稳定控制：

- `vmstate` 到底写到哪个块节点
- 哪些磁盘参与这次内部快照


## 基本实验
```txt
🤒  qemu-img snapshot -l boot1
Snapshot list:
ID      TAG               VM_SIZE                DATE        VM_CLOCK     ICOUNT
1       vm-20260112055339      0 B 2026-01-12 05:53:39  0000:06:37.803         --
```

```txt
(qemu) info snapshots
There is no snapshot available.
(qemu) savevm
(qemu) info snapshots
List of snapshots present on all disks:
ID      TAG               VM_SIZE                DATE        VM_CLOCK     ICOUNT
--      vm-20260112055339 1.56 GiB 2026-01-12 05:53:39  0000:06:37.803         --
(qemu) loadvm vm-20260112055339
```

qemu-img snapshot -a vm-20260112055339 disk.qcow2

qemu-img convert \
  -f qcow2 \
  -O qcow2 \
  -s vm-20260112055339 \
  disk.qcow2 \
  disk-from-snapshot.qcow2

savevm/loadvm 也会有这个问题:
也会遇到这个问题
```txt
Error: State blocked by non-migratable device '0000:00:07.0/nvme'
```

这个是新问题哦
```txt
Error: Device 'pflash1' is writable but does not support snapshots
```
## 问题: migration/savevm.c 中的 save_snapshot 怎么理解?

也就是 savevm 实际上用的就是 save_snapshot() 复用一些热迁移的代码吗?

似乎 qd 中 savevm 用的 migrate to file 的方法

## snapshot-save 保存内存时复用了 migration/savevm 框架：

  snapshot-save
    -> save_snapshot()
    -> qemu_savevm_state()
    -> migrate_init()
    -> save_setup
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
