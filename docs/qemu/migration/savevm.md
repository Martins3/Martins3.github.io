# savevm

## 相关命令梳理
<!-- 107efccd-3e2c-4df3-9757-f32ec1d72e57 -->

这里的 QMP snapshot-save/load 和 HMP savevm/loadvm 底层调用的是完全相同的核心函数：

```txt
HMP savevm       ─┐
                  ├─> save_snapshot()
QMP snapshot-save ┘

HMP loadvm       ─┐
                  ├─> load_snapshot()
QMP snapshot-load ┘
```

如下的几个命令是纯磁盘快照，不保存 RAM 和设备状态，与 savevm 不是同一种机制。
- blockdev-snapshot
- blockdev-snapshot-sync
- blockdev-snapshot-internal-sync

参考:
- https://wiki.qemu.org/Features/SnapshotsMultipleDevices
- https://superuser.com/questions/1768173/how-to-work-with-qemu-snapshots-savevm-in-a-way-that-is-predictable-like-virtual

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

```txt
qemu-img convert \
  -f qcow2 \
  -O qcow2 \
  -s vm-20260112055339 \
  disk.qcow2 \
  disk-from-snapshot.qcow2
```

savevm/loadvm 无法处理 nvme ，这是预期的，因为保存 device state:
```txt
Error: State blocked by non-migratable device '0000:00:07.0/nvme'
```

不过额外发现
```txt
Error: Device 'pflash1' is writable but does not support snapshots
```

## 基本原理
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

关键代码在 migration/savevm.c 中的
- save_snapshot
	- qemu_savevm_state
		- qemu_savevm_state_iterate : 这就是我们熟悉的

不过，会观察到，这个过程也是打开了 dirty tracking
主要是复用通用 migration RAM 保存框架的结果

## 那么 snapshot 机制为什么没有和 backgroup-snapshot 机制配合使用?

snapshot 本来就是直接暂停虚拟机运行的，根本就不需要这么花里胡哨的功能。

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
