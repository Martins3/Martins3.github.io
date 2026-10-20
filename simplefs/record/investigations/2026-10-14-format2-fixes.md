# SimpleFS format 2：修复与 ext4 实测

完整 NFS 工作树的三轮 `cp -r`＋`sync` 中位数：**SimpleFS 174.89 秒，ext4
135.01 秒，倍率 1.295**，达到“不超过 ext4 两倍”的要求。8 次拷贝（含独立采样）
全部成功，同步成功，卸载重挂后的路径、文件大小、符号链接目标和 SHA-256 全部一致。
源目录在整批测试前后也一致。

2026-10-15 完成 generic/001--787 覆盖：**440 PASS / 347 NOTRUN / 0 FAIL /
0 TIMEOUT**。结果由两个有效启动区段合并，二者使用相同的 SimpleFS 模块和 mkfs，
lockdep 均保持启用；具体中断原因与原始失败记录见末节。

## 负载、条件与计时

原始命令保持为：

```bash
cp -r /home/martins3/data/kernel/opencloudos
```

输入含 `.git`、`.ccls-cache` 和构建产物，共 213,175 个普通文件、7,771 个目录，
普通文件逻辑字节数 23,256,897,052。没有排除导致原先失败的大目录。

- VM：nix-replace-kernel，32 vCPU，8 GiB 配置内存；每轮 MemTotal 7,543,016 KiB，
  Balloon 为 0；保留 lockdep/debug 配置。
- 内核：`7.2.0-simplefs-test-00001-geb5a10dc0e00`；镜像、模块和 initramfs 配套构建。
- 源路径是 NFSv4.1；两块目的盘都是 40 GiB QEMU SCSI 盘，挂载点为
  `/mnt/simplefs`、`/mnt/ext4`，每轮重新格式化。cp 以 martins3 身份执行。
- 每轮先同步并清 guest 缓存，不清 NFS 服务端缓存；轮次交替，属于用户要求的
  实际环境粗略对照。两种文件系统都使用默认 journal 设置。
- 三轮正式计时不带 perf；另各跑一轮 `perf record -e cpu-clock -F 99 -g`。
  内容校验、格式化、卸载和重挂均不计入 cp＋sync 时间。

| 轮次 | ext4 cp | ext4 sync | ext4 合计 | SimpleFS cp | SimpleFS sync | SimpleFS 合计 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 132.81 | 0.56 | 133.37 | 174.33 | 0.56 | 174.89 |
| 2（先 SimpleFS） | 134.46 | 0.55 | 135.01 | 173.46 | 0.56 | 174.02 |
| 3 | 134.51 | 0.67 | 135.18 | 194.26 | 0.66 | 194.92 |
| **中位数** | | | **135.01** | | | **174.89** |

单位为秒。三个配对倍率约 1.31、1.29、1.44，均低于 2；中位数比值为 1.295。
cp 的 user＋system CPU 中位数为 **64.35 秒对 68.36 秒，倍率 1.062**。
独立采样轮 cp＋sync 为 ext4 137.41 秒、SimpleFS 188.13 秒，不混入正式中位数。

原来的 283.27 秒尝试缺少 4,352 个文件且 sync 失败，还存在内核与构建产物不一致，
不能把旧时间与本轮直接相除作为严格加速比。

## CPU 花在哪里

下面是独立采样轮 cp 的 inclusive 百分比，包含子调用，父子项不能相加。
两份 perf 报告都没有丢失样本。

| 路径 | ext4 | SimpleFS |
| --- | ---: | ---: |
| `ksys_write`：写入 | 43.95% | 36.39% |
| `ksys_read`：主要是 NFS 读取 | 26.05% | 23.83% |
| `__x64_sys_openat`：打开与创建 | 15.53% | 22.58% |

SimpleFS 的目录查找已经不再主导 CPU：`simplefs_lookup()` 为 **0.39%**，
`simplefs_dir_find()` 为 **0.14%**，`simplefs_dir_set()` 为 0.56%。
创建路径仍有成本：`simplefs_create_internal()` 为 12.81%，其中
`simplefs_new_inode()` 为 8.11%；`simplefs_dir_add()` 为 3.48%。

剩余分配器成本也可见：`__simplefs_get_free_blocks()` 的两个编译实例分别为
6.01% 和 3.69% inclusive，`_find_next_bit` self 为 7.23%。
`simplefs_file_normalize_extents()` self 为 1.80%。

`lock_acquire`、`lock_release`、`lock_is_held_type` 的 self 合计：SimpleFS
41.47%、ext4 49.89%。这是启用 lockdep 的内核，不能把这些样本解释成锁等待时间。
CPU 采样只覆盖 cp 进程链，不是整机统计，也没有采集 off-CPU 等待；不能仅凭剩余
墙钟差值把问题归因于 NFS 或 journal 等待。

## 空间核算

ext4 与 SimpleFS 的 `df used` 是否包含固定元数据的口径不同。主表统一使用
**设备总字节数减实际 free 字节数**，不把 ext4 的 root 保留空间当成已用空间。

| 指标 | ext4 | SimpleFS |
| --- | ---: | ---: |
| 设备实际已用 | 24,778,080,256 B / 23.0764 GiB | 25,013,215,232 B / 23.2954 GiB |
| 拷贝树 `du` | 23,783,186,432 B | 24,732,045,312 B |
| 实际 free | 18,171,592,704 B | 17,936,457,728 B |
| 普通用户 available | 16,007,331,840 B | 17,936,457,728 B |

SimpleFS 实际已用比 ext4 多 **235,134,976 字节，约 224.24 MiB / 0.95%**。
树本身的 `du` 多约 3.99%，但 SimpleFS 的固定 inode table 较小。ext4 默认保留空间
造成 free 与 available 的差异，不能据此声称它消耗了更多文件数据空间。

SimpleFS 的占用可以逐项解释：

```text
copied tree du       24,732,045,312
fixed metadata          281,161,728
filesystem root               8,192
total physical used  25,013,215,232
```

读取磁盘位图逐位核对：free blocks 的计数和位数同为 **4,379,018**，free inodes
同为 **2,400,418**，没有本次快照无法解释的容量损失。

40 GiB 空卷固定元数据由旧布局 1,108,099,072 字节降为 281,161,728 字节。
默认 inode 密度改为 16 KiB/inode，并提供 `-i bytes-per-inode`；普通少量 extent
文件通常省掉一个 4 KiB leaf，小目录按需逐块分配。

## 实现与正确性验证

- `simplefs_sync_fs()` 在 journal 模式等待 inode 写回和日志提交，不再把整个位图
  放入一个事务。分配/释放只记录改变的位图块及计数，持久化错误中止日志并使后续
  操作返回错误。nojournal 在 inode 写回后保存位图。
- 目录共用可扩展 extent 映射，名称哈希、空槽位图和计数按 inode 缓存，重挂后由
  实际目录项重建。删除保持逻辑槽号，空目录在仍可达时分批退役映射。
- extent 根直接保存最多 340 个 extent，超出后扩展为 root＋leaf，缩小后可退回
  直接根；先发布持久映射，再释放旧块。nojournal 先写回叶块再发布根。
- mkfs 修复重复应用首字保留掩码导致的额外 1,022 个不可用 inode，并检查密度与
  32 位容量计算边界。磁盘 magic 为 `0xDEADCE02`，旧格式需重做；statfs 类型标识
  仍为 `0xDEADCE`。
- `simplefs_file_splice_read()` 补齐 shutdown 检查。独立探针曾复现普通 read 返回
  EIO、sendfile 却读出缓存的 1 MiB；修复后两条路径均为 EIO，generic/730 通过。
- 移除 generic/558 的分目录适配，恢复上游单目录并发耗尽 inode 的负载。

匹配新内核后的手工验证已完成：40 GiB journal 下创建 100,000 个有内容文件，
重挂逐个验证、删除复用槽位、硬链接、rename/exchange、清空并 rmdir；20 GiB
nojournal 下重复 1,000 项。400 个相隔 1 MiB 的稀疏写入跨越根容量，重挂检查内容
和洞，再收缩并确认回到根块＋一个数据块的 16 sector。没有新增 WARN。

定向回归 011、013、035、042、074、558 通过；730 在补齐 splice 检查后通过。
Ruff、ty 和 Neovim 中 ruff/ty diagnostics 均为零；ShellCheck、shfmt、构建和
`git diff --check` 通过。全量覆盖的逐例结果为 `final-generic-results.txt`，统计与
告警审计为 `final-generic-summary.json`；完整 787 个编号无缺失、无重复。

## 环境配套与复现

原 VM 的版本字符串不能证明 ABI 匹配：运行内核 BTF 的 `struct module.exit`
偏移为 `0x500`，源码构建模块为 `0x518`，导致零引用模块仍无法卸载；宿主配置还
存在 ORC 与 frame pointer/KGDB 差异。最终在独立工作区从同一源码构建完整内核、
模块、initramfs，并使用唯一 `-simplefs-test` 后缀。新环境中模块加载/卸载正常。

模块构建命令：

```bash
KDIR=/home/martins3/data/linux-simplefs-vm-test ./build.sh
```

VM 的旧配置备份为原始证据目录中的 `vm-config-original.ini`。测试盘通过固定 PCI
设备路径识别；启动盘 vda 没有被格式化。xfstests 在 VM 的私有副本中使用临时 Nix
环境重新构建，避免复用指向宿主库路径的旧 ELF；runner 补齐首次创建挂载目录。

原始证据：`/home/martins3/data/simplefs-fixes-20261014/`。

- `comparison-final/`：8 次计时、sync、完整校验清单、perf 原始数据及报告；`exit=0`。
- `results/timing-summary.json`、`capacity-summary.json`、`directory-profile.txt`：汇总。
- `compare.sh`、`manual-final.sh`、`full-after-comparison.sh`：实际验证流程。
- `final-regressions.log`、`resume551-regressions.log`：回归日志；逐例结果和原始
  dmesg 一并保留，早期中止的运行不能算作最终通过。
- `measured-build/`：与本次性能测量 SHA-256 一致的模块和 mkfs。
- `final-build.sha256`、`final-kernel.config`、`kernel-release`：构建身份。
- `simplefs-full-xfs-interference.out`：包含有效 001--550 以及随后被干扰的原始运行。
- `simplefs-final-551-787.out`、`simplefs-final-metadata.out`：后半程原始日志、
  dmesg、构建标识与 lockdep 状态。

## 全量回归中的环境修正

第一次全量运行发现三项环境问题，原始日志保存在 `simplefs-full-initial.out`：

- generic/010 的旧 dbtest ELF 指向缺失的宿主 glibc。补齐 GDBM 依赖、在 VM 中
  重建后，真实数据库测试通过；没有将它改成 NOTRUN。
- generic/452 把 ls 复制并改名执行；Nix 多调用 coreutils 按 argv[0] 分派，改名后
  报 unknown program。测试环境的私有 PATH 改用 VM 原生独立 ls，原测试通过。
- generic/459 触发 dm-thin 的 pool_map 锁状态告警。无 SimpleFS 模块的 ext4 对照
  也复现了告警：持有 pool->lock 调用 bio_set_dev，在 blkcg 慢路径重新开启 IRQ。
  隔离内核工作区移除该处不必要的 pool 锁，目标持有 data_dev 的生命周期不变。
  修复后的 ext4 对照与 SimpleFS generic/459 均通过，debug_locks 保持 1。

该 dm-thin 补丁单独保存为 `dm-thin-pool-locking.patch`，并已更新测试 VM 的模块与
initramfs。性能测试使用物理 SCSI 目的盘，没有经过 dm-thin；内核镜像、SimpleFS
模块和 mkfs 的 SHA-256 在此修正前后完全相同，已保存校验结果。

lockdep 告警不一定改变 taint 的 WARN 位；本次告警让 debug_locks 变为 0，所以
第一轮在 522 附近被中止，不能作为完整锁检查通过的证据。环境修正后重新启动了
001--787 全量运行，日志为 `final-regressions.log`。

第二轮的 001--550 为 336 PASS / 214 NOTRUN，期间 lockdep 保持启用。551 的随机
AIO 数据验证没有输出错误，但定时启动的 `updatedb` 在根 XFS 上触发递归锁告警，
因此该次 551 仍按 FAIL 保留。栈为 `xfs_readdir()` → buffer 分配 → 内存回收 →
`xfs_icwalk_ag()`，与既有 [XFS 独立复现](xfs-lockdep-repro.md) 相同；没有经过
SimpleFS。原始结果及告警保存在 `simplefs-full-xfs-interference.out`，没有过滤告警。

重启恢复 lockdep 后，临时暂停 updatedb、tmpfiles、dnf 定时维护和相关后台服务，
保持测试体、随机轮数、VM 配置及 SimpleFS 二进制不变，从 551 重跑至 787，日志
为 `resume551-regressions.log`。后半程为 104 PASS / 133 NOTRUN / 0 FAIL /
0 TIMEOUT，551、558、730 均 PASS。合并前半程得到 440 PASS / 347 NOTRUN，
这是两段完整覆盖，不是一次不间断运行。有效区段的逐例 dmesg 与重启后完整 dmesg
未发现 WARNING、Oops、panic、hung-task 或 OOM；最终 `debug_locks=1`。

外层续跑脚本在运行期间被格式化，改变了 shell 读取位置；测试 runner 完整输出
237 项汇总之后，外层误执行残余的 `c` 命令并以 127 退出。该退出码和日志原样保存，
没有当成 runner 的成功退出码；上面的测试结论来自逐例结果、完整汇总及 dmesg 的
独立核验。后续不能编辑仍在运行的脚本文件。

测试结束后已恢复后台服务和 balloon 采样策略，重新挂载 `/mnt/simplefs` 与
`/mnt/ext4`，两边保留完整工作树，各有 213,175 个普通文件，便于继续检查。
