# 2026-07-18 Phase 0+1 完成后全量 generic xfstests 结果

## 环境

- VM: `yyds-fs`，内核 `7.1.2-00001-gd41bd6abfe34`
- SimpleFS: 本 worktree 构建（Phase 0+1 的全部改动）
- 测试与 scratch 镜像默认 2 GiB，单 case 超时 3600 秒
- 运行入口：`./xfstests-full.sh`（无参数跑全部 787 个 generic case）

## 结果

| 指标 | 本次（2026-07-18） | 基线（2026-07-16） | 变化 |
| --- | ---: | ---: | --- |
| PASS | 345 | 307 | +38 |
| NOTRUN | 442 | 480 | -38 |
| FAIL | 0 | 0 | 0 |
| TIMEOUT | 0 | 0 | 0 |

Phase 0（环境 + 3 个 FAIL 修复）+ Phase 1（fzero/finsert/chattr/FITRIM/小项）
净解锁 38 个 case，且未引入任何 FAIL/TIMEOUT。

## 本次新增的 PASS（按功能分）

- Phase 0：704（512 扇区 DIO）、730/731（设备移除 EIO/shutdown）、108/350/741
- fzero：008 009 033 096 469 499 503 511 610 685 758 349
- finsert：058 060 061 063 064 404 485 686
- chattr/fileattr：079 277 424 545 553 555 772 780
- FITRIM：251 260 288 038
- 小项：026（ACL 上限）、425（fiemap XATTR）、402（timestamp bounds）、528（crtime）

## NOTRUN 构成（442 个，按原因，前几位）

- metadata journaling 相关 78：dm-flakey 58（测试先查 dm flakey 再查 journal）、
  metadata journaling 17、507/629、log probing 3、norecovery 1——Phase 2 目标。
- reflink 177（138 scratch + 39 test）：不做（要 CoW 引用计数）。
- quota 31、encryption 26、exchangerange 18、verity 10、dedupe 16：不做。
- 其余小项：idmapped 6、dm thin-pool 4、LOGWRITES_DEV 4、atomic write 6、
  DAX 5、SCRATCH_LOGDEV 2、casefold 2 等。

## 合法 NOTRUN（格式/环境限制，有据可查）

- generic/735：simplefs 单文件上限约 2.9TB < 测试所需 16TB（harness inline 检查）。
- generic/746：loop-on-self 嵌套下 sync-zeroout 分配回写链自锁（tmpfs 后备对照正常）。
- generic/492：label ioctl 已实现，blkid 无 simplefs 探测定义（用户态库）。
- generic/565：跨设备 copy_file_range 与 ext2/ext4 一致返回 EXDEV。
- generic/042：解锁 fzero 后转为 journal 依赖（Phase 2）。

## 关键 harness 基建

- `xfstests-full.sh` 是标准入口；`legacy_mount.c` 把 simplefs 挂载改走
  legacy mount(2)（util-linux 新 mount API 在此内核不触发 timestamp expiry 警告），
  MOUNT_PROG wrapper 处理 MS_* 选项翻译。
- `simplefs_free_sectors.py` 读 simplefs 空闲位图导出空闲段（generic/746 设计用，
  现因 loop-on-self 死锁未启用，留作格式文档）。

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
