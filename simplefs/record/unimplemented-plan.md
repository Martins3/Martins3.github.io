# SimpleFS NOTRUN 分布与后续功能计划（2026-07-17 更新）

基线：`d17b33d4a878`，787 个 generic case，PASS 307 / NOTRUN 480 / FAIL 0 / TIMEOUT 0。
本文替代 2026-03-30 的旧分析（旧文件中 xattr、ACL、mknod、freeze、NFS export、O_DIRECT
当时都未实现，现在已全部完成；当时的 41 个 FAIL 也已清零）。

以下按"减少 NOTRUN 的性价比"排序。case 编号来自 2026-07-16 最终运行的原始日志。

## A. 环境类：不改 simplefs 代码，收益最大

| 原因 | 数量 | 做法 |
| --- | ---: | --- |
| dm flakey | 59 | VM 内核打开 `CONFIG_DM_FLAKEY`（034 039 040 041 056 057 059 065 066 073 090 101 104 106 107 177 311 321 322 325 335 336 341 342 343 348 376 456 479 480 481 483 489 498 502 509 510 512 520 526 527 534 535 547 552 557 640 677 690 695 703 741 745 764 771 779 782 784 785） |
| scsi_debug | 8 | VM 内核打开 `CONFIG_SCSI_DEBUG`（108 349 350 351 704 730 731 767） |
| $LOGWRITES_DEV | 4 | local.config 配置 dm-logwrites 设备（455 470 482 757），与 journal 配合才有意义 |
| 其他内核配置 | 5 | `CONFIG_FAIL_MAKE_REQUEST`（019）、THP（759 760）、SELinux（700）、BSD 进程记账（596） |

注意：dm flakey / logwrites 是故障注入类测试，解锁后会真实检验 simplefs 的 -EIO
路径，可能暴露新 FAIL。按项目规则（NOTRUN 可接受、FAIL 不可接受），解锁后必须全量
复跑确认。

## B. 容易实现：每个都是小改动

| 功能 | 数量 | 说明 |
| --- | ---: | --- |
| timestamp bounds | 1 | 402；在 fill_super 里填 `s_time_min/s_time_max` 即可 |
| maximum ACL count | 1 | 026；ACL 已实现，只需定义上限 |
| inode creation time | 1 | 528；inode 增加 crtime 字段（磁盘格式小改） |
| fs label | 1 | 492；`FS_IOC_GETFSLABEL/SETFSLABEL` + 超级块字段（磁盘格式小改） |
| 跨设备 copy_file_range | 1 | 565；检查为何拒绝，通常 generic fallback 即可 |
| fiemap -a | 1 | 425；fiemap 已有，缺特定 flag 支持 |

## C. 中等难度：基础设施已经存在

| 功能 | 数量 | 说明 |
| --- | ---: | --- |
| fzero（ZERO_RANGE） | 12 | 008 009 033 042 096 469 499 503 511 610 685 758；unwritten extent 已实现（285/286 通过），fallocate 只需增加 ZERO_RANGE 分支 |
| finsert（INSERT_RANGE） | 9 | 058 060 061 063 064 404 485 686 735；COLLAPSE_RANGE 已实现，insert 是对称操作 |
| chattr/lsattr/file_attr | 14 | 079 159 160 277 424 507 545 553 555 607 629 508 772 780；实现 `FS_IOC_GETFLAGS/SETFLAGS`，inode 加 flags 字段，支持 +i/+a 等 |
| FITRIM | 3+2 | 251 260 288 + 746（038 也依赖 FITRIM）；实现 FITRIM ioctl，按 bitmap 释放块 |

B + C 合计约 40 个 case，是"让 NOTRUN 大幅变少"最直接的一批。

## D. journal：代码已存在，但被禁用

现状：

- `simplefs_journal.c`（约 900 行）已实现物理日志，但 `simplefs_super.c:850` 的注释
  明确写着"当前 journaling 路径尚未足够稳定，默认禁用 journal"，fill_super 无条件
  设置 `sbi->s_journal_mode = 1`（禁用），xfstests 挂载也统一传 `-o nojournal`。
- `xfstests-full.sh` 把 simplefs 加进了 `common/rc` 的"无 metadata journaling"名单，
  因此 17 个 case 报 `simplefs does not support metadata journaling`
  （043 044 045 046 047 048 049 051 388 392 417 468 475 505 530 537 753）。
- 另有 norecovery（050）、log state probing（052 054 055）、$LOGWRITES_DEV（455 470
  482 757）共 8 个与日志恢复相关。

把 journal 稳定化（崩溃恢复正确性 + 默认启用 + 去掉 rc patch）大约可解锁 25 个
case。工作量比 B/C 大，且这批测试全是崩溃恢复场景，一旦 journal 有 bug 就是 FAIL，
风险高于 B/C。建议顺序：先做 B/C 的 40 个，再回头攻 journal。

## E. 难 / 不值得（维持 NOTRUN）

- reflink 176（138 scratch + 38 test）：需要共享 extent + CoW 引用计数，格式大改
- quota 31：完整 dquot 会计系统
- encryption 26 + inlinecrypt 2：fscrypt 全集
- verity 10：fsverity
- exchangerange 18：交换两个文件的范围，语义复杂
- atomic write 9：需要块设备 + fs 双重支持
- DAX 5、casefold 2、idmapped mounts 6、delayed allocation 614、huge file 525
  （单 extent 块格式上限约 10 MiB）
- 固定 4 KiB 块大小导致的合法跳过：114 240 366 450 538 641

## 已删除的旧计划

2026-03 版计划中的 mknod、freeze/thaw、NFS export、xattr、ACL 已全部实现，
对应 NOTRUN 已清零，不再列出。

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
