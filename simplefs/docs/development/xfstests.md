# xfstests runner

## 入口职责

`xfstests-full.sh` 只保留五件事：解析用例、隔离 `/tmp`、创建 loop 设备、逐例运行和
汇总结果。xfstests 兼容逻辑不再内嵌在入口中：

| 文件 | 职责 |
| --- | --- |
| `xfstests-full.sh` | runner 生命周期和结果判定 |
| `tests/xfstests/prepare.sh` | private copy、local.config、helper/wrapper 生成 |
| `tests/xfstests/patch.py` | 对上游 common/tests 做幂等 capability 适配 |

拆分的目的不是隐藏复杂度，而是让每层只有一个变化原因：runner 变化不需要阅读
xfstests patch，格式 capability 变化也不需要碰设备清理代码。

## 为什么使用 private copy

源目录固定为 `/home/martins3/data/xfstests`，运行副本位于
`/var/tmp/simplefs-xfstests/xfstests`。适配器只修改副本，包含：

- SimpleFS mkfs/mount helper；
- 固定 4 KiB block、时间戳范围、卷标与 ACL 上限；
- `s_needs_recovery` log-state probe；
- legacy mount wrapper；
- 492、620、735、746 的真实 capability guard；
- 558 在多个子目录中并发耗尽 inode，避免单个 worker 目录超过磁盘格式的
  40,920 项上限。

`patch.py` 可以连续执行，第二次不应继续改变文件。上游 needle 消失时它应失败并
要求人工检查，而不是静默跳过，这能及时发现 xfstests 升级导致的适配失效。

## 用例选择

```bash
./xfstests-full.sh             # generic/001 ... generic/787
./xfstests-full.sh 74          # generic/074
./xfstests-full.sh generic/074
./xfstests-full.sh 100-120
./xfstests-full.sh 1 50 100-110
```

编号超出 001--787、倒序范围或未知格式直接报错。

## 每例隔离

每个 case 前重新创建 sparse image，并绑定固定 loop200/201/202。通常 test/scratch
都是 2 GiB；038、048、312、590、620、694、701、747、781 明确需要 4--16 GiB，
runner 只对这九项改用 20 GiB。这样既不会把环境容量不足误报为 capability NOTRUN，
也避免 787 个 case 都格式化大 inode store。log-writes image 始终为 2 GiB。

每例重建使前一用例的文件系统内容不会泄漏到后一例。runner EXIT trap 会卸载
test/scratch、卸载自己替换的 `/tmp`、rmmod 并 detach loop。

VM 的 `/tmp` 处于 shared propagation 域。runner 先把 target 变成 private，再 bind
自己的 tmp root，防止重复运行产生指数增长的 mount stack。

## 结果判定

`./check` 返回 0 且日志没有 `[not run]` 才是 PASS；含 `[not run]` 是 NOTRUN；
timeout 124 是 TIMEOUT；其他返回是 FAIL。任意 FAIL/TIMEOUT 会使 runner 最终返回
非零。

汇总只是索引，根因在逐例 log 和 dmesg。setup 阶段的 mkfs/mount 失败必须记 FAIL，
不能因为测试正文尚未开始而降级为 NOTRUN。

## 更新适配器的规则

1. capability 必须来自当前磁盘格式或真实外部依赖；
2. NOTRUN guard 只用于确实不支持，不能遮住 SimpleFS 错误；
3. 所有 patch 幂等，needle 不匹配时 fail closed；
4. 先在临时原始文件副本上执行两次并比较哈希；
5. 通过 shellcheck、shfmt、Python syntax，再在 guest 跑至少一个真实 case。

`generic/558` 仍然创建到 inode 耗尽，并没有降低操作数量。适配器只把各 worker 的
文件放进独立子目录：上游测试的目标是验证全局 inode ENOSPC，而不是要求单目录
容纳超过文件系统格式上限的条目。若把它直接 NOTRUN，就会丢失真正有价值的 inode
位图、计数和并发分配覆盖。

`generic/620` 要求在 17 TiB dm-huge-disk 上 mkfs。SimpleFS 的 `nr_blocks`、
`s_journal_start` 和所有物理块引用都是 32 位，4 KiB block 下卷上限小于 16 TiB；
适配器必须在进入 mkfs 前按磁盘格式 NOTRUN，不能等 journal 尾区写入越界后再把
EIO 记成文件系统错误。

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
