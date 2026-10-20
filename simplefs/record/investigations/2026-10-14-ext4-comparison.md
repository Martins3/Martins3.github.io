# SimpleFS 与 ext4 的基本对比（2026-10-14）

本轮要求：virtme 内存后端上的 Linux 源码树拷贝加 `sync` 总耗时低于 ext4 的
2 倍。最终三轮中位数为 **15.70 秒 / 8.66 秒 = 1.81 倍**，达到目标；另两组
基本负载也低于 2 倍。这只证明下面的配置和负载，不代表任意目录规模或存储后端。

## 结果

每组各跑三轮，第二轮交换文件系统执行顺序。每轮重新创建、格式化 8 GiB 镜像；
计时包含 `cp -a` 和随后 `sync -f`，不包含格式化和内容校验。单位为秒。

| 工作负载 | ext4 总耗时中位数 | SimpleFS 总耗时中位数 | 耗时倍数 |
| --- | ---: | ---: | ---: |
| Linux 源码树，约 1.8 GiB、101,060 个文件及目录 | 8.66 | 15.70 | 1.81 |
| 10,000 个 4 KiB 文件，100 个目录各 100 个文件 | 0.68 | 1.18 | 1.74 |
| 单个 512 MiB 文件 | 0.82 | 0.89 | 1.09 |

| 工作负载 | ext4 三轮总耗时 | SimpleFS 三轮总耗时 |
| --- | --- | --- |
| Linux 源码树 | 8.77 / 8.66 / 8.45 | 15.67 / 15.70 / 16.73 |
| 小文件 | 0.71 / 0.68 / 0.68 | 1.20 / 1.18 / 1.18 |
| 大文件 | 0.83 / 0.80 / 0.82 | 0.85 / 0.89 / 0.94 |

18 次拷贝正常退出，18 次卸载、重挂载后的完整内容比较均无差异。源码树使用
`tar -d` 对照原归档，其余使用 `diff -r`。目标统一放在挂载点下的 `payload/`，
因此 ext4 自带的 `lost+found` 不参与内容比较。

## 环境

- collei `virtme`：2 vCPU、8 GiB RAM、无 swap，卸载 guest balloon 驱动固定内存。
- 内核 `7.2.0-00001-geb5a10dc0e00`，保留 lockdep，同一 VM 中交替测试。
- 源目录和目标镜像都在 guest 私有 tmpfs；两种文件系统均为 4 KiB block、启用
  journal、默认挂载选项。没有关闭 barrier、flush 或数据同步。
- ext4 的 `lazy_itable_init=0,lazy_journal_init=0` 在计时前完成格式初始化。
- Linux 输入沿用 `git archive`，提交
  `eb5a10dc0e0026d8144f904b4b98b742a335a1d9`，排除 `.git` 和构建产物。
- 小文件使用确定性的 4 KiB 字节序列；大文件取归档前 512 MiB，实际写入数据，
  不用全零稀疏文件替代拷贝。

最终模块 SHA256：`275900c1c37f2feadbe3a79f8babf3fd28d70ab006b261fede8de05e8d4933c2`。
mkfs SHA256：`2e1e3879eec0d8e6ca53acad6afa7590b7472fb35a9bbb2eaebc486f8e6ba911`。
两者已核对 guest 与宿主一致。virtiofs 曾缓存旧 `.ko`，所以实验使用不同名称的
不可变产物，并把正确版本 bind mount 到 guest 的项目模块路径。

## 优化内容与边界

本轮起点已包含首次写入按需分配优化，但初次对照仍为 SimpleFS 21.93 秒、ext4
9.48 秒，约 2.31 倍。旧路径在整块初始化元数据前先读取旧盘面，源码树拷贝多读
约 1 GiB；只跳过这些读取后，中位数仍是 17.52 秒对 8.30 秒，约 2.11 倍。

最终改动：

1. `simplefs_get_zeroed_folio()` 为立即整块覆盖的元数据提供入口：先取得 JBD2
   write access，再清零、设置 buffer uptodate，普通 folio 映射即可避免读盘。
   只用于新 inode 根、新目录块和重建的 extent leaf；inode table 等局部更新
   继续保留读旧内容的路径。
2. `simplefs_create_internal()` 不再重复初始化和记录 `simplefs_new_inode()`
   已经处理过的根块；整块覆盖入口不再重复申请相同事务的 write access。
3. `simplefs_journal_prepare_new_blocks()` 在支持的内核上只查询已有 buffer，
   不为每个新文件数据块创建 bdev 缓存。可睡眠查询会等待 buffer migration，
   不把原子查询在 `BH_Migrate` 期间的暂时缺失当作可复用；旧内核保留原路径。

原有 revoke、事务归属和位图锁约束保持有效，`buffer_head` 仍只在 journal 适配层。
数据块同步清零、unwritten 转换、fsync 和提交语义没有为测速而削弱。统一采用
unwritten 分配、取消同步清零的试验没有改善总耗时，已撤回，不在最终代码中。

## 基本正确性验证

按本轮要求做基本测试和对比，**没有重新跑 001–787 全量回归**。

- 最终模块的 `smoke.sh`、`name-max.sh`：PASS。
- 默认 journal 和 `nojournal`：mmap/msync、硬链接、rename、32 轮创建/删除/同步
  后的块复用、xattr、重挂载读回，均 PASS。
- 定向 xfstests：013、039、050、075、083、095、730，7 PASS，0 FAIL，0 TIMEOUT。
- 未观察到 WARNING、BUG、Oops、panic、KASAN 或 UBSAN。730 产生预期的设备和
  journal 错误日志，不能据此声称 dmesg 完全没有错误信息。

恢复旧测试盘发现，9 月的全量在 755 项处中断，730 曾失败。当前版本也能复现：
设备移除后仍读到缓存数据。原因是未注册 VFS 的 `super_operations.shutdown`，
只依赖未来的写回错误触发 shutdown。新增 `simplefs_shutdown_sb()` 后立即设置
状态并终止 journal，730 已实际重跑通过，测试正文和预期输出未修改。

## 复查入口

本机目录：`/home/martins3/data/simplefs-ext4-20261014/`。

- `compare-final.sh`：三组、三轮交替对比的完整命令。
- `results-final/comparison.csv`：每轮 copy、sync 和总耗时。
- `results-final/summary.json`：自动计算的中位数和比值。
- `results-final/final-*-copy.txt`、`*-sync.txt`、`*-verify.txt`：原始证据。
- `results-final.tar.gz`、`basic-final.tar.gz`、`dmesg-final.txt`：日志归档。
- `basic-extra.sh`、`mmap-test.out`：额外基本测试及其程序。
- `final-v5.ko`：实际测试模块；其他模块是保留的中间实验产物。

当前 VM 的源数据位于 `/var/tmp/copy-bench/{source,tiny,large}`。复现实验需新的
结果标签或工作目录，脚本会拒绝覆盖已有轮次的计时文件。VM 重启后需重新准备
tmpfs 并解包源归档，不能把宿主 virtiofs 路径直接当成内存源目录。
