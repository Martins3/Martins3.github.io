# lockdep usage
## 配套测试代码

打开的方法 : CONFIG_PROVE_LOCKING

## /proc

和 lockdep 相关的
- lockdep
- lockdep_chains
- lockdep_stats


对应的实现的代码是 : kernel/locking/lockdep_proc.c

lock_stat
locks

## kernel hacking -> Lock Debugging (spinlocks, mutexes, etc...)

```txt
[ ] Lock debugging: prove locking correctness (CONFIG_PROVE_LOCKING 就是大名鼎鼎的 lockdep)
[ ] Lock usage statistics
[ ] RT Mutex debugging, deadlock detection
[ ] Spinlock and rw-lock debugging: basic checks
[ ] Mutex debugging: basic checks
[ ] Wait/wound mutex debugging: Slowpath testing
[ ] RW Semaphore debugging: basic checks
[ ] Lock debugging: detect incorrect freeing of live locks
[ ] Sleep inside atomic section checking
[ ] Locking API boot-time self-tests
< > torture tests for locking
< > Wait/wound mutex selftests
< > torture tests for smp_call_function*()
[ ] Debugging for csd_lock_wait(), called from smp_call_function*()
```

CONFIG_LOCK_TORTURE_TEST : 这个没办法测试 irq 吧，而且 bh 也没有测试
CONFIG_SCF_TORTURE_TEST : 测试 smp_call_function
原来 torture 测试框架在这里: kernel/torture.c

## concurrent/lockdep.c

**这都是 CONFIG_PROVE_LOCKING=y 打开的结果**
看看不打开的结果

### case 0 和 case 1

case 0 : 展示在任何一个位置持有的锁是什么
```txt
[40878.237345]  #0: ffff8881015c2420 (sb_writers#4){.+.+}-{0:0}, at: ksys_write+0x71/0xf0
[40878.238490]  #1: ffff888105faa690 (&of->mutex#2){+.+.}-{4:4}, at: kernfs_fop_write_iter+0x107/0x230
[40878.239799]  #2: ffff88813e562738 (kn->active#129){.+.+}-{0:0}, at: kernfs_fop_write_iter+0x110/0x230
[40878.241119]  #3: ffffffff8255abc0 (rcu_read_lock){....}-{1:3}, at: tes
```

第一个锁 : vfs_write -> file_start_write
```txt
	percpu_down_read(sb->s_writers.rw_sem + level - 1);
```

第二个和第三个锁
```txt
	mutex_lock(&of->mutex); // 注意，这个是文件的锁，不是 inode 的锁
	if (!kernfs_get_active(of->kn)) { // 这个是 lockdep 自己引入的
```
也就是，一共两个锁，一个保障 fs 不被 umount

根据这个观察，我们在 concurrent/lockdep.cpp 可以验证的问题
```txt
 sudo cat /proc/17671/stack
[<0>] test_lockdep+0x81/0x150 [martins3]
[<0>] lockdep_store.cold+0x34/0xcb [martins3]
[<0>] kernfs_fop_write_iter+0x14e/0x230
[<0>] vfs_write+0x2bc/0x570
[<0>] ksys_write+0x71/0xf0
[<0>] do_syscall_64+0x72/0x180
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
~ 🐈
🧀  sudo cat /proc/17672/stack
[<0>] fdget_pos+0x7d/0xb0
[<0>] ksys_write+0x2b/0xf0
[<0>] do_syscall_64+0x72/0x180
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

### case 5 触发经典死锁
```txt
[   21.265316] mutex_store : action = 5 current=bash now=21273248041
[   21.265857]
[   21.265976] ============================================
[   21.266355] WARNING: possible recursive locking detected
[   21.266711] 6.9.1 #3 Tainted: G           O
[   21.267002] --------------------------------------------
[   21.267320] bash/1571 is trying to acquire lock:
[   21.267591] ffffffffc000c8d8 (test_mutex_hi){+.+.}-{4:4}, at: test_mutex+0x58/0x140 [martins3]
[   21.268070]
[   21.268070] but task is already holding lock:
[   21.268378] ffffffffc000c8d8 (test_mutex_hi){+.+.}-{4:4}, at: test_mutex+0x4a/0x140 [martins3]
[   21.268801]
[   21.268801] other info that might help us debug this:
[   21.269103]  Possible unsafe locking scenario:
[   21.269103]
[   21.269377]        CPU0
[   21.269489]        ----
[   21.269593]   lock(test_mutex_hi);
[   21.269740]   lock(test_mutex_hi);
[   21.269882]
[   21.269882]  *** DEADLOCK ***
[   21.269882]
[   21.270109]  May be due to missing lock nesting notation
[   21.270109]
[   21.270368] 4 locks held by bash/1571:
[   21.270515]  #0: ffff888119815428 (sb_writers#3){.+.+}-{0:0}, at: vfs_write+0xe1/0x390
[   21.270809]  #1: ffff888106f4fc90 (&of->mutex#2){+.+.}-{4:4}, at: kernfs_fop_write_iter+0xd0/0x1b0
[   21.271120]  #2: ffff888122ceead0 (kn->active#144){.+.+}-{0:0}, at: kernfs_fop_write_iter+0xd8/0x1b0
[   21.271434]  #3: ffffffffc000c8d8 (test_mutex_hi){+.+.}-{4:4}, at: test_mutex+0x4a/0x140 [martins3]
[   21.271751]
[   21.271751] stack backtrace:
[   21.271904] CPU: 3 PID: 1571 Comm: bash Tainted: G           O       6.9.1 #3
[   21.272152] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[   21.272382] Call Trace:
[   21.272469]  <TASK>
[   21.272546]  dump_stack_lvl+0xa7/0x100
[   21.272681]  validate_chain+0x909/0x2260
[   21.272820]  __lock_acquire+0x963/0xbd0
[   21.272956]  ? test_mutex+0x58/0x140 [martins3]
[   21.273118]  lock_acquire+0xf2/0x290
[   21.273245]  ? test_mutex+0x58/0x140 [martins3]
[   21.273406]  ? test_mutex+0x58/0x140 [martins3]
[   21.273569]  __mutex_lock+0x81/0x860
[   21.273698]  ? test_mutex+0x58/0x140 [martins3]
[   21.273858]  ? _printk+0x5d/0x80
[   21.273974]  test_mutex+0x58/0x140 [martins3]
[   21.274129]  mutex_store+0xa0/0xd0 [martins3]
[   21.274285]  kernfs_fop_write_iter+0x122/0x1b0
[   21.274442]  vfs_write+0x31d/0x390
[   21.274563]  ksys_write+0x75/0xe0
[   21.274683]  do_syscall_64+0xf8/0x210
[   21.274811]  ? exc_page_fault+0xb2/0x280
[   21.274949]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
[   21.275128] RIP: 0033:0x7fa2a6cdfd24
[   21.275254] Code: 15 11 91 0d 00 f7 d8 64 89 02 48 c7 c0 ff ff ff ff eb b7 0f 1f 00 f3 0f 1e fa 80 3d fd 16 0e 00 00 74 13 b8 01 00 00 00 0f 05 <48> 3d 00 f0 ff ff 77 54 c3 0f 1f 00 48 83 ec 28 48 89 54 24 18 48
[   21.275893] RSP: 002b:00007ffcce9c4658 EFLAGS: 00000202 ORIG_RAX: 0000000000000001
[   21.276153] RAX: ffffffffffffffda RBX: 0000000000000002 RCX: 00007fa2a6cdfd24
[   21.276398] RDX: 0000000000000002 RSI: 000055969f29b3e0 RDI: 0000000000000001
[   21.276643] RBP: 000055969f29b3e0 R08: 000055969f29b3e0 R09: 0000000000000000
[   21.276890] R10: 0000000000001000 R11: 0000000000000202 R12: 0000000000000002
[   21.277135] R13: 00007fa2a6dba5a0 R14: 0000000000000002 R15: 00007fa2a6db5880
[   21.277380]  </TASK>
```


### case 6 和 case 7

测试 mutex nested lock
https://lore.kernel.org/lkml/1516699646-7321-1-git-send-email-jasowang@redhat.com/
```txt
[   29.504282] ================================================
[   29.505147] WARNING: lock held when returning to user space!
[   29.505863] 6.15.4 #10 Tainted: G           O
[   29.506454] ------------------------------------------------
[   29.507168] tee/2267 is leaving the kernel with locks still held!
[   29.507946] 2 locks held by tee/2267:
[   29.508337]  #0: ffffffffc0ea8370 (test_mutex_hi){+.+.}-{4:4}, at: test_lockdep+0x58/0x130 [martins3]
[   29.509605]  #1: ffffffffc0ea82d0 (test_mutex_hi2){+.+.}-{4:4}, at: test_lockdep+0x66/0x130 [martins3]
```

这种可以被检测到:
```txt
	mutex_lock(&test_mutex_hi);
	mutex_lock(&test_mutex_hi2);
```

这种不会检测到
```txt
	mutex_lock_nested(&test_mutex_hi, 0);
	mutex_lock_nested(&test_mutex_hi, 1);
```

也就是并不会检测，如果系统调用结束之后，所有的锁都是释放的:
这是一个很简单的需求啊，不知道为什么没有做
```txt
	mutex_lock(&test_mutex_hi);
```

### case 10
```txt
[   83.437982] =============================
[   83.437983] WARNING: suspicious RCU usage
[   83.437984] 6.9.1 #3 Tainted: G           O
[   83.437985] [rcu_basic_writer] start
[   83.437985] -----------------------------
[   83.437987] /home/martins3/core/vn/code/module/rcupdate.c:58 suspicious rcu_dereference_check() usage!
[   83.437992]
[   83.437992] other info that might help us debug this:
[   83.437992]
[   83.437993]
[   83.437993] rcu_scheduler_active = 2, debug_locks = 1
[   83.437995] 2 locks held by kworker/u256:6/64:
[   83.437997]  #0: ffff888104932958 ((wq_completion)rcu_test){+.+.}-{0:0}, at: process_scheduled_works+0x23a/0x640
[   83.438005]  #1: ffffc90000cebe48 ((work_completion)(&rcu_basic2.work)){+.+.}-{0:0}, at: process_scheduled_works+0x25a/0x640
[   83.438009]
[   83.438009] stack backtrace:
[   83.438010] CPU: 0 PID: 64 Comm: kworker/u256:6 Tainted: G           O       6.9.1 #3
[   83.438012] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[   83.438012] Workqueue: rcu_test rcu_basic_reader [martins3]
[   83.438018] Call Trace:
[   83.438019]  <TASK>
[   83.438021]  dump_stack_lvl+0xa7/0x100
[   83.438029]  lockdep_rcu_suspicious+0x14f/0x190
[   83.438034]  rcu_basic_reader+0x1ce/0x200 [martins3]
[   83.438039]  ? process_scheduled_works+0x25a/0x640
[   83.438040]  process_scheduled_works+0x2b0/0x640
[   83.438046]  worker_thread+0x270/0x370
[   83.438047]  ? __pfx_worker_thread+0x10/0x10
[   83.438049]  kthread+0x109/0x130
[   83.438051]  ? __pfx_kthread+0x10/0x10
[   83.438052]  ret_from_fork+0x37/0x50
[   83.438053]  ? __pfx_kthread+0x10/0x10
[   83.438055]  ret_from_fork_asm+0x1a/0x30
[   83.438060]  </TASK>
```

## CONFIG_DEBUG_ATOMIC_SLEEP

如果不打开 lockdep 不会有这个东西吧。
```txt
[  977.663859] preempt_store : action = 0 current=bash now=977704749634
[  977.664875] BUG: sleeping function called from invalid context at /home/martins3/core/vn/code/module/preempt.c:27
[  977.665595] in_atomic(): 1, irqs_disabled(): 1, non_block: 0, pid: 1647, name: bash
[  977.665857] preempt_count: 1, expected: 0
[  977.665997] RCU nest depth: 0, expected: 0
[  977.666141] 4 locks held by bash/1647:
[  977.666274]  #0: ffff88810853e428 (sb_writers#3){.+.+}-{0:0}, at: vfs_write+0xe1/0x390
[  977.666558]  #1: ffff888106fc3490 (&of->mutex#2){+.+.}-{4:4}, at: kernfs_fop_write_iter+0xd0/0x1b0
[  977.666871]  #2: ffff88817e5062e8 (kn->active#146){.+.+}-{0:0}, at: kernfs_fop_write_iter+0xd8/0x1b0
[  977.667188]  #3: ffffffffc000d3d8 (sl_static){....}-{3:3}, at: test_preempt+0x174/0x300 [martins3]
[  977.667500] irq event stamp: 11254
[  977.667620] hardirqs last  enabled at (11253): [<ffffffff8123a2b0>] console_unlock+0xd0/0x190
[  977.667911] hardirqs last disabled at (11254): [<ffffffff8269d693>] _raw_spin_lock_irqsave+0x43/0xd0
[  977.668424] softirqs last  enabled at (10050): [<ffffffff8118072a>] handle_softirqs+0x36a/0x3d0
[  977.668899] softirqs last disabled at (10043): [<ffffffff81180921>] __irq_exit_rcu+0x71/0x100
[  977.669323] CPU: 1 PID: 1647 Comm: bash Tainted: G        W  O       6.9.1 #3
[  977.669670] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[  977.669981] Call Trace:
[  977.670098]  <TASK>
[  977.670199]  dump_stack_lvl+0xa7/0x100
[  977.670369]  __might_resched+0x24f/0x270
[  977.670546]  test_preempt+0x188/0x300 [martins3]
[  977.670753]  preempt_store+0xa0/0xd0 [martins3]
[  977.670957]  kernfs_fop_write_iter+0x122/0x1b0
[  977.671139]  vfs_write+0x31d/0x390
[  977.671284]  ksys_write+0x75/0xe0
[  977.671426]  do_syscall_64+0xf8/0x210
[  977.671579]  ? exc_page_fault+0xb2/0x280
[  977.671743]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
[  977.671939] RIP: 0033:0x7f1d62dadd24
[  977.672073] Code: 15 11 91 0d 00 f7 d8 64 89 02 48 c7 c0 ff ff ff ff eb b7 0f 1f 00 f3 0f 1e fa 80 3d fd 16 0e 00 00 74 13 b8 01 00 00 00 0f 05 <48> 3d 00 f0 ff ff 77 54 c3 0f 1f 00 48 83 ec 28 48 89 54 24 18 48
[  977.672719] RSP: 002b:00007fff94c68ae8 EFLAGS: 00000202 ORIG_RAX: 0000000000000001
[  977.672986] RAX: ffffffffffffffda RBX: 0000000000000002 RCX: 00007f1d62dadd24
[  977.673233] RDX: 0000000000000002 RSI: 0000563d24832410 RDI: 0000000000000001
[  977.673481] RBP: 0000563d24832410 R08: 0000563d24832410 R09: 0000000000000020
[  977.673727] R10: 00007f1d62e87b40 R11: 0000000000000202 R12: 0000000000000002
[  977.673973] R13: 00007f1d62e885a0 R14: 0000000000000002 R15: 00007f1d62e83880
[  977.674224]  </TASK>
[  977.674327] preempt_store : now=977715218039
```
1. 展示持有了那些 lock
2. softirq 和 hardirq 的 disable 的时间


## TODO

这种是靠什么实现的，这个不是比 lockdep_assert_held 要好?
还是他会自动获取的意思?
```txt
static void process_one_work(struct worker *worker, struct work_struct *work)
__releases(&pool->lock)
__acquires(&pool->lock)
```

workqueue 的 process_one_work 中，需要 work 上锁之后，需要停掉 lock

lockdep_depth(current) 是做什么的

这段做什么的?
```txt
#ifdef CONFIG_LOCKDEP
	/*
	 * It is permissible to free the struct work_struct from
	 * inside the function that is called from it, this we need to
	 * take into account for lockdep too.  To avoid bogus "held
	 * lock freed" warnings as well as problems when looking into
	 * work->lockdep_map, make a copy and use that here.
	 */
	struct lockdep_map lockdep_map;

	lockdep_copy_map(&lockdep_map, &work->lockdep_map);
#endif
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
