# 10. 测试与调试：如何证明“没有错误”

文件系统测试的目标不是“命令返回 0”，而是证明用户语义、持久化语义和内核健康
同时成立。SimpleFS 接受功能型 NOTRUN，但不接受 FAIL、TIMEOUT、silent corruption、
warning、Oops 或 panic。

## 三层验证

第一层是可观察的手工实验：创建、写入、fsync、卸载、重挂载、读回；或专门观察
mmap/FIBMAP/255 字节名字。它适合理解调用链，不足以证明并发和错误路径。

第二层是定向 xfstests：修改一个功能后，运行直接用例、曾经失败的固定 workload 和
邻接回归。例如改 unwritten conversion，至少覆盖 partial write、fallocate、mmap、
DIO 和 fsx 相关用例。

第三层是一次连续的 generic 001--787 全量运行，并检查结果集合、每例日志和 dmesg。
不同日期或不同内核环境的零散 PASS 不能拼成一次全量验收。

## 四种结果

- PASS：测试正文实际执行，输出与 golden result 匹配，内核无异常；
- NOTRUN：前置 capability/环境/格式限制不满足，并保留具体原因；
- FAIL：输出错误、setup/mount 失败、runner 非预期返回；
- TIMEOUT：超时，只表示没有完成，不能自动判断内核 hang。

外部中断、SSH 断开、VM reset 后产生的半份汇总都不是 PASS。

## 标准入口

`xfstests-full.sh` 只负责 runner 生命周期；`tests/xfstests/prepare.sh` 生成 private
xfstests 配置和 wrapper；`tests/xfstests/patch.py` 幂等添加 SimpleFS capability。

```text
xfstests-full.sh
  -> parse 001/range/full case list
  -> prepare private xfstests compatibility layer
  -> recreate loop images for each case
  -> run ./check under timeout
  -> save one log per case and aggregate status
```

入口不直接修改 `/home/martins3/data/xfstests`，而是同步到 `/var/tmp` 的私有副本。
这是为了让 capability patch 可重复、可审计，也不污染上游源码。

## 修改后的最小回归

1. 主机普通用户执行 `./build.sh`；
2. 确认 mkfs 和 ko 都是本次构建产物，ko vermagic 匹配 guest `uname -r`；
3. guest root 跑一个最小用例；
4. 立即看该用例 log 和 dmesg；
5. 跑功能定向集；
6. 最终跑 001--787，并机器校验数量、唯一性和 NOTRUN 原因。

旧 mkfs 配当前模块也会表现为 mount/JBD2 错误，所以“代码没改 mkfs”不代表可以复用
旧 binary。构建产物时间、哈希和 vermagic 是测试环境的一部分。

## 固定随机 workload

fsx 失败时先保存 seed、命令行、`.fsxops`、`.fsxgood`、镜像和 dmesg。比较前 N-1
和 N 个操作，找出第一个状态分叉，再检查文件内容、extent 和 page cache。

换随机 seed 后“暂时不失败”只是概率变化。固定 workload 在修复前稳定失败、修复后
稳定通过，才是强证据。

## TIMEOUT 不等于 hang

先看进程状态和 CPU：持续 R 状态、CPU 满载可能只是压力用例正常运行。再看内核栈、
dmesg、I/O 进展和结果文件是否变化。只有状态停止变化且栈/日志支持时才判断 hang。

runner 默认给每例 3600 秒，是因为 debug kernel 下 `generic/074` 等 CPU workload
确实可能运行数分钟。缩短 timeout 不能修复文件系统，只会制造假 TIMEOUT。

## 内核异常时的顺序

出现 WARNING/Oops/panic 或 SSH 不响应时，先保留现场，不要用 guest 内完整 reboot：

1. 用 collei log 读取串口日志；
2. 必要时从 QEMU monitor 保存 guest memory；
3. 用匹配 vmlinux 分析栈或 vmcore；
4. 保存精确 workload 和构建哈希；
5. 之后才用 `force_reboot` 快速恢复 VM 并复现。

完整案例见 [调试方法与案例](../development/debugging.md)。

## “修复”的证据层次

从强到弱：精确 workload 前后对照、对应用例多次 PASS、邻接并发回归、构建和静态
检查、dmesg 健康扫描、最终全量。局部 PASS 不能证明未覆盖的功能，全量 PASS/NOTRUN
统计也不能替代 dmesg 检查。

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
