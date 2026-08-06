# QEMU 热迁移基础

## qemu 存在哪些状态控制

1. hmp : stop / cont
2. snapshot

snapshot_blkdev
snapshot_blkdev_internal
snapshot_delete_blkdev_internal

3. savevm / loadvm

4. migration
	- cpr 优化
	- file / socket


  严格来说，接近“操作可恢复整机状态”的还有：

  - background-snapshot migration：生成某一时刻的 RAM/设备 migration stream，同时尽量让 VM 继续运行；不自动包含磁盘快照。
  - COLO：周期性停止并发送 VM checkpoint，同时配合磁盘复制和网络输出比较；复用了 vm_stop_force_state() 和 savevm device state，migration/colo.c:407。
  - Record/replay checkpoint：VM snapshot 加事件日志，用于回放和回退，适用范围较窄。
  - Xen xen-save-devices-state：QEMU 只保存设备状态，RAM 由 Xen toolstack 保存，是拆分式整机状态管理。

  下面这些看起来类似，但不是完整可恢复状态：

  - dump-guest-memory：只有 RAM/崩溃分析信息。
  - block snapshot/backup：只有磁盘。
  - stop/cont、ACPI S3：只冻结/恢复，不序列化。
  - guest S4/hibernate：状态由 guest OS 写入虚拟磁盘，不保存 QEMU 设备模型状态。
  - system_reset：整体重置状态，不保留旧状态。


最核心的判断标准是：

整机状态 = runstate + CPU + RAM + device state
+ disk contents + external backend/kernel state
+ machine configuration

QEMU 的 migration/savevm 核心主要解决前四项；后三项分别由内部磁盘快照、共享存储/管理层、CPR FD 保留以及重新创建兼容命令行来补齐。

其实也没有这么复杂:
1. device state
2. RAM
3. disk state

1. snapshot / savevm : 所有内容，暂停
2. migration : 先 ram ，最后 device state
3. migration + background : 立刻保存
4. cpr : 仅仅传输 device state ，跳过 ram + disk

### snapshot

2. 只是不想保存“虚拟机数据盘”：可以使用专用 VMState 节点

通过 QMP snapshot-save/snapshot-load，准备一个没有挂给 guest 的 qcow2 节点，例如 vmstate0，只对这个节点做快照：

{
  "execute": "snapshot-save",
  "arguments": {
    "job-id": "save-s1",
    "tag": "s1",
    "vmstate": "vmstate0",
    "devices": ["vmstate0"]
  }
}

恢复：

{
  "execute": "snapshot-load",
  "arguments": {
    "job-id": "load-s1",
    "tag": "s1",
    "vmstate": "vmstate0",
    "devices": ["vmstate0"]
  }
}

这样 guest 的系统盘和数据盘不会被创建或回滚快照。不过 devices 不能是空列表，源码明确要求至少一个节点：block/snapshot.c:483。

磁盘和内存中内容需要一致的才可以。

## QEMU 的 RunState

qapi/run-state.json 中

```txt
  { 'enum': 'RunState',
    'data': [ 'debug', 'inmigrate', 'internal-error', 'io-error', 'paused',
              'postmigrate', 'prelaunch', 'finish-migrate', 'restore-vm',
              'running', 'save-vm', 'shutdown', 'suspended', 'watchdog',
              'guest-panicked', 'colo' ] }
```

可以自动生成这些东西:
```c
typedef enum RunState {
    RUN_STATE_DEBUG,
    RUN_STATE_INMIGRATE,
    RUN_STATE_INTERNAL_ERROR,
    RUN_STATE_IO_ERROR,
    RUN_STATE_PAUSED,
    RUN_STATE_POSTMIGRATE,
    RUN_STATE_PRELAUNCH,
    RUN_STATE_FINISH_MIGRATE,
    RUN_STATE_RESTORE_VM,
    RUN_STATE_RUNNING,
    RUN_STATE_SAVE_VM,
    RUN_STATE_SHUTDOWN,
    RUN_STATE_SUSPENDED,
    RUN_STATE_WATCHDOG,
    RUN_STATE_GUEST_PANICKED,
    RUN_STATE_COLO,
    RUN_STATE__MAX,
} RunState;
```

1. 常规生命周期状态

状态         含义
━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
prelaunch    启动后尚未运行，典型情况是使用了 -S
───────────  ──────────────────────────────────────────────────────
running      Guest 正在正常运行
───────────  ──────────────────────────────────────────────────────
paused       被 QMP/HMP stop 命令暂停
───────────  ──────────────────────────────────────────────────────
suspended    Guest 进入 ACPI S3 suspend
───────────  ──────────────────────────────────────────────────────
shutdown     Guest 已关机，但因为 -no-shutdown，QEMU 进程没有退出

2. 调试及异常停止状态

状态              含义
━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
debug             因 GDB 调试、断点或单步而停止
────────────────  ──────────────────────────────────────────
internal-error    QEMU/KVM 内部错误导致 Guest 无法继续执行
────────────────  ──────────────────────────────────────────
io-error          块设备 I/O 失败，设备配置为遇错暂停
────────────────  ──────────────────────────────────────────
watchdog          Watchdog 触发，配置动作为暂停
────────────────  ──────────────────────────────────────────
guest-panicked    Guest OS 报告 panic，配置动作为暂停

3. 迁移、快照状态

状态              含义
━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
inmigrate         目标端正在等待或接收 incoming migration
────────────────  ─────────────────────────────────────────
finish-migrate    源端暂停 Guest，完成迁移最后阶段
────────────────  ─────────────────────────────────────────
postmigrate       源端迁移成功后保持暂停
────────────────  ─────────────────────────────────────────
save-vm           暂停 Guest，保存 VM state/snapshot
────────────────  ─────────────────────────────────────────
restore-vm        暂停 Guest，恢复 VM state/snapshot
────────────────  ─────────────────────────────────────────
colo              COLO checkpoint 保存或恢复状态


  QEMU 显式维护允许的迁移矩阵，见 system/runstate.c:82。

  例如：

  PRELAUNCH → RUNNING
  RUNNING   → PAUSED
  RUNNING   → IO_ERROR
  RUNNING   → FINISH_MIGRATE
  PAUSED    → RUNNING
  FINISH_MIGRATE → POSTMIGRATE

说明下最经典的状态转移过程:

soruce 端:
```txt
runstate_set current_run_state 6 (prelaunch) new_state 9 (running)

runstate_set current_run_state 9 (running) new_state 7 (finish-migrate)
runstate_set current_run_state 7 (finish-migrate) new_state 5 (postmigrate)

(SIGTERM)
qemu-system-x86_64: terminating on signal 15 from pid 4147494 (/home/martins3/.nix-profile/bin/python3)
```

target 端:
```txt
runstate_set current_run_state 1 (inmigrate) new_state 9 (running)

(hmp 发送 q)
runstate_set current_run_state 9 (running) new_state 11 (shutdown)
```

### 是在什么 thread 下调用的?

看上去什么 thread 都是可以调用的:

- __clone3
  - start_thread
    - qemu_thread_start
      - bg_migration_thread
        - migration_stop_vm
          - do_vm_stop
            - runstate_set

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qmp_cont
        - vm_start
          - vm_prepare_start
            - runstate_set


### 经典案例

```txt
static void kvmclock_vm_state_change(void *opaque, bool running,
                                     RunState state)
```

## 热迁移的基本流程
<!-- 9ee517c5-92a8-4b2e-88a7-dd74c8bc99da -->


### 基本流程
进行 iteration 只有一个 thread 的，也就是
- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run : 在这里挑选进行 dirty bit 的跟踪，发送页面

- migration_iteration_run
	- 比较 s->threshold_size 如果基本上收敛了
	- migration_completion
		- migration_completion_precopy
			- migration_stop_vm
			- migration_switchover_start
				- migration_switchover_prepare
			- qemu_savevm_state_complete_precopy
				- qemu_savevm_state_complete_precopy_iterable
				- qemu_savevm_state_non_iterable
				- qemu_savevm_state_end_precopy
		- migration_completion_end : 可以确认

### 嵌入状态转换
将 notification 和状态转换嵌入之后
```c
typedef enum PrecopyNotifyReason {
    PRECOPY_NOTIFY_SETUP = 0,
    PRECOPY_NOTIFY_BEFORE_BITMAP_SYNC = 1,
    PRECOPY_NOTIFY_AFTER_BITMAP_SYNC = 2,
    PRECOPY_NOTIFY_COMPLETE = 3,
    PRECOPY_NOTIFY_CLEANUP = 4,
    PRECOPY_NOTIFY_MAX = 5,
} PrecopyNotifyReason;
```

```c
typedef enum MigrationStatus {
    MIGRATION_STATUS_NONE,
    MIGRATION_STATUS_SETUP,
    MIGRATION_STATUS_CANCELLING,
    MIGRATION_STATUS_CANCELLED,
    MIGRATION_STATUS_ACTIVE,
    MIGRATION_STATUS_POSTCOPY_DEVICE,
    MIGRATION_STATUS_POSTCOPY_ACTIVE,
    MIGRATION_STATUS_POSTCOPY_PAUSED,
    MIGRATION_STATUS_POSTCOPY_RECOVER_SETUP,
    MIGRATION_STATUS_POSTCOPY_RECOVER,
    MIGRATION_STATUS_COMPLETED,
    MIGRATION_STATUS_FAILING,
    MIGRATION_STATUS_FAILED,
    MIGRATION_STATUS_COLO,
    MIGRATION_STATUS_PRE_SWITCHOVER,
    MIGRATION_STATUS_DEVICE,
    MIGRATION_STATUS_WAIT_UNPLUG,
    MIGRATION_STATUS__MAX,
} MigrationStatus;
```

将 notification 和状态转换 (仅 precopy) 嵌入调用链后:

- migrate_init (migration.c:1709) : 【NONE -> SETUP】
- migration_thread
	- qemu_savevm_state_do_setup (migration.c:3712)
		- qemu_savevm_state_setup
			- ram_save_setup
				- ram_init_all
					- ram_init_bitmaps
						- migration_bitmap_sync_precopy : setup 阶段也会触发 BEFORE/AFTER_BITMAP_SYNC
		- precopy_notify(PRECOPY_NOTIFY_SETUP) (savevm.c:1471)
	- qemu_savevm_wait_unplug (migration.c:3715) : *SETUP -> ACTIVE*
	  有 failover 设备时 *SETUP -> WAIT_UNPLUG -> ACTIVE* (migration.c:3631/3654)
	- migration_iteration_run : iteration 循环期间一直是 *ACTIVE*
		- qemu_savevm_state_pending
			- ram_state_pending (`.save_query_pending`)
				- ram_state_pending_sync
					- migration_bitmap_sync_precopy (ram.c:1174)
						- precopy_notify(PRECOPY_NOTIFY_BEFORE_BITMAP_SYNC)
						- migration_bitmap_sync
						- precopy_notify(PRECOPY_NOTIFY_AFTER_BITMAP_SYNC)
		- 比较 s->threshold_size 如果成立，表示页面迁移完成，基本上收敛，可以开始传输 device state 了
		- migration_completion
			- migration_completion_precopy
				- migration_stop_vm
				- migration_switchover_start
					- migration_switchover_prepare : *ACTIVE -> DEVICE* (migration.c:2789)；
						  若开了 pause-before-switchover，则先 *ACTIVE -> PRE_SWITCHOVER* (migration.c:2801)，
						  等待 qmp migrate_continue 后 *PRE_SWITCHOVER -> DEVICE* (migration.c:2812)
					- qemu_savevm_query_pending_final : 最后一次 pending 查询， 同样经过 ram_state_pending_sync(final=true) 触发 BEFORE/AFTER_BITMAP_SYNC
					- migration_block_inactivate
					- precopy_notify_complete (migration.c:2853)
						- precopy_notify(PRECOPY_NOTIFY_COMPLETE) : **很奇怪** precopy 阶段不就是 device 阶段吗?
				- qemu_savevm_state_complete_precopy
					- qemu_savevm_state_complete_precopy_iterable
					- qemu_savevm_state_non_iterable
					- qemu_savevm_state_end_precopy
				- 失败时 : *ACTIVE/DEVICE -> FAILING* (migration.c:2951)
			- migration_completion_end : *DEVICE -> COMPLETED* (migration.c:3217)
	- migration_cancel (migration.c:1506) : 随时可能从 *SETUP/ACTIVE/WAIT_UNPLUG/PRE_SWITCHOVER -> CANCELLING*
	- migration_cleanup (migration.c:1326) : *CANCELLING -> CANCELLED* (migration.c:1374)，*FAILING -> FAILED* (migration.c:1377)
		- qemu_savevm_state_cleanup
			- precopy_notify(PRECOPY_NOTIFY_CLEANUP) (savevm.c:1881) : **彻底释放**

所以，可以看到 DEVICE 状态就是 switchover 状态了。

### 核心流程
- `qmp_migrate_incoming` / `qmp_migrate_recover`

输入:
- `fd_start_incoming_migration`
    - `fd_accept_incoming_migration` <------ 使用 fd 作为例子
        - `migration_channel_process_incoming`
            - `migration_ioc_process_incoming`
                - `qemu_file_new_input` : single connection
                - `multifd_recv_new_channel` : multiple connection
                - `migration_incoming_process` : 如果准备好了，那么开始
                    - `qemu_coroutine_create(process_incoming_migration_co, NULL);`
                        - `qemu_loadvm_state` : 就是从这里开始的
                        - 如果是 colo 模式，那么还会继续操作

- `qmp_migrate`
    - `fd_start_outgoing_migration`
        - `migration_channel_connect`
            - `migrate_fd_connect`
                - `migration_thread`
                    - `qemu_savevm_state_header`
                    - `qemu_savevm_state_setup`
                    - `qemu_savevm_wait_unplug`
                    - `migration_iteration_run`
                        - `qemu_savevm_state_pending`
                            - `::save_live_pending` : 将所有的 SaveStateEntry 的都执行
                        - `qemu_savevm_state_iterate` ：在这里对于 `savevm_state` 进行这个调用其 hook
                            - [ ] 为什么是 iterated 的，每一个 interaction 的划分标准是什么
                            - `::is_active`  居然只有 block
                            - `::is_active_iterate` 还是只有 block 的
                            - `::has_postcopy` ram 和 block
                            - `save_section_header`
                            - `::save_live_iterate` : vfio block ram
                            - `save_section_footer`
                        - `start_postcopy`

彻底完成 precopy
- `qemu_savevm_state_complete_precopy`
    - `qemu_savevm_state_complete_precopy_iterable`

那么中间发送资源的时间在什么地方?

应该观察下这个就可以了:
br qemu_savevm_state
br qemu_loadvm_state_main (<---- 原来)

migration_switchover_start 的东西

```c
void migration_bitmap_sync_precopy(bool last_stage)
{
    Error *local_err = NULL;
    assert(ram_state);

    /*
     * The current notifier usage is just an optimization to migration, so we
     * don't stop the normal migration process in the error case.
     */
    if (precopy_notify(PRECOPY_NOTIFY_BEFORE_BITMAP_SYNC, &local_err)) {
        error_report_err(local_err);
        local_err = NULL;
    }

    migration_bitmap_sync(ram_state, last_stage);

    if (precopy_notify(PRECOPY_NOTIFY_AFTER_BITMAP_SYNC, &local_err)) {
        error_report_err(local_err);
    }
}
```


## switchover 不是一个点，而是一个阶段
Switchover 就是热迁移的"收尾/切换"阶段——从源端把执行权切到目的端的那一步。它不是迁移状态机里的一个正式状态，而是 QEMU 内部对这段流程的称呼
（代码见 migration/migration.c:migration_switchover_start()）。

以默认的 precopy 为例，整个热迁移分两个阶段：

- 迭代拷贝阶段（active）：源端 guest 一直运行，QEMU 一轮一轮地把内存（以及后续被改脏的页）发往目的端。每轮结束后估算"剩余数据量 / 带宽"所需时间。
- Switchover 阶段：当估计剩余数据能在 downtime-limit 内传完时（migration_switchover_start() 被触发），进入收尾：
    1. 停掉源端 vCPU（guest 真正开始停机，downtime 从这里开始计时）；
    2. 把最后剩余的脏页 + 所有设备状态（CPU、块设备、网卡等 vmstate）一次性传完；
    3. 块设备同步完成后，目的端恢复运行，源端迁移状态变为 completed。

也就是说，switchover 期间 guest 是停止的，这段时间就是你测到的迁移 downtime。

状态机上的位置是：active → (pre-switchover) → device → completed。其中：

- PRE_SWITCHOVER 是中间状态（migration/migration.c:2801），只有开了 pause-before-switchover 能力时才停留——QEMU 会暂停在这里，
	等用户发 migrate_continue 才继续停机切换，用于需要人工确认窗口的场景。
- switchover-ack 能力（qapi/migration.json:510，需要 return-path）：目的端设备加载完数据后主动发 ACK，源端收到 ACK 才真正停 guest。
	这样停机窗口被压缩到很短，VFIO 设备的 precopy 迁移就依赖它。

另外 postcopy 模式下也有 switchover：postcopy-active 期间可以决定切回收尾流程（postcopy → pre-switchover → device），把目的端缺的页拉齐后完成切换。


```txt
- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - migration_completion
            - migration_completion_precopy
              - migration_switchover_start
                - migration_switchover_prepare
```

## iter 和 non-iter 的区别

可以观察到:

- qemu_savevm_state_complete_precopy
	- qemu_savevm_state_complete_precopy_iterable
	- qemu_savevm_state_non_iterable

这两个名字描述的是设备状态在迁移过程中能不能在 VM 还运行时进行多轮迭代传输。

iterable（可迭代的）

指那些注册了 SaveVMHandlers 并且带 save_setup / save_live_iterate / save_complete 回调的设备状态。典型例子：

- RAM：运行中一页一页地扫描 dirty page，反复传脏页。
- VFIO 设备：在 PRE_COPY 状态下反复读取 vendor 驱动的 dirty/initial bytes。

这些状态的特点是：

- 可以在源 VM 运行时不断传一部分；
- 停 VM 之后还需要再传最后一轮剩余数据，这就是 qemu_savevm_state_complete_precopy_iterable() 做的事情；
- 该函数调用每个 iterable 设备的 ->save_complete()（migration/savevm.c:1581）。

non-iterable（不可迭代的）

指普通的 VMState 设备状态，没有 save_setup，只在 VM 停下来之后一次性保存。典型例子：

- CPU 状态（寄存器等）：必须在 vCPU 都停下来、状态同步之后才能抓快照；
- 大部分普通设备配置。

qemu_savevm_state_non_iterable() 里会先 cpu_synchronize_all_states()，然后遍历所有 non-iterable 的 SaveStateEntry，
调用 vmstate_save() 把它们写到迁移流 。

整体调用顺序

在 precopy 迁移里，大致流程是：

1. qemu_savevm_state_do_setup()
    - non_iterable_early()：先保存标记了 early_setup 的 VMState；
    - setup()：调用 iterable 设备的 save_setup()。
2. 迁移线程循环调用 qemu_savevm_state_iterate()，让 iterable 设备反复 save_live_iterate()。
3. 决定 switchover 后，源 VM 停止。
4. qemu_savevm_state_complete_precopy()：
    - 先 complete_precopy_iterable()：停止 VM 后，对 iterable 设备做最后一轮 save_complete()；
    - 再 non_iterable()：一次性保存 CPU 等不可迭代状态。

所以简单说：

- iterable = VM 运行时能分多轮传，停 VM 后还要收尾的状态。
- non-iterable = 必须等 VM 停下来才能一次性快照的状态。

### 如何区分

判定逻辑在 migration/savevm.c 里：
```c
/* Is a save state entry iterable (e.g. RAM)? */
static bool qemu_savevm_se_iterable(SaveStateEntry *se)
{
    return se->ops && se->ops->save_setup;
}
```

Iterabl

- 迭代阶段 qemu_savevm_state_iterate 只调用提供了 save_live_iterate 的 handler（savevm.c:1511），发 QEMU_VM_SECTION_PART
- 停机收尾阶段 qemu_savevm_state_complete_precopy_iterable 只调用提供了 save_complete 的 handler（savevm.c:1562），发 QEMU_VM_SECTION_END

Non-iterable（一把梭）：qemu_savevm_state_non_iterable（savevm.c:1740）在停机后对所有 entry 调 vmstate_save，发 QEMU_VM_SECTION_FULL，一次存完

所以两种情况：

1. 注册了 SaveVMHandlers 但只提供 save_state/load_state（老式 register_savevm）→ 仍然是 non-iterable。vmstate_save 里会走 ops->save_state 分支
   （savevm.c:1069、1086-1087），在停机阶段一次性保存。
2. 没注册 SaveVMHandlers，用 VMStateDescription（绝大多数设备，经 vmstate_register/DCATEGORY）→ se->ops == NULL，自然也是 non-iterable，走
   vmstate_save_vmsd。

也就是说，真正的 iter 设备必须注册带 save_setup + save_live_iterate + save_live_complete_precopy 这一整套 live 回调的 handlers——RAM 是典型，还
有 block、VFIO、部分 virtio 设备等。只注册了 SaveVMHandlers 但没提供 live 回调的，语义上还是 non-iterable。

### 注册了 SaveVMHandlers 但只提供 save_state/load_state 的情况
一般来说，就是 se->ops 来判断了就可以了，但是存在一些特殊设备

1. slirp 用户态网络（net/slirp.c:427）

```c
  static SaveVMHandlers savevm_slirp_state = {
      .save_state = net_slirp_state_save,
      .load_state = net_slirp_state_load,
  };
  ...
  register_savevm_live("slirp", VMSTATE_INSTANCE_ID_ANY,
                       slirp_state_version(), &savevm_slirp_state, s->slirp);
```

注意这里虽然用的注册函数名叫 register_savevm_live，但 handlers 里没有 save_setup/save_live_iterate/save_complete 任何一个 live 回调，所以按
qemu_savevm_se_iterable() 的判定（se->ops && se->ops->save_setup）它就是 non-iterable——停机后在 qemu_savevm_state_non_iterable 里通过
vmstate_save 的 ops->save_state 分支一次性存完。

2. s390x TOD 时钟（hw/s390x/tod.c:106）

```c
  static SaveVMHandlers savevm_tod = {
      .save_state = s390_tod_save,
      .load_state = s390_tod_load,
  };

  static void s390_tod_realize(DeviceState *dev, Error **errp)
  {
      S390TODState *td = S390_TOD(dev);

      /* Legacy migration interface */
      register_savevm_live("todclock", 0, 1, &savevm_tod, td);
  }
```

kimi 的分析有一定的道理的:

有些状态本质上就不适合 VMState

VMState 是"静态描述 struct 字段 → 自动生成序列化"的模型，前提是状态能用 QEMU 的 C struct 字段描述清楚。但有些设备做不到：

- slirp：它的迁移格式由 libslirp 自己定义和产生（slirp_state_save 接收一个 write 回调往流里写），QEMU 这边根本没有对应的 struct 字段可描述，只 能当一段不透明字节流转发。
 有些设备 save/load 里有复杂的条件逻辑、运行时才能确定的布局，硬塞进声明式表反而更绕。

### 一个小小的实验

给 trace_vmstate_downtime_save 挂上 trace ，一共两个位置，结果应该是很清晰了:

```txt
vmstate_downtime_save type=iterable idstr=ram instance_id=0 downtime=570
vmstate_downtime_save type=iterable idstr=dirty-bitmap instance_id=0 downtime=0

vmstate_downtime_save type=non-iterable idstr=apic instance_id=0 downtime=11
vmstate_downtime_save type=non-iterable idstr=apic instance_id=1 downtime=7
vmstate_downtime_save type=non-iterable idstr=0000:00:0b.0/pcie-root-port instance_id=0 downtime=14
vmstate_downtime_save type=non-iterable idstr=timer instance_id=0 downtime=6
vmstate_downtime_save type=non-iterable idstr=slirp instance_id=0 downtime=4
vmstate_downtime_save type=non-iterable idstr=ram instance_id=0 downtime=0
vmstate_downtime_save type=non-iterable idstr=dirty-bitmap instance_id=0 downtime=0
vmstate_downtime_save type=non-iterable idstr=cpu_common instance_id=0 downtime=3
vmstate_downtime_save type=non-iterable idstr=cpu instance_id=0 downtime=72
vmstate_downtime_save type=non-iterable idstr=kvm-tpr-opt instance_id=0 downtime=7
vmstate_downtime_save type=non-iterable idstr=cpu_common instance_id=1 downtime=1
vmstate_downtime_save type=non-iterable idstr=cpu instance_id=1 downtime=56
vmstate_downtime_save type=non-iterable idstr=kvmclock instance_id=0 downtime=3
vmstate_downtime_save type=non-iterable idstr=0000:00:00.0/I440FX instance_id=0 downtime=3
vmstate_downtime_save type=non-iterable idstr=PCIHost instance_id=0 downtime=2
vmstate_downtime_save type=non-iterable idstr=PCIBUS instance_id=0 downtime=3
vmstate_downtime_save type=non-iterable idstr=pflash_cfi01 instance_id=0 downtime=2
vmstate_downtime_save type=non-iterable idstr=pflash_cfi01 instance_id=1 downtime=2
vmstate_downtime_save type=non-iterable idstr=fw_cfg instance_id=0 downtime=5
vmstate_downtime_save type=non-iterable idstr=dma instance_id=0 downtime=7
vmstate_downtime_save type=non-iterable idstr=dma instance_id=1 downtime=6
vmstate_downtime_save type=non-iterable idstr=mc146818rtc instance_id=0 downtime=8
vmstate_downtime_save type=non-iterable idstr=0000:00:01.1/ide instance_id=0 downtime=28
vmstate_downtime_save type=non-iterable idstr=i2c_bus instance_id=0 downtime=2
vmstate_downtime_save type=non-iterable idstr=0000:00:01.3/piix4_pm instance_id=0 downtime=241
vmstate_downtime_save type=non-iterable idstr=0000:00:01.0/PIIX3 instance_id=0 downtime=4
vmstate_downtime_save type=non-iterable idstr=i8259 instance_id=0 downtime=7
vmstate_downtime_save type=non-iterable idstr=i8259 instance_id=1 downtime=4
vmstate_downtime_save type=non-iterable idstr=ioapic instance_id=0 downtime=3
vmstate_downtime_save type=non-iterable idstr=i8254 instance_id=0 downtime=6
vmstate_downtime_save type=non-iterable idstr=pcspk instance_id=0 downtime=2
vmstate_downtime_save type=non-iterable idstr=serial instance_id=0 downtime=6
vmstate_downtime_save type=non-iterable idstr=fdc instance_id=0 downtime=15
vmstate_downtime_save type=non-iterable idstr=ps2kbd instance_id=0 downtime=4
vmstate_downtime_save type=non-iterable idstr=ps2mouse instance_id=0 downtime=4
vmstate_downtime_save type=non-iterable idstr=pckbd instance_id=0 downtime=4
vmstate_downtime_save type=non-iterable idstr=vmmouse instance_id=0 downtime=18
vmstate_downtime_save type=non-iterable idstr=port92 instance_id=0 downtime=1
vmstate_downtime_save type=non-iterable idstr=smbus-eeprom instance_id=0 downtime=1
vmstate_downtime_save type=non-iterable idstr=smbus-eeprom instance_id=1 downtime=0
vmstate_downtime_save type=non-iterable idstr=smbus-eeprom instance_id=2 downtime=1
vmstate_downtime_save type=non-iterable idstr=smbus-eeprom instance_id=3 downtime=0
vmstate_downtime_save type=non-iterable idstr=smbus-eeprom instance_id=4 downtime=1
vmstate_downtime_save type=non-iterable idstr=smbus-eeprom instance_id=5 downtime=0
vmstate_downtime_save type=non-iterable idstr=smbus-eeprom instance_id=6 downtime=0
vmstate_downtime_save type=non-iterable idstr=smbus-eeprom instance_id=7 downtime=0
vmstate_downtime_save type=non-iterable idstr=0000:00:02.0/virtio-scsi instance_id=0 downtime=290
vmstate_downtime_save type=non-iterable idstr=0000:00:02.0/0:1:0/scsi-disk instance_id=0 downtime=7
vmstate_downtime_save type=non-iterable idstr=0000:00:02.0/0:2:0/scsi-disk instance_id=0 downtime=6
vmstate_downtime_save type=non-iterable idstr=0000:00:03.0/virtio-blk instance_id=0 downtime=277
vmstate_downtime_save type=non-iterable idstr=0000:00:04.0/virtio-net instance_id=0 downtime=270
vmstate_downtime_save type=non-iterable idstr=0000:00:05.0/virtio-net instance_id=0 downtime=288
vmstate_downtime_save type=non-iterable idstr=0000:00:06.0/virtio-vhost_vsock instance_id=0 downtime=279
vmstate_downtime_save type=non-iterable idstr=0000:00:07.0/vhost-user-fs instance_id=0 downtime=4166
vmstate_downtime_save type=non-iterable idstr=0000:00:08.0/virtio-balloon instance_id=0 downtime=464
vmstate_downtime_save type=non-iterable idstr=0000:00:0a.0/virtio-console instance_id=0 downtime=482
vmstate_downtime_save type=non-iterable idstr=PCIBUS instance_id=1 downtime=3
vmstate_downtime_save type=non-iterable idstr=0000:00:0c.0/virtio-rng instance_id=0 downtime=509
vmstate_downtime_save type=non-iterable idstr=acpi_build instance_id=0 downtime=2
vmstate_downtime_save type=non-iterable idstr=globalstate instance_id=0 downtime=8
```


## 基本 hmp 命令

```txt
migrate                 migrate_cancel          migrate_continue
migrate_incoming        migrate_pause           migrate_recover
migrate_set_capability  migrate_set_parameter   migrate_start_postcopy
```

### [ ] post 相关的
```txt
(qemu) migrate_pause
Error: migrate-pause is currently only supported during postcopy-active or postcopy-recover state
```

确定一个问题，热迁移的时候，内核中只有一个东西:

### info status

info status 一共有下面 16 种
```txt
VM status: running
VM status: paused
VM status: paused (debug)
VM status: paused (inmigrate)
VM status: paused (internal-error)
VM status: paused (io-error)
VM status: paused (postmigrate)
VM status: paused (prelaunch)
VM status: paused (finish-migrate)
VM status: paused (restore-vm)
VM status: paused (save-vm)
VM status: paused (shutdown)
VM status: paused (suspended)
VM status: paused (watchdog)
VM status: paused (guest-panicked)
VM status: paused (colo)
```

各状态含义：

 状态              含义
━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 running           Guest 正在运行
────────────────  ────────────────────────────────────────────────
 paused            通过 HMP stop 等操作主动暂停
────────────────  ────────────────────────────────────────────────
 debug             被 GDB/debugger 暂停
────────────────  ────────────────────────────────────────────────
 inmigrate         目标端正在等待或接收迁移
────────────────  ────────────────────────────────────────────────
 internal-error    QEMU 内部错误导致 Guest 无法继续
────────────────  ────────────────────────────────────────────────
 io-error          磁盘 I/O 错误，设备策略配置为暂停 VM
────────────────  ────────────────────────────────────────────────
 postmigrate       源端成功完成迁移后保持暂停
────────────────  ────────────────────────────────────────────────
 prelaunch         使用 -S 启动，Guest 尚未开始执行
────────────────  ────────────────────────────────────────────────
 finish-migrate    正在完成迁移的停机阶段
────────────────  ────────────────────────────────────────────────
 restore-vm        正在恢复 VM 快照/状态
────────────────  ────────────────────────────────────────────────
 save-vm           正在保存 VM 快照/状态
────────────────  ────────────────────────────────────────────────
 shutdown          Guest 已关机，但 QEMU 因 -no-shutdown 没有退出
────────────────  ────────────────────────────────────────────────
 suspended         Guest 处于挂起状态，例如 ACPI S3
────────────────  ────────────────────────────────────────────────
 watchdog          Watchdog 触发，且动作配置为暂停
────────────────  ────────────────────────────────────────────────
 guest-panicked    QEMU 收到 Guest OS panic 通知
────────────────  ────────────────────────────────────────────────
 colo              VM 处于 COLO checkpoint 保存/恢复阶段

输出规则在 system/runstate-hmp-cmds.c:hmp_info_status

- 只有 RUN_STATE_RUNNING 输出 VM status: running
- 普通 RUN_STATE_PAUSED 只输出 VM status: paused
- 其他所有状态都输出 VM status: paused (<具体状态>)

完整枚举定义见 qapi/run-state.json 中。

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

### postcopy 专用

严格来说，以下 3 个命令仅用于 postcopy：

```txt
 HMP 命令                  使用位置        作用
━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 migrate_start_postcopy    源端            将已经开始的 precopy 迁移切换到 postcopy
────────────────────────  ──────────────  ────────────────────────────────
 migrate_pause             源端或目标端    主动中断当前 postcopy 迁移通道
────────────────────────  ──────────────  ────────────────────────────────
 migrate_recover URI       目标端          为处于 postcopy-paused 的迁移建立新接收通道
```

其余命令不是 postcopy 专用：

- migrate [-d] URI：发起普通迁移，precopy/postcopy 都从它开始。只有
  migrate -r URI 是 postcopy 断线恢复专用。

- migrate_cancel：取消普通迁移；但 postcopy 阶段一旦真正启动，就不允许取
  消，因为源端已经缺少部分内存页。

- migrate_continue pre-switchover：配合 pause-before-switchover
  capability，允许管理程序在切换点检查后继续。它不是恢复 postcopy 网络连接
  的命令。

- migrate_incoming URI：配合目标端启动参数 -incoming defer，开始接收普通迁
  移。

- migrate_set_capability：通用配置命令，其中有些 capability 与 postcopy 有
  关，例如 postcopy-ram、postcopy-preempt。

- migrate_set_parameter：通用迁移参数配置，其中部分参数仅影响 postcopy。

典型 postcopy 流程：

```txt
# 源端和目标端都设置
migrate_set_capability postcopy-ram on

# 源端先正常发起迁移
migrate -d tcp:<目标IP>:4444

# 再切换到 postcopy
migrate_start_postcopy

断线后恢复：
# 目标端
migrate_recover tcp:0:4445
# 源端
migrate -d -r tcp:<目标IP>:4445
```

因此，可以简单记为：

postcopy 专用命令：
    migrate_start_postcopy
    migrate_pause
    migrate_recover

postcopy 专用选项：
    migrate -r


#### migrate_pause
migrate_pause 的核心用途是：主动切断当前 postcopy 通道，让
迁移进入一个“可以重新连接”的 postcopy-paused 状态。

它暂停的是迁移，不是直接执行 vm_stop 暂停 VM。

为什么需要它？因为 postcopy 有一个“无法简单回滚”的阶段：

1. 目标端 VM 已经开始运行。
2. 一部分内存页已经在目标端。
3. 另一部分内存页仍留在源端，目标端按缺页请求获取。
4. 此时网络异常后：
    - 源端不能恢复运行，因为最新内存状态可能已经在目标        端；
    - 目标端也不能独立运行，因为它仍可能缺少内存页；
    - migrate_cancel 也不再安全。

因此只能保留两端状态，修复网络后重新建立迁移通道。

正常情况下，明确的 socket I/O 错误会让 QEMU自动进入
postcopy-paused，不需要手工执行 migrate_pause。这个命令主
要是一个管理层“逃生开关”，用于以下情况：

- 网络黑洞：TCP 看起来仍是 ESTABLISHED，但数据包被防火墙或
  路由丢弃，QEMU迟迟检测不到断线。

- 主动切换迁移网络：旧通道质量差，需要改用新的 IP、端口或路径。

- 两端状态不同步：一端已进入 postcopy-paused，另一端仍停在 postcopy-active 或 postcopy-recover。

- 恢复过程再次卡住：先用 migrate_pause 退出失败的 recovery，再换 URI 重试。

- 测试 postcopy 断线恢复流程。

执行时，它实际会关闭当前迁移 QEMUFile 通道，促使迁移线程进 入暂停状态。migration/migration.c:1895

典型处理流程：

```txt
# 发现迁移长时间卡在 postcopy-active
(qemu) info migrate

# 在任意一端主动切断旧迁移通道
(qemu) migrate_pause

通常断开事件会传播到另一端。如果没有传播，可以在另一          端也执
行：

(qemu) migrate_pause

确认两端：

(qemu) info migrate
Migration status: postcopy-paused

然后重新连接：

# 目标端监听新通道
(qemu) migrate_recover tcp:0:4445

# 源端连接新通道并恢复
(qemu) migrate -d -r tcp:<目标IP>:4445
```

需要注意：目标端的 info status 可能仍显示 running，但访问
尚未传输页面的 vCPU/线程会阻塞，因此 guest 可能部分或完      全
卡住。migrate_pause 不适合作为普通的迁移限速或长期暂停手
段，它主要服务于 postcopy 故障恢复。

## 记录 migration 相关的 trace
大量的调用:
```txt
migration_transferred_bytes qemu_file 2727 multifd 53855087 RDMA 0
multifd_recv_unfill channel 1 packet_num 31 flags 0x4 next packet size 524303
multifd_send_fill channel 2 packet_num 6454 flags 0x4 next packet size 0
multifd_send_ram_fill channel 0 normal pages 128 zero pages 0
```

然后就是
```txt
ram_load_complete exit_code 0 seq iteration 53
```

```txt
vmstate_subsection_load HIDPointerEventQueue
vmstate_load_state virtio_pci/modern_queue_state v1
vmstate_load_state_field xhci-intr:erstba_low exists=1
vmstate_field_exists xhci-intr:ev_buffer_put field_version 0 version 1 result 0
vmstate_subsection_load_good virtio_pci/modern_queue_state
vmstate_load_state_end virtio_pci/modern_queue_state end/0
vmstate_n_elems erstba_high: 1
```

这似乎就是整个 ram 迭代的过程:
```txt
savevm_section_start ram, section_id 2
savevm_section_end ram, section_id 2 -> 0
migration_rate_limit_pre 94 ms
migration_rate_limit_post urgent: 0
migrate_transferred transferred 111570 time_spent 100 bandwidth 1115 switchover_bw 0 max_size 334710
migrate_pending_estimate estimate pending size 8440582144 (pre = 8440582144 post=0)
savevm_state_iterate
```

```txt
migration_bitmap_clear_dirty rb mem0 start 0x0 size 0x40000000 page 0x0
migration_bitmap_clear_dirty rb mem0 start 0x40000000 size 0x40000000 page 0x40000
migration_bitmap_clear_dirty rb mem0 start 0x80000000 size 0x40000000 page 0x80000
migration_bitmap_clear_dirty rb mem0 start 0xc0000000 size 0x40000000 page 0xc0000
migration_bitmap_clear_dirty rb mem0 start 0x100000000 size 0x40000000 page 0x100000
```

```txt
vmstate_save_state_loop xhci-intr/iman[1]
vmstate_save_state_loop xhci-intr/imod[1]
vmstate_save_state_loop xhci-intr/erstsz[1]
vmstate_save_state_loop xhci-intr/erstba_low[1]
vmstate_save_state_loop xhci-intr/erstba_high[1]
vmstate_save_state_loop xhci-intr/erdp_low[1]
vmstate_save_state_loop xhci-intr/erdp_high[1]
vmstate_save_state_loop xhci-intr/msix_used[1]
vmstate_save_state_loop xhci-intr/er_pcs[1]
vmstate_save_state_loop xhci-intr/er_start[1]
vmstate_save_state_loop xhci-intr/er_size[1]
vmstate_save_state_loop xhci-intr/er_ep_idx[1]
```

这个东西:
```txt
vmstate_save_state_top virtio_pci/modern_queue_state
vmstate_save_state_loop virtio_pci/modern_queue_state/num[1]
vmstate_save_state_loop virtio_pci/modern_queue_state/unused[1]
vmstate_save_state_loop virtio_pci/modern_queue_state/enabled[1]
vmstate_save_state_loop virtio_pci/modern_queue_state/desc[2]
vmstate_save_state_loop virtio_pci/modern_queue_state/avail[2]
vmstate_save_state_loop virtio_pci/modern_queue_state/used[2]
vmstate_subsection_save_top virtio_pci/modern_queue_state
```

## 热迁移的跳过策略

跳过一页并保持 dirty：不调清位函数即可

要跳过某页且保持 dirty，直接在扫描循环里跳过它，不调用 migration_bitmap_clear_dirty。效果是：

- bmap 里的位保持置 1,migration_dirty_pages 不减，下一轮的统计（ram_save_pending）仍然包含这页；
- KVM/listener 侧的 dirty 位也没被清，guest 后续再写这页会被正常捕获；
- 扫描游标（pss_find_next_dirty + rs->last_seen_block/last_page,ram.c:2371）会越过去，绕一圈（complete_round）后 find_dirty_block 会再次找到它，可以在那时再决定是否发送。

上游有一个现成先例：postcopy-preempt 的 ram_save_host_page_urgent(ram.c:2165）
发现 precopy 通道正在发同一页时，直接 return 0 不清位，把页留给 precopy 通道发。

两个必须注意的坑

1. 收敛判定不看 bitmap 是否为空:find_dirty_block 的 PAGE_ALL_CLEAN 条件是"绕完整 RAM 一圈没发出去任何东西"(ram.c:1369)。如果你永久跳过某页，扫描会绕过去、整圈零发送，migration 照 样判定完成——这页就永远没去 dest，precopy 下就是数据缺失。所以跳过的页必须有个兜底：在 last stage(VM 停机后）强制发掉，或者像 postcopy 那样让 dest 缺页时再请求回来。
2. last stage 语义:migration_bitmap_clear_dirty 的注释（ram.c:835-841）指出 last stage 之后不再重新 track——如果迁移在停机阶段失败，tracking 本来就放弃了。所以"保持 dirty 下次再发 "只在迭代阶段有意义。

## memory_region 的生命周期规划

热迁移会保留 memory region 的数量，而不是直接合并的

## 热迁移的过程中不可以热迁移
ram_mig_ram_block_resized 中的注释说:
```c
    if (!migration_is_idle()) {
        /*
         * Precopy code on the source cannot deal with the size of RAM blocks
         * changing at random points in time - especially after sending the
         * RAM block sizes in the migration stream, they must no longer change.
         * Abort and indicate a proper reason.
         */
        error_setg(&err, "RAM block '%s' resized during precopy.", rb->idstr);
        migration_cancel(err);
        error_free(err);
    }
```

那么自动可以有这个东西:

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_create_cli_devices
        - qemu_opts_foreach
          - device_init_func
            - qdev_device_add
              - qdev_device_add_from_qdict
                - migration_is_running

## 调查 qemu_savevm_state_end 的调用者就可以了

```txt
qemu_savevm_state_end()
│
├─ qemu_savevm_state_complete_postcopy()
│    └─ migration_completion_postcopy()
│
├─ qemu_savevm_state_end_precopy()
│    ├─ qemu_savevm_state_complete_precopy()
│    │    ├─ 普通 precopy 热迁移完成
│    │    └─ savevm 内部快照
│    └─ bg_migration_thread()
│         └─ background-snapshot
│
├─ qemu_save_device_state()
│    ├─ COLO checkpoint
│    └─ Xen device-state 保存
│
└─ colo_do_checkpoint_transaction()
     └─ COLO 的 iterable/live state
```
其实也就是 precopy postcopy 和 background-snapshot

```c
void qemu_savevm_state_end(QEMUFile *f)
{
    qemu_put_byte(f, QEMU_VM_EOF);
}
```

## external link

1. https://github.com/abbbi/qmpbackup
1. 分析其他 Hypervisor 上是如何进行热迁移的
	- https://github.com/cloud-hypervisor/cloud-hypervisor
1. Linux 中支持 CONFIG_CHECKPOINT_RESTORE 来实现将 process 的状态保存，进而来实现容器的迁移

3. firecracker/examples/cmd/remote-snapshotter/README.md

## snapshot 嵌套虚拟化需要考虑吗
<!-- 3ac2499c-e366-425e-9761-9635ed4576fc -->
migration 是一定需要考虑嵌套，那么 snapshot 需要吗?

## 其他的考虑
3. 热迁移的时候，中断都是需要保持的 kvm_vcpu_ioctl_x86_get_vcpu_events
	- kvm 中也需要保存 timer

1. 热迁移的时候，如果 guest 当时在 perf，pmu 是需要维护的

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
