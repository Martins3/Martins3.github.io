# 2026-07-20 Phase 2 JBD2 迁移进展

## 当前结论

Phase 2 已于 2026-07-26 完成最终验收。当前内核、模块和 mkfs 产物统一后，
一次连续运行完整覆盖 `generic/001`--`generic/787`，结果为
PASS 432 / NOTRUN 355 / FAIL 0 / TIMEOUT 0。787 个结果和日志一一对应、顺序完整
且无重复，355 个 NOTRUN 全部保留了明确原因。

元数据缓存边界已经固定：SimpleFS 核心路径保持 folio-native；
`buffer_head` 只允许出现在 `simplefs_journal.c` 的 JBD2 适配层中，用于调用
上游 JBD2 客户端 API，不再把 buffer_head 扩散到 inode、目录、extent、xattr、
superblock 或普通文件 I/O 路径。

## 已完成的 JBD2 迁移

- mkfs 写标准大端 JBD2 v2 superblock，日志区至少 1024 blocks；
- mount 使用 `jbd2_journal_init_dev()` 和 `jbd2_journal_load()`；
- transaction wrapper 使用 `jbd2_journal_start/stop()`，当前仍以串行 handle 和
  每事务 checkpoint 保证调试阶段的确定性；
- 修正 JBD2 日志块映射：JBD2 传入的是日志内逻辑块号，`j_bmap` 统一加
  `s_journal_start`，避免日志写入文件系统 home block；
- inode、目录、extent、bitmap、xattr 和 superblock 的 metadata dirty 路径通过
  folio 参数进入单一 JBD2 适配层；适配层先取得 write access，再把 folio 或
  bitmap snapshot 同步到 JBD2 buffer，最后调用
`jbd2_journal_dirty_metadata()`；
- 元数据释放使用 JBD2 revoke/forget；核心只传递块号，相关 buffer_head 仍封装在
  journal adapter 内；
- `norecovery` 继续保持只读、跳过 replay 的语义。

## inode table block 1 覆盖问题

### 证据

`generic/001` 早期失败时，文件系统 block 1（inode table）被目录块内容覆盖。
在 `submit_bh_wbc()` 上跟踪到真正写 block 1 的调用链是：

```text
submit_bh_wbc
__flush_batch
jbd2_log_do_checkpoint
jbd2_journal_flush
simplefs_journal_stop
```

这排除了“JBD2 日志物理映射仍写错位置”，说明 checkpoint 正在提交一个内容
错误的 home buffer。进一步记录 source folio 和目标块号，得到：

```text
target block=1, source folio index=1698
simplefs_jbd2_dirty
simplefs_create_internal
simplefs_create
```

因此错误发生在 create 的目录元数据提交：目录数据 folio 本身正确，但提交时
重新从父目录 extent 映射计算目标块号，目标已经变成 block 1，适配层随后把目录
数据复制到了 inode table 的 JBD2 buffer。

### 修复

- 在目录数据 folio 映射时立即保存 `dir_block_nr`，dirty 时只使用这个稳定块号，
  不在嵌套元数据操作后重新从外层 extent 映射取值；
- create/unlink 都先提交并释放内层目录数据 folio，再修改和提交外层目录索引
  folio，保持嵌套映射的生命周期和提交顺序清晰；
- JBD2 适配层在复制前验证目标块的字节范围确实位于 source folio 内，不匹配时
  返回 `-EFSCORRUPTED`，避免错误元数据继续进入 checkpoint；
- `jbd2_journal_get_write_access()` 必须发生在 folio 内容复制到 JBD2 buffer 之前，
  让 JBD2 先捕获 home buffer 的旧映像。

## generic/051 并发与崩溃恢复修复

JBD2 版首次运行 `generic/051` 暴露了从锁顺序到崩溃原子性的多层问题：

- namespace transaction 持有 JBD2 handle 时调用 `write_inode_now()`，会进入文件
  data writeback 并获取 `extent_lock`；另一侧 extent 更新持锁后启动 journal，
  形成 `jbd2_handle -> extent_lock -> journal` 反向依赖。namespace 路径现直接把
  VFS inode 快照写入 inode-table folio/JBD2，不再等待通用 data writeback；
- link/symlink/mknod 共用的旧 `simplefs_add_to_dir()` 仍直接
  `folio_mark_dirty()`。现已与 create/unlink 一样，通过 folio 参数进入 JBD2，
  并固定目录数据块号及内外层 folio 提交顺序；
- extent leaf 回收不能再依赖普通 folio invalidate 表达“日志元数据已释放”。
  现在由 JBD2 adapter 执行 revoke/forget，再更新空闲位图；
- 无显式 filesystem handle 的 block/inode bitmap allocate/free 过去只改内存，
  crash 后会出现 mapped-but-free。现在 bitmap helper 在获取 `bitmap_lock` 前先取得
  JBD2 handle，锁顺序统一为 journal → bitmap，并把位图和 superblock free-count
  放进同一事务；
- nlink=0 的 evict 过去分别提交 data/metadata block free 和 inode free。现在磁盘
  inode 清零、extent/目录/xattr 块回收、revoke、block bitmap 和 inode bitmap
  处于同一 JBD2 transaction；
- forced-shutdown 卸载在销毁 JBD2 后丢弃残留 bdev folio，防止旧映像跨 recovery
  mount 延迟写回；卸载后期即使 `sb->s_root` 已清空，也能直接从 superblock/journal
  启动后台 inode-table transaction，不再伪报 `-EIO`。

## 当前验证

测试环境：`yyds-fs`，kernel `7.1.2-00001-gd41bd6abfe34`，当前 worktree
重新构建的 `simplefs.ko`。

- `./build.sh`：PASS；
- `git diff --check`：PASS；
- 40 个文件创建后卸载、重挂载并逐文件检查：PASS；
- `generic/001`：PASS（114 秒），内核日志无 WARNING、BUG、Oops、元数据块号
  错配；
- `generic/051`：PASS；最终复跑 dmesg 无 lockdep、JBD2 dirty-buffer、retire
  `-EBUSY`、double-free、free-count 修正、inode-table `-EIO`、WARNING、BUG 或
  Oops；
- `rg` 检查实际 `buffer_head` API/类型只存在于 `simplefs_journal.c`。

## 后续顺序

1. 重跑 `generic/475` 和 `generic/050`，修复剩余故障恢复语义；
2. 实现或接入 `generic/052/054/055` 所需的 simplefs log-state probing；
3. 执行 dm-flakey 和 dm-logwrites 批次；
4. 移除调试阶段的每事务完整 checkpoint，验证正常 JBD2 commit/checkpoint
   生命周期；
5. 全量复跑 787 个 generic case，要求 FAIL 0 / TIMEOUT 0，并保留所有 NOTRUN
   的明确原因。

## 记录规则

目标未完成而阶段性结束时，必须同步更新本文件、`readme.md` 和 `roadmap.md`，
不能只在对话中报告。

## 2026-07-21：异步 commit 与 generic/055（未完成）

- 新增持久化 `s_needs_recovery` 标志，修复失败 mount 的 root inode 所有权释放；
  `generic/050`、`generic/052` 已定向 PASS；
- xfstests harness 已支持 SimpleFS log-state probe 和 `-l defaults` 兼容参数，
  `generic/054` 已完成 20 轮 sync/nosync 并 PASS；
- inode writeback、clear inode、extent、xattr、symlink、mknod、tmpfile、卷标和文件尾
  清零路径都改为在 `simplefs_get_folio()` 前启动或加入 transaction；活跃 JBD2 下
  `simplefs_journal_dirty_folio()` 不再允许“修改后补开 handle”；
- 已移除每个 handle 的 `h_sync` 等待，055 从单轮约 220 秒降到约 4 秒；同步和卸载
  仍使用完整 checkpoint，`GOING_DOWN_LOGFLUSH` 只写回 dirty inode 并等待 JBD2
  commit，不主动 checkpoint home block；
- 异步运行首次暴露 freed metadata block 在 freeing transaction commit 前被复用，
  导致同一 transaction 重复 revoke；进一步发现 JBD2 开启下一事务时会清 revoke
  标志，但旧 journal head 仍可能位于 committing transaction 的 `BJ_Forget`。
  分配器现在通过 JBD2 适配层同时检查 revoke 和 running/committing/next transaction
  所有权，跳过尚不可复用的候选；
- `buffer_head` 边界扫描仍为零：除 `simplefs_journal.c` 外没有类型、头文件或 API
  依赖。

修复后 `generic/055` 的完整 10 轮已 PASS（40 秒），dmesg 无 duplicate revoke、
journal abort、JBD2 assertion、WARNING、BUG 或 Oops。此前 crash 前 `ls -RF` 把
少数目录读成普通文件、replay 后恢复目录的直接原因，是旧 `BJ_Forget` 块跨事务
提前复用，不是 inode generation；延迟 inode 号复用的隔离实验没有改善结果，已
撤回。

同一状态下已定向 PASS：001、050、051、052、054、055、475。文件尾清零最初在
只读 extent 查询前启动 transaction，051 的 lockdep 发现这会形成 journal →
extent 与普通分配 extent → journal 的反向锁序；现改为先完成只读映射查询，再在
映射和修改目标 folio 前启动 transaction，051 已重新干净 PASS。全量 787 case、
dm-flakey 和 dm-logwrites 批次仍未执行，Phase 2 尚未完成。

## 2026-07-21：dm-flakey 与 dm-logwrites 进展（未完成）

### JBD2 durability 与 replay 后内存状态

- 文件和目录 `fsync` 现在先同步 data/inode，再等待 JBD2 commit；活跃日志下不再
  直接写回 block-device metadata mapping，避免 home metadata 越过 commit record；
- `syncfs`/`sync_fs(wait=1)` 同样以 JBD2 commit 作为 metadata durability 边界；
- `jbd2_journal_load()` replay 发生在 mount 期间。replay 后重新从块设备 folio
  载入 inode/block free bitmap，重新排除 journal 区并计算 free counts，最后才从
  replay 后的 inode table 创建 root inode；这修复了 replay 后内存位图仍是旧副本
  导致的 double free 和重复 revoke；
- 上述修复后，早期失败的 dm-flakey 用例 034、039、040、041、056、057、059、
  065、066、073、321 均已定向 PASS。

### dm-flakey 批次

扫描到的 62 个 generic dm-flakey 用例已经全部执行：

- PASS 58；
- NOTRUN 4：501、546、588 因 SimpleFS 不支持 reflink，745 因测试要求单 inode
  支持超过 1000 个 xattr；
- FAIL 0，TIMEOUT 0。

### dm-logwrites 环境与首轮结果

- 运行内核原配置没有 `CONFIG_DM_THIN_PROVISIONING` 和
  `CONFIG_DM_LOG_WRITES`。已从运行内核精确配置和相同提交
  `d41bd6abfe34` 构建并加载 `dm-thin-pool`、`dm-log-writes` 及依赖模块；
- xfstests harness 新增独立 `LOGWRITES_DEV=/dev/loop202`，并在 VM 重启后加载
  dm-thin-pool/dm-log-writes；shellcheck 和 shfmt 检查通过；
- 470 因 SimpleFS 不支持 DAX 合法 NOTRUN；
- 455 首轮的直接失败原因是 SimpleFS 拒绝测试强制传入的 `-o discard`；后续已
  实现真实 discard 并进入内容 replay 调试，详见下节；
- 482 的全部 FUA replay/check 循环已执行，xfstests 最终因
  `dm_log_writes` 的 `log-write` kthread 退出时触发 refcount underflow warning 而
  判 FAIL；告警栈位于 dm-log-writes/kthread，不在 SimpleFS，但仍按 FAIL 保留；
- 757 的 100 次日志前缀回放已完成，最终同样因 dm-log-writes 模块 warning 判
  FAIL，文件系统 replay/check 循环本身已执行完毕。

整个过程中继续执行边界扫描：除 `simplefs_journal.c` 外，SimpleFS 核心没有
`buffer_head` 头文件、类型或 API 依赖。

## 2026-07-21：dm-logwrites 深入定位（未完成）

- 757 已完成 100 次前缀 replay/check；和 482 一样，文件系统检查循环完成，最终
  仅因 `dm_log_writes` 的 `log-write` kthread 退出触发 refcount underflow warning
  被 wrapper 判 FAIL；
- 455 的并发 mkdir 曾在嵌套 bitmap 分配中重复启动 adapter handle，形成同任务
  自锁。JBD2 adapter 现显式记录 handle owner/active handle，同任务嵌套加入外层
  transaction；
- `-o discard` 已从参数兼容扩展为真实在线 discard：free 只登记 pending，块重新
  分配时取消旧生命周期的 pending；discard 延迟到 fsync/syncfs durability boundary，
  在 metadata commit 后和最终 device flush 前下发；
- discard 必须服从设备 granularity。455 的 dm-thin cluster 是 64 KiB；原先直接
  下发 4 KiB discard 会稳定产生交替数据块归零。现在只下发向内对齐、整个单元
  仍 free 且 pending 的 64 KiB 整数区间；日志确认不再出现小粒度 discard；
- 并发 fsync 可能由另一线程提前提交共享 transaction。adapter 的 force-commit
  即使没有新 transaction 也会补一次 block-device flush，保证当前文件的数据写回
  位于 logwrites mark 之前；
- buffered unwritten ioend 转换增加 per-inode 计数和等待，fsync、truncate、punch、
  zero、collapse、insert 在改变 extent 前等待异步转换完成；
- ext4 在相同 dm-thin/dm-log-writes 栈和 455 固定 workload 下内容 replay 全通过，
  但同样触发 dm-log-writes refcount warning，证明该 warning 是独立测试模块问题；
- 455 的 11-op 并发最小批次已经通过所有内容 replay；200-op 固定 seed 已消除早期
  小粒度 discard 损坏；后续结论见下节。

边界扫描约束不变：只有 `simplefs_journal.c` 的 JBD2 adapter 可以依赖
`buffer_head`，SimpleFS 核心保持 folio/iomap 路径。

## 2026-07-21：generic/455 内容回放修复

继续缩减 455 后确认，mark7/INSERT_RANGE 不是独立根因。单文件 45-op 固定种子可在
mark2 稳定复现：回放前日志中物理块 1329 的最后一条数据记录与期望内容一致，独立
回放到全新裸镜像也逐字节一致，但把同一镜像挂载为 SimpleFS 后该块立即变为全零。
在 dm-thin volume 上再叠一层只用于取证的 dm-log-writes，记录到 mount/recovery
阶段由 JBD2 向数据块 1329 回写了旧零映像。

解析 JBD2 日志后发现：revoke 正确包含旧 extent leaf 1328、1330，但 descriptor
同时错误包含普通文件数据块 1329、1331。根因是
`simplefs_zero_file_block_tail()` 绕过 inode mapping，直接修改 block-device folio，
再调用 `simplefs_journal_dirty_folio()`，把文件尾数据误当成 metadata 加入 JBD2。
后续 buffered write 只走正常 iomap 数据路径，不会更新这条错误的 metadata log；
crash recovery 因而用旧零块覆盖已经正确持久化的新数据。

修复删除了这条重复的 bdev/JBD2 尾块清零路径。两个调用点此前已经先调用
`iomap_truncate_page()`，它本身就是通过 inode mapping 把 EOF 到块尾清零，所以不需要
再次通过 metadata adapter 清零。修复后：

- 单文件 45-op 固定种子的 mark0、mark1、mark2 和最终 unmount 内容全部通过；
- 4 文件 × 200-op 固定种子（7609/7611/7612/7613）的所有 mark 和最终内容通过；
- 标准 455 再随机得到种子 42182/42181/42183/42184，所有 mark 和最终内容通过；
- `generic/009`（`FALLOC_FL_ZERO_RANGE`）PASS，确认尾块/zero-range 语义没有回退；
- 455 的 xfstests 最终状态仍显示 FAIL，但唯一原因是与 ext4 对照相同的
  dm-log-writes kthread refcount-underflow warning；SimpleFS 内容检查已无失败。

对 warning 又做了两个不经过 SimpleFS 的最小实验：

- 只创建并立即删除一个空 dm-log-writes mapping，不挂载文件系统且不发数据 I/O，
  `log-write` 线程仍在 `release_task()` 触发同一 refcount underflow；
- 从同一内核源码和配置构建内核自带的 `preemptirq_delay_test`，其标准
  `kthread_run()`/`kthread_stop()` 生命周期也让 `irq_test` 线程在同一
  `release_task+0x43e` 位置触发告警。

因此该 warning 是运行内核 `7.1.2-00001-gd41bd6abfe34` 的通用 kthread 退出问题，
并非 dm-log-writes I/O、ext4 或 SimpleFS 触发。当前不在 harness 中过滤告警；
455/482/757 仍如实记录 wrapper FAIL，同时单独记录其文件系统 replay/check 已完成。

普通文件数据继续只走 folio/iomap。实际 `buffer_head` 类型、头文件和 API 的边界扫描
仍只命中 `simplefs_journal.c` 的 JBD2 adapter。

## 2026-07-22：journal 全量 generic 首轮与失败收敛（进行中）

首次全量发现运行已覆盖 001--787，三个结果目录合并为 PASS 424 /
NOTRUN 355 / FAIL 8 / TIMEOUT 0。所有大型 fsx、inode 耗尽、碎片 punch/zero、
device-removal 和 O_TMPFILE 压力用例均完成。8 个 FAIL 的精确分类是：

- 013、017、032、037：被同时运行的 `preemptirq_delay_test` warning 污染，
  用例本身没有输出错误；
- 455、482、757：文件系统内容/replay 检查完成，仅因 dm-log-writes
  `log-write` kthread 在 `release_task()` 中触发 refcount warning；
- 042：唯一真实的 SimpleFS 输出失败。

042 的根因不是 4 KiB write/speculative preallocation。该用例创建 25 MiB（6400
块）内层文件系统，mkfs 中原有“4 MiB JBD2 日志不得超过文件系统 10%”
的人为限制，会悄然把该镜像格式化成无日志 SimpleFS。trace 中因此完全没有
journal transaction，`godown -f` 也无 log 可提交。mkfs 现在仅当“固定元数据 +
一个根数据块 + JBD2 最小 1024 块”确实放不下时才禁用日志。重新格式化
25 MiB 镜像后，`generic/042` 三轮 LOGFLUSH recovery 已 PASS。

dm-log-writes warning 也已找到环境根因：运行内核的精确配置禁用
`CONFIG_AUDIT`，而先前补建 dm-log-writes/dm-thin 模块的配置启用了它，导致
外部模块中内联的 `task_struct` 字段偏移与运行内核不同。kprobe 确认
`log-write` 进入 `release_task()` 时 `rcu_users` 已被错位访问破坏。使用运行
内核配置和相同提交 `d41bd6abfe34` 重建并替换 dm-log-writes、dm-thin-pool
及依赖后，455 已干净 PASS。不再需要在 harness 中过滤该 warning。

干净 VM 中对原 8 个 FAIL 的当次分组重跑，013、017、032、037、042、455、482
已经 PASS；757 在写入结果前被外部中断，因此当时没有把它记作 PASS。该历史重跑
随后由 2026-07-26 的统一环境全量运行取代，最终全量中的 757 已完成全部 100 轮
FUA 回放并 PASS。

边界约束不变：SimpleFS 核心保持 folio/iomap，只有 `simplefs_journal.c` 的
JBD2 adapter 可以使用 `buffer_head`。

## 2026-07-26：Phase 2 最终验收（完成）

最终验收使用一次连续、统一环境的完整运行，不再合并早期批次：

- 运行内核为 `7.1.2-00001-gfb512e2a3eed #26`；
- SimpleFS 和 dm-log-writes 模块的 vermagic 均为
  `7.1.2-00001-gfb512e2a3eed SMP preempt mod_unload`；
- SimpleFS 模块 SHA-256 为
  `530c48ef6720438fc7e2bd4fa8014222c5f90a846ff92b9188d5d2eb4bc89656`，
  mkfs SHA-256 为
  `272a9b157b7ba553725b75fc4f47f2a74fb24bcb4db242f296cc682075a57227`；
- 内核切换后重新生成 initramfs，并按当前 `CONFIG_AUDIT=y` 配置构建
  dm-bufio、dm-bio-prison、dm-persistent-data、dm-thin-pool 和
  dm-log-writes，避免外部模块与 `task_struct` 布局再次不一致；
- 全量前 smoke 的 042、455、680 为 PASS 3 / FAIL 0 / TIMEOUT 0。

完整结果位于 VM：

```text
/home/martins3/mnt/simplefs-xfstests-phase2-fb512-full-20260726/
  xfstests_full_results.txt
  xfstests_full_logs/
```

机器校验结果：

```text
lines=787 unique_cases=787
counts=PASS:432 NOTRUN:355 FAIL:0 TIMEOUT:0
ordered_001_787=True
logs=787 exact_log_set=True
notrun_reasons=355/355 missing=0
```

大型和故障注入用例均包含在这次统一运行中。521/522 的百万次 fsx、551 的
100 轮直接 AIO 压力、680、730/731，以及 757 的完整 100 轮 FUA replay/check
均已 PASS。551 在 ext4 工作盘和隔离后台 XFS 访问源的环境中完成，没有 XFS
lockdep、OOM、hung task 或 SimpleFS 错误。

最终 dmesg 严格扫描没有 WARNING、BUG、Oops、general protection fault、
recursive-lock、refcount、hung-task、panic、KASAN 或 UBSAN，也没有 SimpleFS
corruption/assert/abort/error。故障注入用例按预期留下 106 条
`JBD2: I/O error when updating journal superblock`；这些是 dm-flakey/
dm-logwrites 主动注入 I/O 故障时的预期诊断，对应用例均由 xfstests 完整检查并
PASS，不能与非预期内核告警混为一谈。

源码边界扫描确认，`simplefs_journal.c` 之外没有 buffer_head 头文件、类型或 API
依赖；其他文件中仅有两处架构约束注释和一处 ioend 注释包含文字
`buffer_head`。普通文件数据继续走 folio/iomap，只有 JBD2 adapter 可以依赖
buffer_head。SimpleFS、mkfs 和精确配置的 DM 模块构建均通过，`git diff --check`
通过。

验收日志固定后，已经移除为 680 临时添加的 `fsgqa` home traverse ACL，恢复
containerd、polkit、系统维护 timers 和 root user tmpfiles timer，并删除已卸载且
loop 设备已分离的 `/var/tmp/simplefs-042-debug` 临时镜像目录。全量结果目录继续
保留。

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
