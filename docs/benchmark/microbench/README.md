# 本机 C++ microbench

对应 [个人性能测试记录](../my-result.md)。Linux x86-64、C++20，无第三方 benchmark 库。源码入口在 `microbench.cpp` 的 `main()`，每组实验是一个独立函数。

## 编译与运行

```bash
cd /home/martins3/data/vn/docs/benchmark/microbench
./run.sh
```

脚本重新编译，创建独立的 `results/日期时间.随机后缀/`，保存环境、原始 CSV、进度日志和运行后的 load average。完整采样每个配置 5 次；最大同时使用 32 个逻辑 CPU，主要内存分配峰值约 512 MiB。不会清理系统缓存、更改 governor、关闭安全防护或者重启机器。二进制以 `.out` 结尾。

本机同时有 Nix 和系统工具链，因此 Makefile 使用 `/usr/bin/g++ -B/usr/bin/`，保证系统编译器、链接器和运行库配套；否则 PATH 中的 Nix 链接器可能注入另一版本的 glibc。编译参数为 `-O3 -g -std=c++20 -march=native`。如需其他工具链，可同时覆盖 `CXX` 和 `CXXFLAGS`。`-march=native` 的产物只应在兼容 CPU 上运行。

```bash
make smoke
./microbench.out --group memory --samples 5 > memory.csv
./microbench.out --group compute --cpu 16 --samples 5 > compute-e-core.csv
./microbench.out --group scaling --samples 7 > scaling.csv
./microbench.out --group context --samples 5 > context.csv
sudo ./microbench.out --group syscall --samples 5 > syscall-pmu.csv
```

`--quick` 减少大部分循环次数，用于功能检查，不作为正式结果。大工作集指针链仍至少访问两遍。`--cpu` 指定串行实验和通信发起者；`scaling` 始终使用程序启动时允许的整个 CPU 集合。可以通过 `taskset -c` 限制这个集合。程序自动按 sysfs 的 socket/core/sibling 信息分组，不把连续编号误当成独立物理核。`smt_cores` 指具备 SMT 的核，`single_thread_cores` 指没有 SMT sibling 的核；仅在这台已核实的 13900K 上分别解释为 P/E 核。

`stdout` 只有 CSV，`stderr` 是进度、PMU 可用性和最终 checksum。`perf_event_paranoid` 阻止内核态计数时仍执行耗时测试，并明确提示 PMU 不可用，不填伪造的零。

## 实验与计量边界

| 分组 / 函数 | 测试方法 | 输出和限制 |
| --- | --- | --- |
| `compute` / `compute()` | 64 位无符号整数乘加，一条依赖链与四条独立链 | `ns/update`；一次 update 是一次乘加，包含循环开销，不是单条指令延迟。禁止 SIMD 合并，体现标量指令级并行 |
| `branch` / `branch_loop()` | 同一组 64 KiB 的 0/1 数据，各占一半；排序与随机打乱 | `ns/branch`；内联汇编固定条件跳转，避免被优化为 cmov。差值包含预测和流水线等影响，不是直接测得的单次预测失败惩罚 |
| `memory` / `memory()`、`chase()` | 4 KiB 到 256 MiB，每 64 B 一节点，固定种子构造遍历所有节点的随机环 | `ns/load`；真正串行依赖的 pointer chasing，无法通过并发访存隐藏延迟。使用 `MADV_NOHUGEPAGE`，因此大工作集也包含 TLB miss/page walk，不能称为纯 DRAM 延迟 |
| `memory` / `memory()` | 32 KiB、1 MiB、16 MiB、256 MiB 的 libc `memcpy` | `GB/s` 是有效载荷字节数/时间，十进制 GB；实际访存流量可能包含读、写和 RFO。CSV 工作集大小是单个缓冲区，源+目标占两倍 |
| `syscall` / `syscalls()` | 循环基线、raw `getpid`、libc `clock_gettime`、raw `clock_gettime`、向 `/dev/null` 写 1 B | `ns/call`。raw 路径用 `syscall()`，避免把 vDSO 当成陷入内核；libc 时钟路径在本机使用 vDSO |
| `syscall_cpu` / `Usage` | 用 `getrusage(RUSAGE_THREAD)` 观察批量操作的用户态/内核态 CPU 时间 | `user_ns/call`、`system_ns/call`；有记账粒度和测量边界开销，用于解释 us/sys，不适合推断极短操作的精确分账 |
| `syscall_pmu` / `Counters` | 当前线程的 perf cycles/instructions 事件组，包含内核态，排除 hypervisor | `cycles/call`、`instructions/call`；包含用户态循环、libc 包装及极少的计时/PMU 控制开销。不是 syscall 内核函数单独的指令数，也不是指令种类分布。若发生 multiplexing 则报错，不盲目缩放 |
| `scaling` / `scaling()` | 多种核放置，分别执行依赖链和四条独立链；每个配置的总工作量固定 | `Mupdates/s`；预热和创建线程不计时，起止 barrier 计时。配置顺序逐轮轮换。包含不同核的频率差异、同步开销和尾部慢核影响 |
| `coherence` / `coherence()` | 两线程在同一 atomic token 上 acquire-load/release-store 交替，自旋等待，使用 `pause` | `ns/roundtrip`；一次完整往返，**不是单向延迟**。包含轮询和线程执行时间；SMT sibling 共享资源，不能统一解释为核间互连延迟 |
| `sharing` / `coherence()` | 两线程分别对两个 atomic counter 做 relaxed `fetch_add`；同一 cache line 与两个独立 line 对比 | `ns/increment` 是总墙钟时间除以两个线程的总操作数，即吞吐量的倒数，不是单次 RMW 的响应时间。计数结束后验证结果 |
| `context` / `context()` | 两条 pipe 传 1 B，pthread 与 fork 进程，比较同一逻辑 CPU、SMT sibling、不同物理核 | `ns/roundtrip`；含四次读写 syscall、pipe、阻塞、唤醒、调度及数据传递，不是纯 `switch_to()` 或内核线程切换开销 |
| `context_switches` / `context()` | 发起线程的 `ru_nvcsw` 和 `ru_nivcsw` 差值 | 每轮自愿/非自愿切换次数；只统计发起者，不能当作双方总数。跨核两端可并行执行 |

## 防止测到错误东西

- 主线程、每个工作线程和子进程都显式绑定 CPU，亲和性设置失败则终止。拓扑从绑定前的允许集合读取。
- 每项先预热，再计时；内存分配、随机排列、首次缺页和线程创建在计时外。内存预触摸不保证后续绝无回收或中断干扰。
- 使用 `steady_clock` 批量计时，除以操作次数；没有将 TSC tick 直接称为 CPU cycle，也没有从墙钟和标称频率推导 cycles。
- 计算结果保留到 checksum；编译器屏障防止消除乘加和 memcpy。可用 `objdump -d -C microbench.out` 查看 `chain()`、`independent()`、`branch_loop()`、`chase()`。
- CSV 保存每次样本，不仅保存最好的一轮；正文给出中位数及必要的 min/max。少量批次的最大值不是单操作 p99。
- 没有锁定睿频、隔离 CPU 或暂停其他本机任务。运行中的负载、温度、频率和共享 LLC 会影响数据，结论只适用于记录中的这次实验。
- 缓存行按本机核实的 64 B 编写。此套件不是任意架构的自动调参工具。

## CSV 字段

`group,case,cpus,working_set_bytes,sample,operations,elapsed_ns,unit,value`

`cpus` 用 `+` 连接 CPU 编号，避免与 CSV 分隔符冲突；`sample` 从 0 开始。`operations` 在 memcpy 行表示累计载荷字节，在其他行表示该项定义的迭代、乘加、往返或原子增量次数。对 `(group, case, cpus, working_set_bytes, unit)` 分组后取 `value` 的中位数即可复现正文统计。所有时间结果越小越好，GB/s 和 Mupdates/s 越大越好。

## 与原始笔记的关系

[core-to-core-latency](https://github.com/nviennot/core-to-core-latency) 通过固定两线程的 CPU 来测缓存一致性通信。本实现使用 load/store token 往返而非相同 CAS 协议，数值不能直接和它的表格混用。

[Tsuna 的 context-switch 文章](https://blog.tsunanet.net/2010/11/how-long-does-it-take-to-make-context.html) 是原笔记的参考阅读；这里明确把 pipe 往返称为调度和通信的组合测试，不把往返除二当作纯上下文切换耗时。

内核完整构建、VM host/guest 对照、真实编辑器输入到显示的延迟、安全防护开关的 reboot A/B，以及指令类别分布，都需要进一步的专项实验。当前 microbench 不替代它们。

## STREAM 式物理内存带宽

`stream.cpp` 是独立的 AVX2/FMA 测试，不是官方 STREAM 提交版本。默认每数组
512 MiB，共三个数组，比较 4 KiB 页和 THP；每配置预热一批，再测七批，
每批连续扫描四遍。输出到 stdout 的是逐批 CSV，stderr 是中位数和 min/max。

```text
make stream
./stream.out > stream.csv 2> stream.log
./stream.out --pages thp,4k --mib 1024 --samples 9 --passes 8 --threads 1,2,4,8
./stream.out --pages thp --cpus 16,17,18,19,20,21,22,23 --threads 1,2,4,8
./stream.out --pages hugetlb --mib 512 --threads 1,2,4,8
```

- `cpu_order()` 从当前允许的 CPU 集合及 sysfs 拓扑取每物理核一个线程，最后才添加
  SMT siblings。`--cpus` 可显式指定顺序；`--threads` 取该列表前 N 个 CPU。
  默认线程数扫 1/2/4/8/16/24/32 以及可用 CPU 总数。CSV 的 `cpus` 保留真实放置。
- `Mapping` 用 `MADV_NOHUGEPAGE` 强制 4 KiB 页，THP 用 2 MiB 对齐映射、
  `MADV_HUGEPAGE`，并在系统头文件支持时请求 `MADV_COLLAPSE`。
  `huge_kib_before/after` 来自本次三个数组的 `/proc/self/smaps`，表示计时前后
  实际大页覆盖的 KiB 数。THP 请求可能只部分成功，必须检查覆盖率。
- `--pages hugetlb` 显式请求 2 MiB `MAP_HUGETLB`，需要足够的大页池及权限；
  程序不会修改系统大页池，申请失败直接报错，不会静默降级。默认不运行该模式。
- `run_op()` 在计时外并行初始化、核实页映射和创建工作线程；工作线程跨样本复用。
  起止 barrier 的 completion function 记录墙钟，计时包含同步唤醒与慢线程拖尾。
  分块边界为 128 B 的倍数，支持 3、7、24 等线程数，不越界、不共享边界缓存行。
  数组在同一页模式内复用，改变线程数不会迁移已有物理页；跨 NUMA 机器需另外控制
  内存放置。本次物理机只有一个 NUMA node。
- `read_avx()` 使用四个独立 AVX 累加器；普通写和 NT 写显式使用不同存储指令，
  `triad`/`triad_nt` 使用 FMA。每次扫描包含编译器屏障，存储循环末尾执行 fence，
  但 fence 不会把所有普通脏缓存行强制写回 DRAM。
- 每配置结束后在计时外检查三个数组的每个元素及读操作 checksum，校验成功才输出。
  小数组可用于校验程序，但会测到缓存，不能纳入 DRAM 峰值。正式结果应独立复测，
  并用更大数组排查缓存影响。

`GB/s = payload_bytes / elapsed_ns`，均为十进制。计数口径如下：

| 操作 | 每个 double 的计数字节 | 典型 DRAM 流量模型 |
| --- | ---: | ---: |
| read | 8 | 8 |
| write | 8 | 16（含 RFO） |
| copy | 16 | 24（含 RFO） |
| triad | 24 | 32（含 RFO） |
| write_nt | 8 | 8 |
| copy_nt | 16 | 16 |
| triad_nt | 24 | 24 |

这些模型不能代替内存控制器硬件计数器：缓存命中、写回时机和处理器优化都可能
改变实际流量。`copy` 的有效读写带宽是复制文件常用的“源数据字节数/时间”的两倍。
测试不锁频、不暂停其他程序、不清缓存；输出的最大样本是观察到的最好一批，
不是机器在所有条件下的硬件极限。

页模式依据：[Linux THP 文档](https://docs.kernel.org/admin-guide/mm/transhuge.html)、
[HugeTLB 文档](https://docs.kernel.org/admin-guide/mm/hugetlbpage.html)；
计数口径依据：[STREAM FAQ](https://www.cs.virginia.edu/stream/ref.html)。

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
