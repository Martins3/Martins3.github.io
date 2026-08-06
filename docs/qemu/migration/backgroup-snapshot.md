# background-snapshot
<!-- 7293bd0e-75e3-4705-9e8d-ea707f54cda8 -->

当时的 patch :
https://lists.nongnu.org/archive/html/qemu-devel/2021-01/msg05482.html

bg_migration_thread 中
	- qemu_savevm_state_header
	- migration_stop_vm
	- qemu_savevm_state_do_setup ( state 保存)
	- bg_migration_iteration_run ( 保存 ram)

```c
/*
 * Return true if continue to the next iteration directly, false
 * otherwise.
 */
static MigIterateState bg_migration_iteration_run(MigrationState *s)
{
    int res;

    res = qemu_savevm_state_iterate(s->to_dst_file, false);
    if (res > 0) {
        bg_migration_completion(s);
        return MIG_ITERATE_BREAK;
    }

    return MIG_ITERATE_RESUME;
}
```

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
