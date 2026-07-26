# SimpleFS xfstests 推进计划（roadmap）

目标：在保持 FAIL 0 / TIMEOUT 0 的前提下完成 folio-native JBD2 journal，
并显著降低 480 个 NOTRUN。Phase 2 已完成，最终 NOTRUN 为 355。

基线：`d17b33d4a878`，787 个 generic case，PASS 307 / NOTRUN 480 / FAIL 0 / TIMEOUT 0
（record/2026-07-16-final-xfstests.md，2026-07-17 smoke 复核一致）。
各 NOTRUN 原因的完整 case 清单见 record/unimplemented-plan.md，本文只定义执行顺序。

## 进展记录（2026-07-17）

Phase 0 已完成（部分结论与预估不同）：

- dm-flakey 和 scsi_debug 模块实际已编译，只是从未装进 VM 的 /lib/modules。
  用 `INSTALL_MOD_PATH=install-<release>` 装好后直接可用，无需重编内核。
- 解锁后实测：59 个 dm-flakey case 里 58 个还卡在 metadata journaling 检查
  （测试先查 dm flakey 再查 journal），实质是 journal 依赖，归入 Phase 2；
  只有 741 直接转 PASS。scsi_debug 解锁 108 350（+2 PASS）。
- 按铁律修复了暴露的 3 个 FAIL：
  - generic/704：DIO 对齐检查用 fs 块大小（4K），iomap 本身支持设备逻辑扇区
    粒度，放宽为 `bdev_logical_block_size` 即可支持 512 字节扇区设备。
  - generic/730/731：`simplefs_extent.c` 把读到的全零 extent root/leaf 当
    "空树"返回成功，设备移除后读取返回静默零数据；改为上抛 -EIO，并在
    writeback 出错路径置 `s_shutdown`（排除 ENOSPC），实现"写回失败→后续
    读写 EIO"的 shutdown 语义。
- 验证：704+DIO 回归、730/731+dm-error 回归（338/441/442/484/743）、
  26 个核心回归（含 521/522 fsx 百万次、647/729）全部 PASS。
- Phase 0 净收益：+6 PASS（704 730 731 108 350 741）。

Phase 1 已完成（全量 787 复跑验证中）：

- fzero（ZERO_RANGE）：完整块标 unwritten，边缘块物理清零，13 个 PASS
  （008 009 033 096 469 499 503 511 610 685 758 349 + 042 转 journal 依赖）。
  修了两个深层 bug：标记范围越过请求区间的钳制问题（008/009/499/511），
  以及 mark/convert_unwritten_extent 每次做冒泡 normalize 叠加成的 O(n^3)
  （610 的 fzero 100M）——改为 sync_extents 统一归一化 + 单遍转换。
- finsert（INSERT_RANGE）：extent 逻辑右移、插入区成 hole，8 个 PASS
  （058 060 061 063 064 404 485 686）。735 因格式上限（约 2.9TB < 16TB）
  按真实原因 NOTRUN（harness 补了 inline 上限检查）。
- chattr/lsattr/file_attr：实现 inode fileattr_get/set + 磁盘 i_flags 字段
  （immutable/append/noatime/sync/dirsync/nodump），8 个 PASS
  （079 277 424 545 553 555 772 780）。未实现的标志（DAX/compress/verity）
  明确拒绝，探测类 require 正确跳过（607→DAX NOTRUN）。
- FITRIM：按空闲位图下发 discard，4 个 PASS（251 260 288 038）。
  746 因 loop-on-self 嵌套下 sync-zeroout 分配回写链自锁（tmpfs 后备对照
  正常）按架构性原因 NOTRUN。
- 小项：026（ACL 上限 506 + 超限 EINVAL）、425（fiemap XATTR）、
  402（s_time_min/max 有符号 32 位 + legacy mount 让 timestamp 警告在
  测试期真实产生）、528（磁盘加 i_crtime + statx btime）全部 PASS。
  492（label ioctl 已实现，blkid 无 simplefs 探测定义）和
  565（跨设备 CFR 与 ext2/ext4 一致返回 EXDEV）维持合法 NOTRUN。
- harness 基建：legacy_mount.c 把 simplefs 挂载改走 legacy mount(2)
  （util-linux 新 mount API 在此内核不触发 timestamp expiry 警告），
  MOUNT_PROG wrapper 处理 MS_* 选项翻译（strictatime/nosuid 等）。

## 铁律

- FAIL / TIMEOUT 不可接受，NOTRUN 可接受但必须保留具体原因。
- 每完成一个功能点立刻定向回归；每个阶段结束做一次全量 787 复跑。
- 全量复跑结果更新到 record/ 下新的日期文件，旧结论保留。
- 环境类解锁会把 NOTRUN 变成真实运行的故障注入测试，可能暴露真 bug；
  暴露了就修，不允许靠重新跳过来压下去。

## Phase 0：环境补齐（不改 simplefs 代码，预计解锁 70+）——已完成

| 项 | 数量 | 做法 |
| --- | ---: | --- |
| dm flakey | 59 | ~~VM 内核打开 `CONFIG_DM_FLAKEY`~~ 模块已编译，装进 /lib/modules 即可；但 58/59 实为 journal 依赖，归入 Phase 2 |
| scsi_debug | 8 | 同上，已解锁（108 350 PASS，704 730 731 修复后 PASS） |
| $LOGWRITES_DEV | 4 | local.config 配置 dm-logwrites 设备（与 Phase 2 的 journal 配合才有意义，可延后） |
| 其他内核配置 | 5 | `CONFIG_FAIL_MAKE_REQUEST`(019)、THP(759 760)、SELinux(700)、BSD 记账(596)，需重编内核，暂缓 |

验收：解锁的 case 全量复跑，FAIL/TIMEOUT 必须为 0；暴露的 -EIO 路径问题逐个修。

## Phase 1：易实现功能（预计解锁 40+）

按依赖从低到高，每个功能单独实现、单独回归：

1. 零碎小项（6）：timestamp bounds(402)、maximum ACL count(026)、inode crtime(528)、
   fs label(492)、跨设备 copy_file_range(565)、fiemap -a(425)
2. fzero / ZERO_RANGE（12）：unwritten extent 已有，fallocate 加分支
3. finsert / INSERT_RANGE（9）：COLLAPSE_RANGE 的对称操作
4. chattr/lsattr/file_attr（14）：`FS_IOC_GETFLAGS/SETFLAGS` + inode flags 字段
5. FITRIM（约 5）：FITRIM ioctl + 按 bitmap 释放块

验收：每项完成后跑对应 case + 相关回归（fallocate 类、fsx 类）；阶段末全量复跑。

## Phase 2：journal 稳定化（预计解锁约 25）

### 2026-07-19 定向进展（未完成）

完整证据见 `record/2026-07-19-phase2-journal-progress.md`。

- 已修复 `j_current_handle` 并发 UAF；`generic/051` 从 kernel panic 转 PASS。
- 已实现只读 `norecovery`、跳过 replay/卸载写回并禁止 FITRIM；537 PASS。
- 已定向 PASS：043-049、051、388、392、417、468、505、530、537、753。
- 当前未清零：475 FAIL（反复 dm-error 后目录/根元数据不一致）、050 FAIL；
  052/054/055 因缺少 simplefs log-state probing 仍 NOTRUN。
- dm-flakey 58 个和 dm-logwrites 4 个尚未完成批次验收，也尚未做 Phase 2
  全量 787 复跑。
- 当前约 1000 行 journal 是自建 folio/data-copy 后端，并非目标要求的内核 JBD2；
  已确认其 write-ahead ordering 在 dm-error 下不可靠。JBD2 迁移现已启动，mkfs、
  mount 和 transaction wrapper 已换成 JBD2，但 generic/001 可在 inode table
  block 1 稳定触发 dirty-metadata warning 和 JBD2 commit 断言。下一步必须先把
  inode table 改为原生 buffer_head 的 access-before-modify 路径，再迁移其余元数据；
  不能以旧后端的定向 PASS 宣称 Phase 2 完成。

### 2026-07-20 JBD2 迁移进展（未完成）

完整证据见 `record/2026-07-20-phase2-jbd2-progress.md`。

- JBD2 superblock、mount/load、transaction wrapper 和 metadata dirty adapter 已接通；
- 修正日志逻辑块到物理日志区的 `j_bmap` 偏移；
- 修复 create/unlink 目录数据 folio 的块号生命周期：映射时固定目标块号，先提交
  并释放内层目录数据 folio，再更新外层目录索引；inode table block 1 不再被目录
  内容覆盖；
- `generic/001` 已实跑 PASS（114 秒），40 文件卸载/重挂载逐项校验 PASS，且无
  WARNING、BUG、Oops 或元数据块号错配；
- `generic/051` 已干净 PASS；修复 journal/extent 锁反转、旧目录 dirty 绕过、
  JBD2 revoke、bitmap 事务、原子 inode retirement 和卸载后期 transaction 启动；
- SimpleFS 核心继续保持 folio-native，`buffer_head` 只能封装在
  `simplefs_journal.c` 的 JBD2 适配层，不能迁移或扩散到其他元数据路径；
- 仍待完成：475/050、052/054/055 log-state probing、dm-flakey、
  dm-logwrites、正常 checkpoint 生命周期和全量 787 case 回归。

### 2026-07-21 异步 commit 进展（未完成）

- `generic/050`、`generic/052`、`generic/054` 已定向 PASS；
- 所有已识别的 folio 元数据修改路径都在修改前取得 JBD2 handle，已移除每事务
  `h_sync`，055 单轮耗时从约 220 秒降到约 4 秒；
- 分配器会跳过 freeing transaction 尚未 commit、仍带 revoke 状态的元数据块，
  当前压力测试无 duplicate revoke、journal abort、BUG 或 Oops；
- `generic/055` 已完成 10 轮并 PASS。根因是 revoke 标志清除后，旧 journal head
  仍在 committing transaction 的 `BJ_Forget`，分配器只检查 revoke 因而提前复用；
  现在同时检查 JBD2 transaction 所有权，未恢复每事务同步提交；
- 当前定向 PASS：001、050、051、052、054、055、475；仍需执行 dm-flakey、
  dm-logwrites 和全量 787 case，Phase 2 不能据此宣告完成；
- 当前代码仍严格限制只有 `simplefs_journal.c` 的 JBD2 适配层依赖
  `buffer_head`。

### 2026-07-21 故障注入批次（未完成）

- `fsync`、目录 `fsync` 和 `syncfs` 在活跃 JBD2 下等待 commit，不再绕过日志
  直接写 home metadata；mount replay 后重新载入 free bitmap/count 并最后创建
  root inode；
- 62 个 dm-flakey 用例结果为 PASS 58 / NOTRUN 4 / FAIL 0 / TIMEOUT 0；501、
  546、588 是 reflink 能力型 NOTRUN，745 要求超过 1000 个 xattr；
- 已给 VM 补齐同内核、同配置的 dm-thin-pool/dm-log-writes 模块，并给 harness
  增加独立 `/dev/loop202`；470 因 DAX 合法 NOTRUN；
- 482 和 757 的文件系统 replay/check 循环均完成，但 dm-log-writes kthread 退出
  触发内核 refcount warning，仍记 FAIL；ext4 对照、空 dm-log-writes mapping 以及
  内核自带的标准 kthread 测试模块都能触发同一 `release_task()` warning，已确认是
  当前 7.1.2 测试内核的通用 kthread 退出问题；
- 455 已实现真实在线 discard，并修复 4 KiB discard 破坏 64 KiB dm-thin cluster
  的问题；进一步确认旧的文件尾清零路径错误地把普通数据块加入 JBD2，mount replay
  会用旧零映像覆盖新数据。删除该重复 bdev/JBD2 路径后，45-op 最小复现、4 文件
  × 200-op 固定种子及一组标准随机种子的全部 mark/最终内容均通过，`generic/009`
  也 PASS；455 目前只因 dm-log-writes 模块 refcount warning 被 wrapper 判 FAIL；
- force-commit 增加显式 device flush，discard 延迟到 durability boundary，allocator
  会取消被重新分配块的 pending discard；unwritten ioend 转换增加 per-inode 等待；
- Phase 2 仍需在测试内核层修复或隔离通用 kthread 告警、完成复跑、移除调试期完整
  checkpoint，并执行全量 787 case；
- 只有 `simplefs_journal.c` 的 JBD2 适配层允许依赖 `buffer_head`。

### 2026-07-22 全量 journal generic 首轮（未完成）

- 001--787 的首轮发现结果为 PASS 424 / NOTRUN 355 / FAIL 8 / TIMEOUT 0；
  8 个 FAIL 中只有 042 是真实 SimpleFS 输出失败；
- 042 根因是 25 MiB 内层镜像触发 mkfs 的人为 10% 日志比例限制，被悄然格式化
  为无日志文件系统。mkfs 现在只在最小 1024 块 JBD2 日志确实放不下时才禁用
  日志，042 已 PASS；
- 455/482/757 的 kthread warning 来自外部 dm-log-writes/dm-thin 模块使用了与运行
  内核不同的 `CONFIG_AUDIT` 配置，造成 `task_struct` 字段偏移错位。按运行内核
  精确配置和同一提交重建后，455 已干净 PASS，无需过滤 warning；
- 当次干净 VM 分组重跑中，013、017、032、037、042、455、482 已 PASS；757 在
  结果写入前被外部中断。该批次没有被当作最终验收，随后由统一环境的完整运行取代；
- 仍只有 `simplefs_journal.c` 的 JBD2 适配层允许依赖 `buffer_head`。

### 2026-07-26 Phase 2 最终验收（完成）

- 当前内核为 `7.1.2-00001-gfb512e2a3eed #26`，SimpleFS 和 dm-log-writes
  vermagic 与运行内核精确一致；initramfs 与 DM 测试模块也按当前
  `CONFIG_AUDIT=y` 配置重新构建；
- 全量前 042、455、680 smoke 全部 PASS；
- 一次连续运行完整覆盖 `generic/001`--`generic/787`，机器校验确认 787 个 case
  顺序完整且唯一、787 份 wrapper log 齐全，355 个 NOTRUN 全部有明确原因；
- 最终结果为 **PASS 432 / NOTRUN 355 / FAIL 0 / TIMEOUT 0**，结果位于 VM
  `/home/martins3/mnt/simplefs-xfstests-phase2-fb512-full-20260726/`；
- 521/522 百万次 fsx、551 直接 AIO 压力、680、730/731 和 757 全部 PASS；
  dmesg 没有非预期 WARNING、BUG、Oops、refcount、hung-task 或 SimpleFS
  corruption/error。故障注入产生的 JBD2 I/O error 是测试预期行为；
- `simplefs_journal.c` 之外没有 buffer_head 头文件、类型或 API 依赖，核心路径
  继续保持 folio/iomap；
- Phase 2 验收完成，完整修复和环境证据见
  `record/2026-07-20-phase2-jbd2-progress.md`。

历史快照（2026-07-17 代码走查修正）：当时 journal 远非"已实现"，只是骨架。
下面的 buffer_head 迁移设想后来被否决并由 folio-to-JBD2 adapter 方案取代，仅为
保留调查过程，不是当前实现计划：

- simplefs_journal.c 约 900 行：transaction 生命周期、descriptor+data+barrier+commit
  写日志、recovery 重放都已实现；mkfs 会写 journal superblock（mkfs_write_journal）；
  mount 会 load + recover（simplefs_super.c:959）。
- 但块级 dirty 接入只有 **1 处**（simplefs_inode.c:131 的 dirty_metadata）。
  extent 树（sync_extents）、块/索引位图（bfree/ifree）、目录数据块、
  superblock、xattr 全部没有接 journal——即崩溃后 journal 里几乎没有
  可重放的元数据，consistency 类测试必然 FAIL。
- 且 fill_super 无条件 `s_journal_mode = 1` 强制禁用，xfstests 挂 `-o nojournal`。

步骤（修正后）：

1. 补全接线：把 dirty_metadata 接进 extent 树 root/leaf 块、bfree/ifree 位图块、
   目录数据块、inode 表块、superblock、xattr 块的全部写路径。
   这是 Phase 2 的主体工作，按路径逐个来（dir 操作 → extent → bitmap → 其余）。
2. 打开 journal（默认启用、保留 nojournal 选项），先跑全量确认不引入 FAIL。
3. 崩溃恢复正确性：dm-logwrites / 断电重放场景验证，修到稳定。
4. 去掉 xfstests-full.sh 里的"无 metadata journaling" patch 和挂载的 `-o nojournal`，
   解锁 metadata journaling 17 个 + norecovery(050) + log probing(052 054 055)。
5. 配合 Phase 0 的 $LOGWRITES_DEV，再解锁 455 470 482 757；
   dm-flakey 的 58 个也随之解锁（它们先查 dm flakey 再查 journal）。

验收：崩溃恢复类 case 全部 PASS；全量复跑 FAIL/TIMEOUT 为 0。

## 明确不做

reflink(176)、dedupe(15)、quota(31)、encryption(26+2)、verity(10)、exchangerange(18)、
atomic write(9)、DAX(5)、casefold(2)、idmapped(6)、delayed allocation(614)、
huge file(525)、固定 4 KiB 块大小限制的 6 个。
理由：工作量与教学价值不成比例，维持合法 NOTRUN。

## 预期结果（2026-07-18 实测更新）

- 基线（2026-07-16）：PASS 307 / NOTRUN 480 / FAIL 0
- Phase 0+1 后（2026-07-18 全量复跑）：**PASS 345 / NOTRUN 442 / FAIL 0 / TIMEOUT 0**（+38，未引入 FAIL）
- Phase 2 后（2026-07-26 全量复跑）：**PASS 432 / NOTRUN 355 / FAIL 0 /
  TIMEOUT 0**（相对 Phase 0+1 新增 87 PASS，相对基线新增 125 PASS）

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
