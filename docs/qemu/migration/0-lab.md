# migration 基本测试

## 有趣的 remote-snapshotter
firecracker/examples/cmd/remote-snapshotter/README.md

## 这个错误是什么意思?
```txt
qemu-system-aarch64: Unknown savevm section or instance '0000:00:0b.0/pcie-root-port' 0. Make sure
that your current VM setup matches your saved VM setup,
including any hotplugged devices
```

## switchover
```txt
(qemu) info migrate
globals:
store-global-state: on
only-migratable: off
send-configuration: on
send-section-footer: on
send-switchover-start: on
clear-bitmap-shift: 18
```

switchover-ack 依赖 return-path :
```txt
migrate_set_capability  return-path on
migrate_set_capability  switchover-ack on
```

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - migration_completion
            - migration_completion_precopy
              - migration_switchover_start

send-switchover-start 在 target 端是执行
loadvm_postcopy_handle_switchover_start
这是是一个最近刚刚加入的东西:


```diff
History:        #0
Commit:         4e55cb3cdeb099cb65f75f5d3b061e3e1319cf3b
Author:         Maciej S. Szmigiero <maciej.szmigiero@oracle.com>
Committer:      Cédric Le Goater <clg@redhat.com>
Author Date:    Wed 05 Mar 2025 06:03:32 AM CST
Committer Date: Thu 06 Mar 2025 01:47:33 PM CST

migration: Add MIG_CMD_SWITCHOVER_START and its load handler

This QEMU_VM_COMMAND sub-command and its switchover_start SaveVMHandler is
used to mark the switchover point in main migration stream.

It can be used to inform the destination that all pre-switchover main
migration stream data has been sent/received so it can start to process
post-switchover data that it might have received via other migration
channels like the multifd ones.

Add also the relevant MigrationState bit stream compatibility property and
its hw_compat entry.

Reviewed-by: Fabiano Rosas <farosas@suse.de>
Reviewed-by: Zhang Chen <zhangckid@gmail.com> # for the COLO part
Signed-off-by: Maciej S. Szmigiero <maciej.szmigiero@oracle.com>
Link: https://lore.kernel.org/qemu-devel/311be6da85fc7e49a7598684d80aa631778dcbce.1741124640.git.maciej.szmigiero@oracle.com
Signed-off-by: Cédric Le Goater <clg@redhat.com>
```

## 这些命令都熟悉一下

似乎都是 postcopy 相关的
```c
enum qemu_vm_cmd {
    MIG_CMD_INVALID = 0,   /* Must be 0 */
    MIG_CMD_OPEN_RETURN_PATH,  /* Tell the dest to open the Return path */
    MIG_CMD_PING,              /* Request a PONG on the RP */

    MIG_CMD_POSTCOPY_ADVISE,       /* Prior to any page transfers, just
                                      warn we might want to do PC */
    MIG_CMD_POSTCOPY_LISTEN,       /* Start listening for incoming
                                      pages as it's running. */
    MIG_CMD_POSTCOPY_RUN,          /* Start execution */

    MIG_CMD_POSTCOPY_RAM_DISCARD,  /* A list of pages to discard that
                                      were previously sent during
                                      precopy but are dirty. */
    MIG_CMD_PACKAGED,          /* Send a wrapped stream within this stream */
    MIG_CMD_ENABLE_COLO,       /* Enable COLO */
    MIG_CMD_POSTCOPY_RESUME,   /* resume postcopy on dest */
    MIG_CMD_RECV_BITMAP,       /* Request for recved bitmap on dst */
    MIG_CMD_SWITCHOVER_START,  /* Switchover start notification */
    MIG_CMD_MAX
};
```

## 测试一下 postcopy 的行为

## 这里的东西都该测试一下
docs/devel/migration/

## 测试下如果两个设备不兼容，会在那里报错
强行迁移

## 为什么本地的迁移速度这么慢啊?
只有 1G 的速


## 一旦热迁移搭建成功后

1. 测试 virtio 设备热迁移的时候，inflight io 如何处理
2. vhost-net 和 virtio-blk 的热迁移的时候差别
4. 测试一下，如果是 ping 10.0 的网段的地址，看看效果
  - 不过，这个是如何实现的，


## 各种观测的工具

### 基本操作
```txt
migrate                 migrate_cancel          migrate_continue
migrate_incoming        migrate_pause           migrate_recover
migrate_set_capability  migrate_set_parameter   migrate_start_postcopy
```
原来这个还不可以使用啊？
```txt
(qemu) migrate_pause
Error: migrate-pause is currently only supported during postcopy-active or postcopy-recover state
```

确定一个问题，热迁移的时候，内核中只有一个东西:

### calc_dirty_rate

```txt
(qemu) help calc_dirty_rate
calc_dirty_rate [-r] [-b] second [sample_pages_per_GB] -- start a round of guest dirty rate measurement (using -r to
                         specify dirty ring as the method of calculation and
                         -b to specify dirty bitmap as method of calculation)
```
migration/dirtyrate.c

### info dirty_rate

### info migrate
在热迁移的过程中:
```txt
(qemu) info migrate -a
Status:                 active
Time (ms):              total=6092, setup=9, exp_down=300
RAM info:
  Throughput (Mbps):    1169.84
  Sizes:                pagesize=4 KiB, total=8.13 GiB
  Transfers:            transferred=459 MiB, remain=278 MiB
    Channels:           precopy=744 B, multifd=459 MiB, postcopy=0 B
    Page Types:         normal=112561, zero=1946319
  Page Rates (pps):     transfer=66560
  Others:               dirty_syncs=1
Globals:
  store-global-state: on
  only-migratable: on
  send-configuration: on
  send-section-footer: on
  send-switchover-start: on
  clear-bitmap-shift: 18
```


在热迁移之后:
```txt
info migrate -a
Status:                 completed
Time (ms):              total=6728, setup=8, down=32
RAM info:
  Throughput (Mbps):    649.36
  Sizes:                pagesize=4 KiB, total=8.13 GiB
  Transfers:            transferred=520 MiB, remain=0 B
    Channels:           precopy=808 B, multifd=520 MiB, postcopy=0 B
    Page Types:         normal=127568, zero=2003477
  Page Rates (pps):     transfer=40960
  Others:               dirty_syncs=3
Globals:
  store-global-state: on
  only-migratable: on
  send-configuration: on
  send-section-footer: on
  send-switchover-start: on
  clear-bitmap-shift: 18
```
现在source 以及结束了，然

- https://developers.redhat.com/blog/2015/03/24/live-migrating-qemu-kvm-virtual-machines#
- https://www.qemu.org/docs/master/devel/migration.html

### info migrate_parameters

```txt
(qemu) info migrate_parameters
announce-initial: 50 ms
announce-max: 550 ms
announce-rounds: 5
announce-step: 100 ms
throttle-trigger-threshold: 50
cpu-throttle-initial: 20
cpu-throttle-increment: 10
cpu-throttle-tailslow: off
max-cpu-throttle: 99
tls-creds: ''
tls-hostname: ''
max-bandwidth: 134217728 bytes/second
avail-switchover-bandwidth: 0 bytes/second
downtime-limit: 300 ms
x-checkpoint-delay: 20000 ms
multifd-channels: 2
multifd-compression: none
zero-page-detection: multifd
xbzrle-cache-size: 67108864 bytes
max-postcopy-bandwidth: 0
tls-authz: ''
x-vcpu-dirty-limit-period: 1000 ms
vcpu-dirty-limit: 1 MB/s
mode: normal
direct-io: off
```

## 似乎操作 vhost user 是有问题的
- loadvm_postcopy_handle_advise 中触发了这个错误：
```txt
qemu-system-x86_64: RAM postcopy is disabled but have 16 byte advise
qemu-system-x86_64: load of migration failed: Invalid argument
```
有趣，这个不就是说过的，无法热迁移的问题吗?

## 在一个机器上操作的时候

- 观测 host swap 的处理
- 观测 qcow2 的处理
  - qcow2 的文件锁的转移
- host swap 似乎会让热升级很难做人的哇

## 分析其他 Hypervisor 上是如何进行热迁移的
- https://github.com/cloud-hypervisor/cloud-hypervisor

## 热迁移的时候，如果 guest 当时在 perf，pmu 的状态可以维护吗？


## Linux 中支持 CONFIG_CHECKPOINT_RESTORE 来实现将 process 的状态保存，进而来实现容器的迁移

## 热迁移的时候，中断都是需要保持的
kvm_vcpu_ioctl_x86_get_vcpu_events

kvm 中也需要保存 timer 的

## 问题

- 热迁移的时候，osv 或者 bridge 是如何识别的?
  - 不需要，这个是 ovs / bridge 的工作

- 看看 qemu_add_vm_change_state_handler 的调用者

```txt
static void kvmclock_vm_state_change(void *opaque, bool running,
                                     RunState state)
```
running 是将要变为的状态

## 测试一下 CPU throttle 的东西

## 如果 qemu savevm 的时候，里面还有嵌套虚拟机

可以保存吗? 如果恢复，l1 和 l2 虚拟机可以继续运行吗?

## 保存到文件才可以恢复，太难了

migrate "exec:cat > vmstate.bin"
qemu-system-x86_64 -incoming "exec:cat vmstate.bin" ...

```txt
virsh save <domain> <state-file>
virsh restore <state-file>
```
就是读写文件，实在是有点简陋了:

```c
/* Helper function called while vm is active.  */
int
qemuMigrationSrcToFile(virQEMUDriver *driver, virDomainObj *vm,
                       const char *path,
                       int *fd,
                       virCommand *compressor,
                       qemuMigrationParams *migParams,
                       unsigned int flags,
                       virDomainAsyncJob asyncJob)
{
	// ...
    if (migParams &&
        qemuMigrationParamsCapEnabled(migParams, QEMU_MIGRATION_CAP_MAPPED_RAM))
        rc = qemuMigrationSrcToSparseFile(driver, vm, path, fd, flags, asyncJob);
    else
        rc = qemuMigrationSrcToLegacyFile(driver, vm, *fd, compressor, asyncJob);
```

## 尝试把这个项目集成过来
https://github.com/abbbi/qmpbackup

## qemu 热迁移 background-snapshot 的语义
<!-- 7293bd0e-75e3-4705-9e8d-ea707f54cda8 -->

当时的 patch :
https://lists.nongnu.org/archive/html/qemu-devel/2021-01/msg05482.html

关键区别在于：background-snapshot 打开的已经不是“普通 live migration 完成交接”的语义，而是“生成一个开始时刻的一致性快照”的语义。
先看能力定义，QEMU 自己就写得很直白：

```txt
  # @background-snapshot: If enabled, the migration stream will be a
  #     snapshot of the VM exactly at the point when the migration
  #     procedure starts.  The VM RAM is saved with running VM.
```


  1. 打开 `background-snapshot` 时，走后台快照线程

  在 migration_start_outgoing() 里，QEMU 会直接分叉两条路径：

   migration/migration.c lines 3824-3830

```txt
  if (migrate_background_snapshot()) {
      qemu_thread_create(&s->thread, MIGRATION_THREAD_SNAPSHOT,
              bg_migration_thread, s, QEMU_THREAD_JOINABLE);
  } else {
      qemu_thread_create(&s->thread, MIGRATION_THREAD_SRC_MAIN,
              migration_thread, s, QEMU_THREAD_JOINABLE);
  }
```

bg_migration_thread() 的做法是：
1. 先短暂停一下 VM
2. 把“非 RAM 状态”先暂存到内存 buffer
3. 开启 RAM 写保护跟踪
4. 立刻把 VM 恢复运行
5. 在 VM 继续跑的同时，把 RAM 快照写到迁移流里
6. 最后再把之前缓存的设备状态补到流尾

  核心代码就在这里：

migration/migration.c lines 3680-3699

```txt
  bql_lock();
  if (migration_stop_vm(s, RUN_STATE_PAUSED)) {
      error_setg(&local_err, "Failed to stop the VM");
      goto fail_with_bql;
  }
  if (qemu_savevm_state_non_iterable(fb, &local_err)) {
      ...
  }
  qemu_savevm_state_end_precopy(s, fb);
  if (ram_write_tracking_start()) {
      ...
  }
  /* Start VM from BH handler ... */
  migration_bh_schedule(bg_migration_vm_start_bh, s);
  bql_unlock();
```

  而这个 BH 里就是直接恢复源 VM：

migration/migration.c lines 3599-3605

```c
  static void bg_migration_vm_start_bh(void *opaque)
  {
      MigrationState *s = opaque;
      vm_resume(s->vm_old_state);
      migration_downtime_end(s);
  }
```

所以现象上你会看到：
- 它不是完全不停，而是开始时会有一个短暂停顿
- 但很快就 vm_resume() 了
- 后面生成 mig 文件时，原 VM 还能继续运行



  2. 关闭 `background-snapshot` 时，走普通 migration 线程

普通路径走 migration_thread()。这条路径的语义是“把 VM 迁移完成并交给目标端”，
因此到了 completion 阶段，必须做一次 switchover，也就是把源 VM 停住，冻结最终状态，再把最后那部分状态写完。
普通 completion 里明确先停 VM：

   migration/migration.c lines 2747-2761

  bql_lock();
  if (!migrate_mode_is_cpr()) {
      ret = migration_stop_vm(s, RUN_STATE_FINISH_MIGRATE);
      if (ret < 0) {
          goto out_unlock;
      }
  }
  if (!migration_switchover_start(s, NULL)) {
      ret = -EFAULT;
      goto out_unlock;
  }
  ret = qemu_savevm_state_complete_precopy(s);

  这里的 migration_stop_vm() 会把旧状态保存下来并把 VM 切到 RUN_STATE_FINISH_MIGRATE：

   migration/migration.c lines 278-285

  static int migration_stop_vm(MigrationState *s, RunState state)
  {
      migration_downtime_start(s);
      s->vm_old_state = runstate_get();
      global_state_store();
      ret = vm_stop_force_state(state);

  迁移成功后，普通路径会进入 MIGRATION_STATUS_COMPLETED，随后源端 runstate 会被设成 POSTMIGRATE：

   migration/migration.c lines 3298-3301

  switch (s->state) {
  case MIGRATION_STATUS_COMPLETED:
      runstate_set(RUN_STATE_POSTMIGRATE);
      break;

  这就是为什么你看到：
  • background-snapshot off
  • migrate "exec:cat > mig"

  之后，源 VM 不会继续跑。
  因为 QEMU 并不知道你是“只是想导出个文件”；在它看来，这仍然是一次正常迁移，语义上应该是“源端停机，目标端接管”。exec:cat > mig
  只是传输通道是一个 pipe/file，不会改变 migration 的完成语义。

* 兼容性限制: background-snapshot 与其他一些迁移功能不兼容，例如：
    * postcopy-ram
    * dirty-bitmaps
    * postcopy-blocktime
    * late-block-activate
    * return-path
    * multifd
    * pause-before-switchover
    * auto-converge
    * release-ram
    * rdma-pin-all
    * xbzrle
    * x-colo
    * validate-uuid
    * zero-copy-send
    * （以及 CPR 模式如 cpr-reboot, cpr-transfer, cpr-exec）
* 用途: 这个功能特别适用于需要在不影响客户机运行的情况下获取其某一时刻精确状态的场景，例如在线备份或调试。

## 两个长久没有完成的测试

```txt
	# @todo 测试发现，目标端是否为 smm 不影响迁移
	arg_machine="-machine pc,accel=kvm,kernel-irqchip=on,smm=on"
	# target 端是啥 cpu model 根本没影响啊
	arg_cpu_model="-cpu Broadwell-IBRS"

	# 需要保证迁移的两侧的 romfile 内容一致才可以
	# arg_network="$arg_network,romfile=/home/martins3/hack/vm/img1"
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
