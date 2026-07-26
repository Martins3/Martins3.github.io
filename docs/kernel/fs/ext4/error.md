# ext4 的错误处理路径


## scsi_debug 错误注入

```sh
sudo modprobe scsi_debug

disk=/dev/sda
disk=/dev/sda
sudo mkfs.ext4 -F "$disk"
sudo mount "$disk" /mnt
# sudo chown -R martins3 /mnt

echo a > /mnt/a

# echo 30000000 | sudo tee /sys/module/scsi_debug/parameters/delay
echo 1 | sudo tee /sys/bus/pseudo/drivers/scsi_debug/every_nth
# RECOVERED_ERROR
# echo 8 | sudo tee /sys/bus/pseudo/drivers/scsi_debug/opts
# ABORTED_COMMAND
echo 0x10 | sudo tee /sys/bus/pseudo/drivers/scsi_debug/opts
```

触发的结果 ro 的路径为:

- ret_from_fork_asm
  - ret_from_fork
    - kthread
      - ext4_lazyinit_thread
        - ext4_lazyinit_thread
          - ext4_run_li_request
            - ext4_mb_prefetch_fini
              - ext4_mb_init_group
                - ext4_mb_init_cache
                  - ext4_wait_block_bitmap
                    - __ext4_error
                      - ext4_handle_error

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_getdents64
        - __se_sys_getdents64
          - __do_sys_getdents64
            - iterate_dir
              - file_accessed
                - touch_atime
                  - generic_update_time
                    - __mark_inode_dirty
                      - ext4_dirty_inode
                        - __ext4_journal_start
                          - __ext4_journal_start_sb
                            - ext4_journal_check_start
                              - __ext4_error
                                - ext4_handle_error

的确可以观察到:
```txt
sd 0:0:0:0: [sda] tag#158 CDB: opcode=0x2a 2a 08 00 00 00 02 00 00 02 00
I/O error, dev sda, sector 2 op 0x1:(WRITE) flags 0x820800 phys_seg 1 prio class 3
Buffer I/O error on dev sda, logical block 1, lost sync page write
EXT4-fs (sda): I/O error while writing superblock
EXT4-fs (sda): Remounting filesystem read-only
```

### old thing
ext4_shutdown 和 ext4_force_shutdown 都没有触发

```txt
@[
    ext4_handle_error+1
    __ext4_error+298
    ext4_journal_check_start+130
    __ext4_journal_start_sb+63
    ext4_dirty_inode+56
    __mark_inode_dirty+87
    touch_atime+415
    iterate_dir+272
    __x64_sys_getdents64+136
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 1
@[
    ext4_handle_error+1
    __ext4_error+298
    ext4_read_inode_bitmap+960
    __ext4_new_inode+935
    ext4_create+297
    path_openat+3750
    do_filp_open+179
    do_sys_openat2+171
    __x64_sys_openat+110
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 1
@[
    ext4_handle_error+1
    __ext4_error+298
    ext4_wait_block_bitmap+156
    ext4_mb_init_cache+397
    ext4_mb_init_group+238
    ext4_mb_prefetch_fini+125
    ext4_lazyinit_thread+1071
    kthread+229
    ret_from_fork+49
    ret_from_fork_asm+27
]: 1
@[
    ext4_handle_error+1
    __ext4_error+298
    __ext4_journal_get_write_access+316
    ext4_init_inode_table+456
    ext4_lazyinit_thread+952
    kthread+229
    ret_from_fork+49
    ret_from_fork_asm+27
]: 1
```
一次 io 错误，几乎必然可以导致系统挂掉。
只要打开了文件，不进行任何的 io ，就会 dirty io

系统中总是有人在执行各种程序，打开程序之类，必然导致写 inode 的。

看来触发的位置还是很多的。

## 拔盘测试

qemu hmp
```txt
device_del boot1
```

### kimi
当 ext4 被强制 shutdown 后：

- 文件系统变成只读/不可写状态；
- 后续的写操作会返回 -EIO；
- mount 选项里会显示 shutdown，表示“该文件系统已经通过 EXT4_IOC_SHUTDOWN ioctl 或 abort 等手段被关闭”。

相关源码：

- fs/ext4/super.c 的 ext4_show_options() 会在 ext4_forced_shutdown(sb) 为真时输出 shutdown：
```c
if (ext4_forced_shutdown(sb))
        SEQ_OPTS_PUTS("shutdown");
```
- fs/ext4/ioctl.c 的 ext4_force_shutdown() 实现了强制 shutdown 逻辑。

相关文档

内核文档中与“异常关机/崩溃”相关的 ext4 mount option 记录在：

- Documentation/admin-guide/ext4.rst

其中提到的相关选项包括：

┌────────────────────────────────────┬─────────────────────────────────────────────────────────────┐
│ 选项                               │ 作用                                                        │
├────────────────────────────────────┼─────────────────────────────────────────────────────────────┤
│ barrier / nobarrier                │ 控制 write barrier，保证日志提交顺序，异常掉电时更安全      │
├────────────────────────────────────┼─────────────────────────────────────────────────────────────┤
│ data=ordered / journal / writeback │ 控制数据与元数据的写入顺序，writeback                       │
│                                    │ 在异常关机后可能暴露旧数据                                  │
├────────────────────────────────────┼─────────────────────────────────────────────────────────────┤
│ commit=nrsec                       │ 限制事务最大提交间隔，默认 5 秒                             │
├────────────────────────────────────┼─────────────────────────────────────────────────────────────┤
│ norecovery / noload                │ 挂载时不回放日志，文件系统可能处于不一致状态                │
├────────────────────────────────────┼─────────────────────────────────────────────────────────────┤
│ abort                              │ 调试用途，模拟调用 ext4_abort()，会使文件系统进入 shutdown  │
│                                    │ 状态                                                        │
├────────────────────────────────────┼─────────────────────────────────────────────────────────────┤
│ errors=remount-ro / continue /     │ 遇到文件系统错误时的行为                                    │
│ panic                              │                                                             │
└────────────────────────────────────┴─────────────────────────────────────────────────────────────┘

所以如果你看到的是 ro,shutdown
这样的挂载选项，它说明的是文件系统当前状态，而不是一个可以手动挂载时设置的选项。


### kimi 2

从代码可以明确看到默认值逻辑：

```c
  if (le16_to_cpu(es->s_errors) == EXT4_ERRORS_PANIC)
      set_opt(sb, ERRORS_PANIC);
  else if (le16_to_cpu(es->s_errors) == EXT4_ERRORS_CONTINUE)
      set_opt(sb, ERRORS_CONT);
  else
      set_opt(sb, ERRORS_RO);
```

默认行为

默认是 errors=remount-ro（遇到错误后把文件系统重挂载为只读）。

也就是说：

• errors=panic：显式设置才生效
• errors=continue：显式设置才生效
• errors=remount-ro：默认行为

当 superblock 里的 s_errors 字段既不是 PANIC 也不是 CONTINUE 时，内核就会按 REMOUNT_RO
处理。这个默认值通常也是 mke2fs/tune2fs 创建 ext4 文件系统时的默认设置。

(这就非常奇怪了，如果这个就是默认数值，为什么不会 ro 了)

### kimi 3

- ret_from_fork_asm
  - ret_from_fork
    - kthread
      - worker_thread
        - process_scheduled_works
          - process_one_work
            - virtscsi_handle_event
              - virtscsi_handle_transport_reset
                - scsi_remove_device
                  - __scsi_remove_device
                    - device_del
                      - bus_remove_device
                        - device_release_driver
                          - device_release_driver_internal
                            - __device_release_driver
                              - sd_remove
                                - del_gendisk
                                  - __del_gendisk
                                    - blk_report_disk_dead
                                      - bdev_mark_dead
                                        - fs_bdev_mark_dead
                                          - ext4_shutdown

同时伴随日志为:
```txt
[   80.928151] device offline error, dev sdb, sector 8 op 0x1:(WRITE) flags 0x803000 phys_seg 1 prio class 2
[   80.928759] Buffer I/O error on dev sdb, logical block 1, lost async page write
[   80.929198] device offline error, dev sdb, sector 8472 op 0x1:(WRITE) flags 0x803000 phys_seg 1 prio class 2
[   80.929708] Buffer I/O error on dev sdb, logical block 1059, lost async page write
[   80.930110] device offline error, dev sdb, sector 74008 op 0x1:(WRITE) flags 0x803000 phys_seg 1 prio class 2
[   80.930616] Buffer I/O error on dev sdb, logical block 9251, lost async page write
[   80.931012] device offline error, dev sdb, sector 4194304 op 0x1:(WRITE) flags 0x803000 phys_seg 1 prio class 2
[   80.931538] Buffer I/O error on dev sdb, logical block 524288, lost async page write
[   80.931952] device offline error, dev sdb, sector 4194432 op 0x1:(WRITE) flags 0x803000 phys_seg 1 prio class 2
[   80.932476] Buffer I/O error on dev sdb, logical block 524304, lost async page write
[   80.932865] device offline error, dev sdb, sector 4194560 op 0x1:(WRITE) flags 0x803000 phys_seg 1 prio class 2
[   80.933460] Buffer I/O error on dev sdb, logical block 524320, lost async page write
[   80.933877] device offline error, dev sdb, sector 4260096 op 0x1:(WRITE) flags 0x803000 phys_seg 1 prio class 2
[   80.934300] Buffer I/O error on dev sdb, logical block 532512, lost async page write
[   80.935118] device offline error, dev sdb, sector 0 op 0x1:(WRITE) flags 0x800 phys_seg 0 prio class 2
[   80.943250] EXT4-fs (sdb): shut down requested (2)
[   80.944256] Aborting journal on device sdb-8.
[   80.946061] device offline error, dev sdb, sector 8650752 op 0x1:(WRITE) flags 0x829800 phys_seg 1 prio class 2
[   80.946624] Buffer I/O error on dev sdb, logical block 1081344, lost sync page write
[   80.947038] JBD2: I/O error when updating journal superblock for sdb-8.
[   80.975876] sd 1:0:1:0: [sdb] Synchronizing SCSI cache
[   80.976387] sd 1:0:1:0: [sdb] Synchronize Cache(10) failed: Result: hostbyte=0x04 driverbyte=DRIVER_OK
```

用 sudo umount -l /home/martins3/mnt 来清理

### shutdown 机制的加入
super_operations::shutdown 是在 Linux 6.5-rc1 周期引入的：

- 框架 commit：87efb39075be —— fs: add a method to shut down the file system
    - 作者：Christoph Hellwig
    - 时间：2023-06-01
- ext4 接入 commit：f5db130d4443 —— ext4: wire up sops->shutdown
    - 时间：同一天

作用

->shutdown 是 VFS 层给文件系统的一个回调，在底层块设备被标记为 dead
时调用，用于通知文件系统“底层设备已经不可用，请把自己关掉”。

调用路径大致是：

```text
  块设备出错/被移除
    → blk_holder_ops::mark_dead / fs_bdev_mark_dead
    → fs/super.c
    → sb->s_op->shutdown(sb)
```

在 ext4 里，它对应的是：

```c
  static void ext4_shutdown(struct super_block *sb)
  {
      ext4_force_shutdown(sb, EXT4_GOING_FLAGS_NOLOGFLUSH);
  }
```

也就是强制关闭文件系统，不回放/刷新日志，之后文件系统进入只读状态，后续写操作返回 -EIO。

和 EXT4_IOC_SHUTDOWN 的关系

ext4 本身早就有强制 shutdown 的能力（通过 ioctl：EXT4_IOC_SHUTDOWN），6.5 新增的 ->shutdown
操作是把这个能力接入了 VFS/块设备层的统一机制，使得块设备异常失效时内核能自动触发文件系统
shutdown，而不需要用户态手动调用 ioctl。

文档

内核里没有单独的文档专门讲 super_operations::shutdown，它的设计和用途主要在 commit message
里说明。ext4 的 shutdown 挂载选项显示，也只是反映“文件系统已被
shutdown”的状态，而不是一个用户可以挂载时指定的选项。

## 为什么现在没有 ro 了


• 结论：mount -o errors=panic 可以触发 panic，但不能保证在 device_del/磁盘移除场景触发。是否 panic 取决于 ext4 错误处理和 block-device shutdown 谁先到
  达。

  你的日志没有触发 panic/readonly，是符合代码逻辑的。

  调用时序大致是：

  del_gendisk
    └─ blk_report_disk_dead
        └─ fs_bdev_mark_dead
            ├─ sync_filesystem
            │   └─ 普通文件数据写回失败
            │       └─ Buffer I/O error       # 不进入 ext4 errors= 策略
            └─ ext4_shutdown
                └─ EXT4_FLAGS_SHUTDOWN
                └─ jbd2_journal_abort(-ESHUTDOWN)

  之后所有 ext4_error/ext4_std_error
    └─ 发现 SHUTDOWN
        └─ 直接返回，不再检查 errors=panic

  关键点如下。

  1. 日志中的 Buffer I/O error 不是 ext4 filesystem error

  这些日志来自数据页写回完成路径，例如 fs/ext4/page-io.c:94。它主要设置 mapping error、记录数据丢失，不直接调用 ext4_handle_error()。

  所以默认的：

  errors=remount-ro
  errors=panic

  都不会仅仅因为普通文件数据写失败而执行。

  2. 设备死亡走独立 shutdown 路径

  fs/super.c:1456 最终调用 fs/ext4/super.c:1471，后者直接执行：

  ext4_force_shutdown(sb, EXT4_GOING_FLAGS_NOLOGFLUSH);

  fs/ext4/ioctl.c:810 设置 EXT4_FLAGS_SHUTDOWN 并 abort journal，但不调用 ext4_handle_error()，因此不检查 ERRORS_PANIC。

  后续 ext4 操作通过 ext4_emergency_state() 返回 -EIO。严格说并不是“阻塞”，而是拒绝访问；也不是正常 readonly 的 -EROFS 状态。

  3. errors=panic 什么时候能触发

  只有错误在 shutdown 标志生效前进入 fs/ext4/super.c:690，才会执行其中的 panic 判断。

  典型情形包括：

  - inode/block bitmap、group descriptor 等元数据读取失败；
  - journal transaction start/stop 返回错误并调用 ext4_std_error()；
  - 元数据 checksum 或结构一致性检查失败；
  - JBD2 已 abort，另一个线程在 shutdown 前开始新的 journal transaction；
  - 其他线程已经通过了 __ext4_error() 开头的 shutdown 检查，此时即使 shutdown 随后置位，它仍可能继续到 ext4_handle_error() 并 panic。

  还必须满足：

  test_opt(sb, ERRORS_PANIC) && !system_going_down()

  因此正常 reboot/shutdown 阶段不会 panic。

  4. 在你的 device_del 测试中仍存在竞态窗口

  非 surprise removal 会在 ext4_shutdown() 前先调用 sync_filesystem()。如果这个窗口内：

  - journal/元数据 I/O 失败；
  - ext4 或并发线程把该错误送进 ext4_handle_error()；

  那么 errors=panic 可以触发。

  但你当前失败的是普通数据页；随后 shutdown 抢先置位，因此没触发。data_err=abort 可以增加 journal 被 abort 的概率，但仍不能保证在 shutdown 前由 ext4 消费
  这个错误，所以也不是确定方案。

  因此最终判断是：

  - 验证 errors=panic 本身：使用保持 gendisk 存活的元数据/journal I/O 错误注入，可以稳定触发。
  - 验证 device_del：可能 panic，但属于竞态，不能依赖。
  - 如果产品要求“磁盘消失必定导致宿主机 panic”，仅靠 mount option 不够，需要在 ext4_shutdown() 或设备移除策略中显式实现。

  现有文档 /home/martins3/data/vn/docs/kernel/fs/ext4/error.md:1 中“shutdown 后文件系统变成只读”的说法不准确；应改成“进入 shutdown 状态，访问通常返回
  -EIO，不等同于 readonly/-EROFS”。

## 尝试复现一下 ro 的结果

的确是可以的，但是为什么是 emergency_ro
/dev/sda /mnt ext4 rw,relatime,stripe=512,emergency_ro 0 0

原来如此:
```txt
History:        #0
Commit:         6b76715d5e41fc332b0b879e66fad6ef3db07a3f
Author:         Baokun Li <libaokun1@huawei.com>
Committer:      Theodore Ts'o <tytso@mit.edu>
Author Date:    Wed 22 Jan 2025 07:41:29 PM CST
Committer Date: Thu 13 Mar 2025 10:16:34 PM CST

ext4: show 'emergency_ro' when EXT4_FLAGS_EMERGENCY_RO is set

After commit d3476f3dad4a ("ext4: don't set SB_RDONLY after filesystem
errors") in v6.12-rc1, the 'errors=remount-ro' mode no longer sets
SB_RDONLY on errors, which results in us seeing the filesystem is still
in rw state after errors.

Therefore, after setting EXT4_FLAGS_EMERGENCY_RO, display the emergency_ro
option so that users can query whether the current file system has become
emergency read-only due to errors through commands such as 'mount' or
'cat /proc/fs/ext4/sdx/options'.

Fixes: d3476f3dad4a ("ext4: don't set SB_RDONLY after filesystem errors")
Signed-off-by: Baokun Li <libaokun1@huawei.com>
Reviewed-by: Jan Kara <jack@suse.cz>
Reviewed-by: Zhang Yi <yi.zhang@huawei.com>
Link: https://patch.msgid.link/20250122114130.229709-7-libaokun@huaweicloud.com
Signed-off-by: Theodore Ts'o <tytso@mit.edu>
```

## 需要尝试一下这个操作
trigger_fs_error

## 一个错误，文件系统就会只读吗?
不一定。ext4_handle_error() 不是块设备 I/O 错误的统一回调，只有 ext4 将该错误认定为“文件系统级错误”并调用 ext4_error*() / ext4_std_error() 等接口时，才会进入它。

典型区别：

- 普通文件数据读失败：返回 -EIO、标记 folio 读取失败，一般不调用 ext4_handle_error()。
- 普通文件数据写失败：记录到 mapping->wb_err，后续 fsync() 等返回 -EIO；这里只打印 warning，通常不触发文件系统只读。见 fs/ext4/page-io.c:366。
- inode bitmap、block bitmap、目录块等元数据读取失败：通常调用 ext4_error_err(..., EIO, ...)，进而进入 ext4_handle_error()。
- journal I/O 失败：JBD2 会 abort journal，通常作为严重文件系统错误处理，并强制进入只读状态。
- 异步元数据写回错误：ext4 通过块设备 wb_err 检测后调用 ext4_error_err()。见 fs/ext4/ext4_jbd2.c:203。

调用关系大致是：

磁盘 I/O 错误
  ├─ 普通文件数据 I/O → 返回 EIO / mapping_set_error
  └─ 元数据或 journal I/O
       → ext4_error_err / ext4_std_error / ext4_abort
       → ext4_handle_error

进入 fs/ext4/super.c:ext4_handle_error 后，行为取决于挂载选项：

- errors=continue：记录错误后继续运行。
- errors=remount-ro：abort journal，并设置 emergency read-only。
- errors=panic：触发 panic。
- journal 等不可恢复错误带 force_ro=true 时，即使配置 errors=continue 也会强制只读。

所以关键不是“错误是否来自 disk”，而是“失败的是普通用户数据，还是影响 ext4 一致性的元数据/journal”。

### 这个可以做一个实验，但是如何知道是一个问题

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
