# microbench 简单的测试

## 2026-09-21：本机 C++ microbench

代码和运行说明：[microbench/README.md](microbench/README.md)，实现：[microbench.cpp](microbench/microbench.cpp)。在该目录执行
`./run.sh` 可以重新构建并采样。

本次普通用户运行覆盖 68 个实验配置，每项 5 次，共 470 行数据（含 CPU
时间、上下文切换等辅助指标）。另以 root 运行 syscall 组以读取包含内核态的
PMU，得到 125 行数据。以下默认报告 5
次的中位数；区间是批次最小值到最大值，不是单操作的 p99。

- [完整原始样本](microbench/results/20260921-173030.5XK8r0/samples.csv)
- [syscall PMU 样本](microbench/results/20260921-173030.5XK8r0/syscall-pmu.csv)
- [中位数与 min/max 汇总](microbench/results/20260921-173030.5XK8r0/summary.csv)
- [环境快照](microbench/results/20260921-173030.5XK8r0/environment.txt)
- [执行日志](microbench/results/20260921-173030.5XK8r0/run.log)

### 机器与实验条件

| 项目             | 本次配置                                                                    |
| ---------------- | --------------------------------------------------------------------------- |
| CPU              | Intel Core i9-13900K，8 个 P 核 + 16 个 E 核，共 24 个物理核、32 个逻辑 CPU |
| P 核编号         | `(0,1)`、`(2,3)`、…、`(14,15)`，括号内是同一核的两个 SMT sibling            |
| E 核编号         | `16..31`，每核一个线程                                                      |
| 默认串行测试 CPU | CPU 0，P 核                                                                 |
| 缓存             | CPU 0 的 L1D 48 KiB、L2 2 MiB；共享 L3 36 MiB；cache line 64 B              |
| 内核             | `7.1.3-201.fc44.x86_64`，PREEMPT_DYNAMIC                                    |
| 工具链           | 系统 GCC 16.2.1，`-O3 -std=c++20 -march=native`，系统链接器                 |
| 调频             | `performance` governor，未锁定频率，未禁用睿频                              |
| 内存页           | 指针链与 memcpy 缓冲区显式 `MADV_NOHUGEPAGE`，4 KiB 页                      |
| 安全防护         | **已有启动参数 `mitigations=off`**，本次没有修改                            |
| PMU 权限         | `perf_event_paranoid=2`，普通用户测耗时；root 单独测 cycles/instructions    |

这是正在使用的工作站，并非空闲专用测试机：有 QEMU、浏览器等后台进程，运行后的
load average 为 `3.17 2.46 2.53`。没有停止其他任务、清理系统缓存、改 governor
或重启。CPU 亲和性防止线程迁移，但不能排除共享缓存、后台任务、温度和频率的影响。

## 理解超线程核心

- [x] 对比使用 4 个 core 和使用这 4 核的 8 个 thread。

`scaling()` 用固定总量的整数乘加任务，每种配置均匀分配到各线程。`chain`
每线程只有一条依赖链；`ilp4` 每线程有四条独立链，编译器屏障阻止将其合并为
SIMD。两类负载分别比较，不把 update 当成单条机器指令。单位均为
**百万次乘加/秒**，越高越好。

| 放置                   | 逻辑 CPU                         |  chain |   ilp4 |
| ---------------------- | -------------------------------- | -----: | -----: |
| 1 个 P 核，1 线程      | `0`                              |  1,357 |  5,406 |
| 1 个 E 核，1 线程      | `16`                             |    713 |  2,749 |
| 4 个 P 核，4 线程      | `0,2,4,6`                        |  5,425 | 21,619 |
| 同 4 个 P 核，8 线程   | `0..7`                           | 10,865 | 21,703 |
| 8 个 P 核，8 线程      | `0,2,...,14`                     | 10,874 | 43,028 |
| 4 个 E 核，4 线程      | `16..19`                         |  2,846 | 10,926 |
| 8 个 E 核，8 线程      | `16..23`                         |  5,681 | 21,802 |
| 24 个物理核，各 1 线程 | P 核各取一个 sibling + 全部 E 核 | 16,452 | 64,165 |
| 全部 32 个逻辑 CPU     | `0..31`                          | 20,925 | 71,148 |

同 4 核从 4 线程到 8 线程，chain 吞吐提升 **2.00 倍**，ilp4 仅 **1.004
倍**。这支持“超线程收益取决于单线程是否留下执行资源空闲”的解释；增加到 8
个真正的 P 核后，ilp4 才接近再翻倍。

全部 32 线程的 ilp4 批次范围是 63,421–79,325 Mupdates/s，波动明显。P/E
核均分工作也有慢核拖尾问题，因此这项不是最优调度策略或整机理论极限。

## 到底应该使用多少个 thread 来编译内核

**仍待实际构建实验，不能从上面的乘加吞吐推出 `make -j` 的最佳值。**
编译混合了分支、访存、文件访问和多进程调度，和固定的计算循环不同。

后续可固定源码版本、config、工具链和目标，分别测
`-j8/16/24/32/40/48`；每次使用同样的干净构建状态，区分是否启用
ccache、文件缓存是冷是热，重复记录 wall
time、user/sys、峰值内存。不要把增量构建和完整构建混在一起。

## 虚拟化的开销

**本次没有做 host/guest 对照。** 当前 syscall 实验可以提供 host 基线，但 `top`
的 us/sys 比例本身不是虚拟化开销百分比；CPU 时间分账和同一任务在 host/guest
上增加多少 wall time 是两个问题。

后续应在同一物理机上控制 vCPU 放置、CPU
型号暴露、宿主负载和内存策略，用同样二进制与参数重复采样。`--cpu`
可以控制单核实验，`taskset` 可以控制整个可用 CPU 集合。

## Jeff Dean 延迟表格：用本机数据形成量级直觉

这些是本机当前状态下各段代码的批量平均开销，不是跨机器通用常数。

### 缓存与内存访问

`memory()` 用随机指针环，每 64 B
一节点，必须完成上一次加载才能知道下一个地址。构造、缺页预触摸、预热在计时外，阻止顺序预取和内存级并行掩盖延迟。

| 工作集  | ns/load 中位数 |      批次范围 |
| ------- | -------------: | ------------: |
| 4 KiB   |           0.92 |     0.91–0.93 |
| 32 KiB  |           0.93 |     0.92–0.94 |
| 48 KiB  |           0.94 |     0.93–0.96 |
| 64 KiB  |           2.94 |     2.92–2.95 |
| 128 KiB |           2.96 |     2.95–2.97 |
| 512 KiB |           3.32 |     3.30–3.36 |
| 1 MiB   |           3.91 |     3.89–3.97 |
| 2 MiB   |           8.97 |     8.90–9.12 |
| 4 MiB   |          15.15 |   14.99–15.31 |
| 8 MiB   |          41.92 |   30.97–68.58 |
| 16 MiB  |         102.44 | 100.69–104.97 |
| 32 MiB  |         106.87 | 103.95–109.14 |
| 64 MiB  |         105.56 | 102.76–109.08 |
| 128 MiB |         115.61 | 108.52–122.60 |
| 256 MiB |         108.24 | 105.93–114.27 |

48 → 64 KiB 的跳变与本机 48 KiB L1D 容量吻合。但不能把“16 MiB 约 102 ns”写成 L3
命中延迟：这次使用普通 4 KiB 页，大工作集包含 TLB miss/page
walk，另外还有后台负载和共享 LLC 竞争。这里没有用 PMU
分离这些原因，不能仅凭容量和曲线断言具体瓶颈。

为检查波动，另以相同二进制和 CPU 0 独立复测了内存组，每项 3 次，保存
[复测原始数据](microbench/results/20260921-173030.5XK8r0/memory-repeat.csv)。8
MiB 指针链中位数变为 19.40 ns，16 MiB 为 96.10 ns，256 MiB 为 104.33 ns。8 MiB
对运行时状态明显敏感，不能作为固定的 L3
延迟引用；正文保留初次数据，没有挑选更快的一组替换。

### memcpy 吞吐

| 单个缓冲区大小 | 有效载荷 GB/s |    批次范围 |
| -------------- | ------------: | ----------: |
| 32 KiB         |         74.67 | 74.54–74.79 |
| 1 MiB          |         52.94 | 52.48–53.50 |
| 16 MiB         |          8.04 |  5.89–10.66 |
| 256 MiB        |          7.85 |   7.43–8.46 |

源与目标各占表中的大小。只计算复制的有效载荷，实际总访存流量更多；这是单线程
libc memcpy 的结果，不是整机 DRAM 带宽。16 MiB 项波动较大，不能把 8.04 GB/s
当成稳定硬件参数。

独立复测时，16 MiB 和 256 MiB 分别为 19.76 和 11.84
GB/s，再次说明这台运行着其他任务的机器不适合只报告一个“内存带宽常数”。本次没有证据把变化归因到某个特定后台进程。

### 指令依赖与分支

| 实验                   |          中位数 |
| ---------------------- | --------------: |
| 单条依赖链中的整数乘加 | 0.736 ns/update |
| 四条独立链中的整数乘加 | 0.184 ns/update |
| 排序的 50% 取分支数据  | 0.184 ns/branch |
| 随机打乱的同一份数据   | 2.444 ns/branch |

四条独立链每次乘加的摊销时间约为单链的
1/4。分支循环用汇编保留了条件跳转，随机数据比排序数据慢约 13.3
倍；两者加载相同数量的字节，条件成立次数相同。差值不是直接测得的单次 branch miss
惩罚，也不能外推为真实程序的整体加速比。

### CPU 之间传递数据

参考：[core-to-core-latency](https://github.com/nviennot/core-to-core-latency)。这里使用
acquire/release atomic token 的完整往返，协议不同，不能直接与该工具的 CAS
延迟混用。

| 放置                    | CPU    | ns/roundtrip |
| ----------------------- | ------ | -----------: |
| 同一 P 核的 SMT sibling | `0,1`  |        35.26 |
| 不同 P 核               | `0,2`  |        66.02 |
| P 核和 E 核             | `0,16` |        70.74 |

这些包含原子加载/存储、轮询、`pause` 和两个方向的交接。SMT sibling
共享核内资源；表格不是单向的互连硬件延迟。

### 伪共享

两线程各对自己的 atomic counter 做 `fetch_add(relaxed)`，比较 counter
位于同一缓存行和不同缓存行。数值是总时间/两个线程的总增量次数，反映吞吐，越低越好。

| 放置              | 同行 ns/increment | 分行 ns/increment | 同行/分行 |
| ----------------- | ----------------: | ----------------: | --------: |
| SMT sibling `0,1` |             4.411 |             4.639 |      0.95 |
| 不同 P 核 `0,2`   |             5.773 |             1.665 |      3.47 |
| P/E 核 `0,16`     |             6.827 |             2.145 |      3.18 |

跨物理核时，分开缓存行带来明显改善；SMT sibling
上没有改善，说明不能把跨核的伪共享结论无条件套到同核线程上。本项使用原子
RMW，不代表所有普通存储代码的开销。

## 编辑器的延迟

仍待实际应用测量。计算、syscall 和调度量级不能替代“按键输入 → 编辑器处理 →
terminal/compositor → 屏幕显示”的端到端延迟；本次没有声称测过 Neovim
的输入延迟。

## 一次 kernel thread 上下文切换的开销

参考：[How long does it take to make a context switch?](https://blog.tsunanet.net/2010/11/how-long-does-it-take-to-make-context.html)

本次 `context()` 测的是**用户态 pthread / fork 进程通过两条 pipe
阻塞唤醒**，没有创建内核 kthread，也没有直接测 `switch_to()`。每次往返包含两次
write、两次 read 和唤醒/调度工作。

| 放置               | pthread µs/roundtrip | fork 进程 µs/roundtrip |
| ------------------ | -------------------: | ---------------------: |
| 同一逻辑 CPU `0,0` |                1.458 |                  1.564 |
| SMT sibling `0,1`  |                2.816 |                  2.799 |
| 不同 P 核 `0,2`    |                2.649 |                  2.604 |
| P/E 核 `0,16`      |                3.008 |                  3.064 |

发起者每轮的自愿+非自愿切换次数约为 1；同 CPU pthread 测得自愿约 0.487、非自愿约
0.513，跨 CPU 的自愿切换约为
1。不能预设所有交接都被记为自愿阻塞。只测了发起者的计数，不能当成双方合计。

同 CPU 的往返更快，不意味着上下文切换“免费”，也不意味着两个计算线程应该挤在同一
CPU。这个结果只适用于频繁互相等待、每次仅传 1 B 的协议。往返除二仍包含 syscall
和 pipe 等工作，不能写成纯切换耗时。

## 一个 syscall 的开销

### 耗时和用户态/内核态 CPU 时间

| 操作                                 | 墙钟 ns/call | user ns/call | system ns/call |
| ------------------------------------ | -----------: | -----------: | -------------: |
| 循环基线                             |        0.185 |        0.000 |          0.000 |
| raw `getpid`                         |       46.079 |       10.992 |         34.909 |
| libc `clock_gettime`，本机 vDSO 路径 |        9.488 |        9.980 |          0.000 |
| raw `clock_gettime`                  |       69.805 |       12.989 |         55.893 |
| 向 `/dev/null` 写 1 B                |       58.588 |       20.944 |         36.917 |

CPU 时间由 `getrusage(RUSAGE_THREAD)`
对一百万次调用分批计量，有记账粒度和额外边界开销，各列也分别取了中位数，因此不能要求它们严格相加等于墙钟。循环基线的
CPU 时间为 0 是记账分辨率不足，不是循环不消耗 CPU。

同样读取单调时钟，本机 vDSO 路径比显式 syscall 快约 7.36 倍。`getpid` 使用
`syscall(SYS_getpid)`；时钟 raw 路径使用
`syscall(SYS_clock_gettime)`，避免测错路径。

### Spectre / Meltdown

当前机器已用 `mitigations=off`
启动，所以以上数字**不是防护默认开启时的结果**。本次没有重启做 on/off
A/B，也不能从这份数据推出防护开销。实际对照需要保持内核、微码、频率策略和负载一致，并分别记录启动参数与
vulnerability 状态。

## 指令种类的分布，平均多少条指令一个 syscall

`Counters` 使用 `perf_event_open()` 同时读取 cycles 与 retired
instructions，包含用户态和内核态。下面来自独立的 root syscall 运行，仍为 CPU
0、每项 5 次、每次一百万次迭代。

| 操作                  | cycles/迭代 | retired instructions/迭代 |
| --------------------- | ----------: | ------------------------: |
| 循环基线              |        1.00 |                      4.00 |
| raw `getpid`          |      251.35 |                    324.34 |
| libc `clock_gettime`  |       52.00 |                     75.07 |
| raw `clock_gettime`   |      382.10 |                    428.50 |
| `write(/dev/null, 1)` |      319.98 |                    585.43 |

这些是该 benchmark 每次迭代的总退休指令数，包含循环、结果累加、返回值检查、libc
包装和内核执行，以及摊薄后的计时/PMU 控制开销。**不能称为 syscall
内核实现单独的指令数**；简单减去基线也不能完全剥离这些差异。cycles 来自
PMU，没有用标称频率乘墙钟估算。

“指令种类分布”尚未测量：retired instructions 只有总数，静态反汇编中的
load/store/branch 比例也不等于动态执行比例。后续需要选择具体代码段和可用
PMU/追踪机制再定义统计口径。

## 复现与检查

具体命令、单位和源码函数对应关系见
[microbench/README.md](microbench/README.md)。原始 CSV 保留了 CPU
放置、工作集、操作次数和逐轮结果，可自行计算中位数并检查离群值。检查记录见
[validation.md](microbench/results/20260921-173030.5XK8r0/validation.md)。没有
git stage 或 commit。

## 2026-09-22：本机 GPU microbench

代码和运行说明：[gpu-microbench/README.md](gpu-microbench/README.md)。`./run.sh` 重新构建并全量采样，覆盖 device / compute / memory / exec / launch / pcie 六组，共 50660 行数据。以下默认报告 5 次的中位数；区间是批次最小值到最大值。`tail` 组是 20005 次单操作往返，正文按百分位报，不取中位数了事。

- [完整原始样本](gpu-microbench/results/20260922-192048.uObQAp/samples.csv)
- [环境快照](gpu-microbench/results/20260922-192048.uObQAp/environment.txt)
- [执行日志](gpu-microbench/results/20260922-192048.uObQAp/run.log)

### 机器与实验条件

| 项目 | 本次配置 |
| --- | --- |
| GPU | NVIDIA GeForce RTX 5060 Ti，Blackwell，`sm_120`，36 个 SM |
| 显存 | 16.6 GB GDDR7，128 bit，显存时钟 14001 MHz |
| L2 | 32 MiB；每 SM shared memory 100 KiB（每 block 上限 48 KiB） |
| 额定时钟 | `cudaDevAttrClockRate` = 2572 MHz；**实际持续负载下 2790 MHz**（boost 超过标称） |
| 功耗墙 | 180 W；本次持续负载约 101 W、56 摄氏度，未顶到墙 |
| 环境 | **KVM guest**，RTX 5060 Ti 经 VFIO 直通，另有 virtio-gpu |
| PCIe | NVML 报 **Gen5 x8**（上限 Gen5 x16），负载前/中/后三次采样都是 x8 |
| 工具链 | CUDA 13.1 / nvcc，系统 g++ 15.2.1，`-O3 -std=c++20 -gencode arch=compute_120,code=sm_120` |
| 调频 | 未锁频，未用 `nvidia-smi -lgc`；每组吞吐测试前 `warm_clocks()` 压 1.5 s 负载 |
| 计时 | GPU 侧 `cudaEvent`，CPU 侧 `steady_clock`；两钟用 64 轮 ping-pong 标定，可行区间宽 4864 ns |

正在使用的工作站，不是空闲专用测试机。运行后 load average 1.40 1.32 1.02。

## 算力：实测几乎贴满理论峰值

B1 用内联 PTX 的 FMA/MAD，每线程 8 条独立链；B2 是 warp 级 MMA（wmma 对 FP16/BF16/TF32/INT8，裸 PTX `mma.sync` 对 FP8），累加器 8 条独立链。单位 `Gflop/s`（整数 `Gop/s`），越高越好。

| dtype / 形状 | 实测 | 说明 |
| --- | ---: | --- |
| FP32 `fma.rn.ftz.f32` | **25,423 Gflop/s** | |
| FP64 `fma.rn.f64` | 407.9 Gflop/s | FP64/FP32 = **1/62.3**，与假设的 1/64 吻合 |
| FP16 `fma.rn.f16x2` | 26,064 Gflop/s | 一条指令两个 MAC |
| BF16 `fma.rn.bf16x2` | 26,100 Gflop/s | |
| INT32 `mad.lo.s32` | 13,053 Gop/s | |
| SFU `sin/cos/rsqrt.approx` | 1,630 Gop/s | |
| MMA FP16 `m16n16k16` | **52,188 Gflop/s** | FP32 累加，稠密，无稀疏 |
| MMA BF16 `m16n16k16` | 51,963 Gflop/s | |
| MMA TF32 `m16n16k8` | 13,044 Gflop/s | |
| MMA FP8 e4m3 `m16n8k32` | **104,363 Gflop/s** | 裸 PTX，wmma 没有 FP8 fragment |
| MMA FP8 e5m2 `m16n8k32` | 104,359 Gflop/s | |
| MMA INT8 `m16n16k16` | 104,325 Gop/s | |

A1 推的理论峰值是 23,704 Gflop/s FP32（按标称 2572 MHz）和 448.0 GB/s 显存。**实测 FP32 25,423 比这个"理论值"还高**，因为持续负载下 SM 时钟是 2790 到 2797 MHz 而不是标称的 2572 MHz。峰值的算法是

```
FP32 峰值 = SM 数 x 每 SM 的 CUDA core 数 x 每条 FMA 的 FLOP 数 x 时钟
          = 36     x 128                x 2                  x 2.572 GHz
          = 23,704 Gflop/s
```

按 2790 MHz 重算是 25,713 Gflop/s，实测是它的 **98.9%**。这套换算的来历和利用率该怎么读，见 [peak-flops.md](peak-flops.md)。反过来用同一个公式解时钟，`25,423 / (36 x 128 x 2) = 2759 MHz`，与 A2 遥测到的 2790 到 2797 MHz 相符（compute 组本身没采遥测，时钟是从 A2 推的）。A1 里把 `cuda_cores_per_sm_assumed` 单独出一行，就是为了让这个换算可复核 —— 128 这个数 CUDA 不暴露，是公开资料的假设，98.9% 的利用率反过来支持它；若实际是 64，利用率就会是 198%，公式本身就错了。

FP8 e4m3 / e5m2 / INT8 三者都是 104 TOPS 左右，同速：它们共用同一套 tensor MAC 阵列，只是数制不同。FP16/BF16 是 52 TFLOPS，正好一半 —— `m16n16k16` 的 FLOP 计数翻倍但 MAC 数相同。**厂商标的 FP16 峰值（这块卡约 98 TFLOPS）是用 FP16 累加或稀疏得到的**，这里一律 FP32 累加、稠密，所以是 52。

MMA 的累加器用 4 条独立链和 8 条结果相同（都是 52 TFLOPS 量级），说明瓶颈不在累加器依赖上，是发射端。反过来 `mma_latency`（累加器单链）是 181 ns/update —— 但那是 2304 个 warp 并发共用每 SM 的 tensor core 测出的推进速度，不是孤立的 mma 依赖延迟。

### 指令延迟：与 CPU 版同方法

`chain1` / `chain4` 与 CPU 版 `compute()` 的一条依赖链 / 四条独立链同名同方法，`ns/update` 越小越好：

| | GPU FP32 FMA | CPU 整数乘加（2026-09-21） |
| --- | ---: | ---: |
| `chain1` | 5.86 ns | 0.736 ns |
| `chain4` | 2.96 ns | 0.184 ns |
| `chain1/chain4` | 1.98 | 4.00 |

绝对值不能直接比：CPU 版是 64 位整数乘加且是 P 核，这里是 FP32 FMA。比值的差别倒是有信息：CPU 上四条独立链把摊销时间砍到单链的 1/4，也就是把依赖延迟藏得很干净；GPU 上只砍到 1/2。原因是 `chain1` 的 5.86 ns 里含约 2 ns 的循环开销（一次 update 不是一条 FFMA 的指令延迟），留给 ILP 去藏的部分没那么多。

## 存储层次

### 带宽：L2 约 2.9 TB/s，DRAM 达到理论的 92%

`GB/s` 是有效载荷，稳态（热缓存）测量，越高越好：

| 工作集 | read | write | copy |
| ---: | ---: | ---: | ---: |
| 1 KiB | 12.3 | 14.8 | 28.1 |
| 16 KiB | 194 | 233 | 445 |
| 256 KiB | 1,276 | 1,194 | 2,106 |
| 1 MiB | **2,857** | 1,137 | **2,239** |
| 8 MiB | 1,788 | 1,113 | 1,454 |
| 64 MiB | 399 | 420 | 362 |
| 512 MiB | 410 | 407 | 349 |
| 1 GiB | 412 | 401 | 346 |

台阶很清楚：L1/L2 里 read 峰值 2.86 TB/s，跨过 32 MiB L2 之后落到 DRAM。1 GiB 的 read 是 **412 GB/s，是理论 448 GB/s 的 92%**。copy 的 346 GB/s 是有效载荷，实际总线流量是它的两倍（读加写加 RFO）。

write 在 1 MiB 之后就只有 1.1 TB/s，不到 read 的一半 —— 写路径更早撞到某个瓶颈，不是 DRAM 带宽（DRAM 段两者都是 400 GB/s 量级）。

### 指针追踪延迟

64 B 节点随机环，256 条独立链并发，`ns/load` 越小越好：

| 工作集 | `ld_default` | `ldg` | `ldca` | `ldcs` | `ldlu` |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 32 KiB | 15.6 | | | | |
| 2 MiB | 120.7 | | | | |
| 32 MiB | 139.9 | 138.7 | 145.5 | 231.4 | 260.2 |
| 256 MiB | 316.3 | | | | |

层次很清楚：32 KiB 在 L1 里 15.6 ns，2 MiB 进 L2 120.7 ns，256 MiB 落到 DRAM 316 ns。**但这些绝对值偏高**：256 条链并发，`elapsed/steps` 是并发压力下每条链的推进时间，含内存控制器排队，不是孤立加载延迟。**能读的是比值**：DRAM/L1 = 20 倍，L2/L1 = 7.7 倍。

同一工作集下加载路径的差别：`ld_default` 与 `ldg` 基本相同（139.9 vs 138.7），`ldca` 略慢，`ldcs`（streaming）和 `ldlu`（last-use）明显更慢（231 / 260 ns）—— 依赖链上的 load 用 streaming/last-use 提示是反效果，它们本来是为无复用的扫描准备的。

### shared memory 的 bank conflict

`GB/s`，索引随迭代游走防止编译器外提，越小表示冲突越重：

| stride | 1 | 2 | 3 | 4 | 8 | 16 | 32 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| GB/s | 5,945 | 5,997 | 5,999 | 3,264 | 1,506 | 756 | 378 |
| 相对 stride=1 | 1.00 | 1.00 | 1.00 | 1.83 | 3.95 | 7.86 | 15.7 |
| 理论冲突倍数 | 1 | 2 | 1 | 4 | 8 | 16 | 32 |

stride = 4/8/16/32 的下降是干净的等比（每翻倍约砍半），符合 f32 在 32 bank 上 `32/gcd(stride,32)` 路冲突的形状。两点偏差要说明：stride=2 理论上是 2 路冲突却没变慢；实测惩罚大约是理论倍数的一半。原因是索引随迭代游走，相邻迭代的冲突能和上一次的等待重叠。所以这张表给的是**冲突的相对代价**，不是单次 bank conflict 的精确周期数。

### 原子操作

`Gop/s` 是 RMW 吞吐，不是单次响应时间，越高越好：

| 目标 | shared+contended | global+sharded | global+contended | l2+contended | global+random |
| --- | ---: | ---: | ---: | ---: | ---: |
| Gop/s | 1,631 | 226 | 80.3 | 80.3 | 33.1 |

shared 内的争用原子比 global 的分片原子还快 7 倍，比 global 争用快 20 倍。global 的 contended 和 sharded 之间 2.8 倍就是一致性协议的代价。给目标加 access policy window 让它常驻 L2（`l2+contended`）**没有任何改善**（80.3 vs 80.3）—— 瓶颈不在 L2 命中率上。

## 执行模型

`occupancy_sweep` 是固定每线程 4 条依赖 FFMA 的吞吐，`Gflop/s` 越高越好；`occupancy` 是每 SM 活动 warp 数：

| 配置 | warps/sm | Gflop/s |
| --- | ---: | ---: |
| tpb=32 | 24 | 12,341 |
| tpb=64 | 48 | 23,655 |
| tpb=128 | 48 | 22,667 |
| tpb=256 | 48 | 23,094 |
| tpb=512 | 48 | 23,948 |
| tpb=1024 | 32 | 23,871 |
| tpb=256 + smem=8 KiB | 48 | 22,381 |
| tpb=256 + smem=16 KiB | 40 | 22,457 |
| tpb=256 + smem=32 KiB | 24 | 22,413 |
| tpb=256 + smem=64 KiB | 8 | 11,661 |

tpb=32 时只有 24 个 warp/SM，吞吐掉到一半；tpb≥64 之后 48 个 warp/SM，延迟隐藏就饱和了，再加占用率没有收益。用 shared memory 把占用率压到 24 个 warp 仍然不掉速，压到 8 个才掉一半。**结论是这个负载只需要约 24 个 warp/SM 就能隐藏完延迟**，occupancy 本身不是越高越好的目标。

屏障和发散的代价（`ns/barrier`、`ns/iter`，越小越好）：

| `__syncwarp` | `__syncthreads` 32 | 256 | 1024 | grid sync |
| ---: | ---: | ---: | ---: | ---: |
| 1.67 | 5.17 | 11.45 | 53.86 | 552 |

| 发散模式 | uniform | half | quarter | rotating |
| --- | ---: | ---: | ---: | ---: |
| ns/iter | 84.5 | 113.9 | 136.5 | 148.4 |
| 相对 uniform | 1.00 | 1.35 | 1.62 | 1.76 |

两臂等价、完全发散的 warp 理论上应该慢 2 倍，实测 `half` 只有 1.35 倍 —— 因为两臂的 FMA 链在迭代之间还能部分重叠。`__syncthreads` 的代价随 block 线程数增长（32→1024 慢 10 倍），grid sync 是 552 ns。

`warp_op`（结果串成依赖链的 warp 原语，`ns/op`）：`shfl_down/xor` 66.4 / 66.4、`ballot` 74.4、`any` 80.5、`all` 85.5、`reduce_add` 105。**这组只能当上限读**：循环里有一次按运行期参数做的 `switch`，分支开销和 warp 指令混在一起，没有分开。干净的循环基线是 `fma_latency` 的 `chain1` = 5.86 ns。

## 启动与驱动开销

CPU 侧只计 API 调用本身，每 32 次提交后在计时区外同步，`ns/launch` 越小越好：

| API | ns/launch |
| --- | ---: |
| `cuLaunchKernel`（driver API） | **1,227** |
| `cudaLaunchKernelExC` | 1,279 |
| `cudaLaunchKernel` | 1,287 |
| `cudaStreamCreate`（对照，非 launch） | 1,126 |
| `<<<>>>` 指定流 | 1,286 |
| `<<<>>>` 默认流 | 1,330 |
| `<<<256, 256>>>` 默认流 | 1,328 |
| `cudaEventRecord` | 494 |

从 `<<<>>>` 到 `cuLaunchKernel` 只差 8%，说明 C++ `<<<>>>` 包装层的开销不大，1.2 us 的地板是驱动和硬件提交本身。网格写 `<<<1,1>>>` 还是 `<<<256,256>>>` 没有差别（1328 vs 1330）。`cudaEventRecord` 494 ns，比一次 launch 便宜 2.7 倍。

CUDA Graph 是这一组最有冲击力的数字：

| | 每次 |
| --- | ---: |
| `cudaGraphLaunch`（64 个空节点） | **12.99 ns/节点** |
| `<<<>>>` 单独提交 | 1,330 ns/节点 |

**每节点便宜 102 倍。** 对小 kernel 密集的负载（比如每次只喂一个 warp 的细粒度任务），把 64 次提交折成一次 graph launch，CPU 侧的提交开销基本消失。

分配和同步（`ns`，越小越好）：

| | 4 KiB | 1 MiB | 64 MiB | 512 MiB |
| --- | ---: | ---: | ---: | ---: |
| `cudaMalloc` | 16,750 | 15,976 | 148,554 | 1,273,000 |
| `cudaFree` | 14,048 | 13,546 | 28,119 | 138,547 |
| `cudaMallocAsync` 提交 | 76,287 | 76,150 | 86,074 | 254,876 |
| `cudaMallocAsync` 到可用 | 76,516 | 76,532 | 86,571 | 255,285 |

`cudaMalloc`/`cudaFree` 会隐式同步整个设备，所以 4 KiB 也要 15 us 量级，512 MiB 是毫秒级。**反直觉的是 `cudaMallocAsync` 在所有尺寸上都比 legacy `cudaMalloc` 更慢**（76 us vs 16 us 起步），而且"只提交"和"到可用"几乎一样，说明慢的不是同步而是分配器本身。本微基准的循环是每次 alloc 后立刻 free 加 sync，stream-ordered 分配池会反复回收重建；真实负载里分配是成批的，这组数字不能直接套用。512 MiB 上 async 确实赢了（255 us vs 1.27 ms），大块分配才是它的适用区间。

`cudaEventSynchronize` 2,740 ns，`cudaEventElapsedTime` 55 ns。同步一次刚记录的 event 是一次到 GPU 的往返。

### 空 kernel 往返的长尾：1.76% 落在 130 us 的第二簇

`tail` 组跑 20005 次 `<<<1,1>>>` 空 kernel 加 `cudaDeviceSynchronize`，CPU 侧计时；同时用 kernel 第一条指令读 `%globaltimer` 拆出"从 CPU 发起到 GPU 开始执行"的部分。两个时钟的 ping-pong 可行区间宽 4864 ns，所以下面 `gpu_start_latency` 的绝对值有约 ±2.4 us 的不确定度，但**尾部结构不受影响**。

| | p50 | p90 | p99 | p999 | max | p99/p50 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `e2e_roundtrip`（ns） | 5,265 | 5,742 | 133,726 | 146,768 | 822,034 | **25.4x** |
| `gpu_start_latency`（ns） | 3,737 | 4,361 | 130,091 | 143,197 | 820,145 | 34.8x |

p50 到 p90 只涨 9%，是干净的单峰；然后在 p99 跳到 25 倍。按阈值数一遍：

| 阈值 | 占比 |
| --- | ---: |
| e2e > 2x p50（10.5 us） | 1.79% |
| e2e > 5x p50（26.3 us） | 1.77% |
| e2e > 10x p50（52.6 us） | 1.76% |
| e2e > 20x p50（105.3 us） | 1.42% |
| e2e > 50x p50（263 us） | 0.01% |

2 倍到 10 倍之间的占比几乎不变（1.79% → 1.76%），说明**慢的那批不是拖尾分布，而是落进一个独立的、约 130 到 150 us 的簇里**。约 1.76%（约每 57 次一次）的往返会额外付约 130 us。而且 `gpu_start_latency` 有完全相同的结构（130 us 簇），说明**额外的时间花在 GPU 真正开始执行之前**，也就是提交到开始之间，不是 kernel 执行或回程同步。

在 KVM guest 里，这个签名几乎可以确定是**虚拟中断注入 / VM exit 的周期性开销**：hypervisor 处理定时器或外设中断时，vCPU 被拖住，命令提交就慢一拍。1.76% 的频率与 guest 的时钟中断 / 调度节拍量级相符。

对照 2026-09-21 的 CPU 版"上下文切换"和"一次 syscall"：那边是 p50 量级的数字，没有做长尾。**这次 GPU 往返的 p50 只有 5.3 us，但 p99 是 134 us** —— 如果只报中位数，会完全看不到虚拟化的影响。这也解释了为什么 GPU 版的 README 里坚持"少量批次的最大值不是 p99"。

`clock_sync` 的 offset 行（`offset_arbitrary_epoch`）绝对值没有意义：`steady_clock` 从进程启动算，`%globaltimer` 从设备自己的纪元算，两者相差约 1.8e18 ns。它只在进程内用于换算 `gpu_start_latency`，window 才是同步质量。

## 主机-设备传输：链路是 x8，天花板 17 GB/s

PCIe 链路三个采样点（负载前 / 负载中 / 负载后）都是 **Gen5 x8**。理论上限 32 GT/s x 8 lane / 8 bit x 128/130 ≈ 31.5 GB/s。pinned 内存的有效载荷带宽：

| 工作集 | H2D | D2H | bidi | pageable H2D |
| ---: | ---: | ---: | ---: | ---: |
| 4 KiB | 4.0 | 5.7 | 5.4 | 4.4 |
| 64 KiB | **18.2** | **27.1** | **25.4** | 10.8 |
| 1 MiB | 13.5 | 18.5 | 15.2 | 13.6 |
| 4 MiB | 12.5 | 13.0 | 13.9 | 13.8 |
| 64 MiB | 13.6 | 16.6 | 15.8 | 7.7 |
| 256 MiB | 15.2 | 17.8 | 20.0 | 8.3 |
| 1 GiB | 16.5 | 17.3 | 19.4 | 9.5 |

大块传输稳在 **16 到 17 GB/s，是 x8 链路理论 31.5 GB/s 的约 53%**。如果这块卡在宿主机上是 x16，同样的测试大约会到 33 GB/s。64 KiB 那一档出现 27 GB/s 的峰值，是缓存在起作用，不能当链路带宽读。

双向（bidi）在 1 GiB 上是 19.4 GB/s，只比单向 H2D 的 16.5 高 18%；如果真的全双工应该是两者之和。**这条链路没有做到全双工**。

pageable 的 H2D 在 1 GiB 上只有 9.5 GB/s，是 pinned 的 58% —— 驱动经 pinned 中转缓冲再 DMA，多一次拷贝。README 里写的"pageable 的 `cudaMemcpyAsync` 并不异步"就是这个。

多流不涨：`multistream_H2D` 从 1 条流到 8 条流都是 13.3 到 13.6 GB/s，**确认只有一个 copy engine**，加流不会变快。

计算与传输重叠倒是完美：

| | 时间 |
| --- | ---: |
| 只跑计算 | 51.13 ms |
| 只做 256 MiB H2D | 17.58 ms |
| 两者同时 | 51.15 ms |
| efficiency = max(两者)/同时 | **0.9997** |

拷贝比计算短，同时跑的时间等于计算时间，说明 copy engine 与 SM 完全并行。这块是独立的硬件单元，和"单 copy engine"不冲突。

### 小传输的固定开销

同步 `cudaMemcpy`，每次单独计时，`ns/transfer` 越小越好：

| 大小 | pinned H2D | pinned D2H | pageable H2D |
| ---: | ---: | ---: | ---: |
| 1 B | 3,419 | 3,804 | 2,835 |
| 64 B | 3,759 | 3,897 | 3,164 |
| 1 KiB | 3,838 | 3,911 | 3,181 |
| 4 KiB | 4,362 | 4,018 | 3,487 |

1 B 到 1 KiB 之间几乎是平的（3.4 到 3.8 us），**固定开销约 3.4 us**，比一次 kernel launch 的 1.3 us 贵 2.6 倍。到 4 KiB 只涨了 943 ns（折合 4.3 GB/s），说明在这个区间里数据传输完全可以忽略。

有意思的是 pageable 的 1 B 反而比 pinned 便宜（2,835 vs 3,419 ns）：极小的 pageable 拷贝驱动会走一条不建 DMA 描述符的捷径，而 pinned 一定会建。要 4 us 以下的主机-设备往返，只能靠 pinned + 常驻缓冲 + 批量，或者干脆用 CUDA Graph 把多次提交合成一次。

### UVM 迁移：两项测量只在第一次有效

| | 256 MiB 的代价 |
| --- | ---: |
| `uvm_migrate +h2d`（主机写完，GPU 读） | 832 us（5 次都慢） |
| `uvm_migrate +d2h`（GPU 读完，主机读） | 中位数 20 ns，最大 65 到 76 us |
| `uvm_prefetch` | 中位数 1.5 到 1.7 TB/s，最小 579 GB/s |

**这三项有测量缺陷，只有第一次样本是真实的**：UVM 按页迁移，数据迁走之后再访问就不再迁移。`+h2d` 每次都用 kernel 读满整个 256 MiB，会把数据重新拉回设备，所以 5 个样本都慢，832 us 这个数可信（折合 324 GB/s，接近设备侧搬运速度）。`+d2h` 只读了首尾两个字节，所以只触及 2 页，而且第 0 个样本之后数据就常驻主机了 —— 中位数 20 ns 是空转，真实代价要看 max 列的 65 到 76 us，而且那也只是 2 页不是整块。`uvm_prefetch` 同理，中位数里的 TB/s 是"数据已经在目标位置"的空操作。

所以正确的读法是：**`+h2d` 的 832 us 可用；`+d2h` 和 `uvm_prefetch` 的中位数无效**，本次没有测到 256 MiB 的完整回迁代价。`madvise(MADV_HUGEPAGE/MADV_NOHUGEPAGE)` 在三种 `thp=` 设定下结果都在噪声范围内，UVM 很可能根本没理会它。

### zero-copy 走 PCIe 比走显存慢 5 倍

同样的指针追踪，节点放在 `cudaHostAllocMapped` 的主机内存里（`ns/load`，越小越好）：

| 工作集 | zero-copy（主机内存） | device memory 对照 |
| ---: | ---: | ---: |
| 4 KiB | 15.8 | |
| 1 MiB | 683 | |
| 32 MiB | 849 | 32 MiB: 140 |
| 256 MiB | | 316 |

命中设备 L2 的 4 KiB 档是 15.8 ns，和 device 内存的 32 KiB 档（15.6 ns）一样 —— 这档根本没走 PCIe。到 32 MiB 就是 849 ns，是同尺寸 device 内存 140 ns 的 **6 倍**。zero-copy 只适合极小、极高复用的元数据；批量数据必须先搬到显存。

### 主机注册的一次性成本

| | 1 MiB | 64 MiB | 512 MiB |
| --- | ---: | ---: | ---: |
| `cudaHostRegister` | 63.6 us | 1.25 ms | 9.52 ms |
| `cudaHostAlloc` | 263 us | 13.4 ms | 98.8 ms |

`cudaHostAlloc` 比 `cudaHostRegister` 贵 4 到 10 倍：前者要新分配并锁住主机页，后者只锁已有的页。512 MiB 上 `cudaHostAlloc` 要近 100 ms，且这些页会被 pin 住、阻止回收。

## 复现与检查

- 每个算术 kernel 都有自检：`a = b = 1` 时递推是 `x = x + 1`，从 `1 + lane` 出发走 7 步后精确等于 `8 + lane`，wrap 里 32 个 lane 求和可核对；wmma/FP8 的 MMA 按闭式核对累加器。自检不过直接 `fail`，不出 CSV。
- **`cuobjdump -sass` 核对过每个 kernel 发射的是 `FFMA`/`HFMA2` 而不是 `UFFMA`/`UHFMA2`。** 这一点是本次最大的坑：Hopper 之后 LLVM 的 uniformity 分析会把算子全是线程无关值的依赖链提升进 uniform 数据通路，测到的是 warp 共享的 UFFMA 而不是每 lane 的 CUDA core。修复前 FP32 只有 12.9 Tflop/s（理论的 54%）、`chain4` 比 `chain1` 还慢；给累加器一个依赖 `threadIdx.x` 的初值之后 FP32 变成 25.4 Tflop/s、`chain4` 正常快于 `chain1`。任何 GPU 算力微基准都应该做这个核对。
- `operations` 是 `value` 的分母这一条对所有行成立，正文比值都可从 CSV 复算。延迟类的分母是一条链的步数不是全批次总量。
- 已知测量缺陷三处，上文都标了：`uvm_migrate+d2h` 和 `uvm_prefetch` 只有第一次样本真实；`warp_op` 的循环含运行期 `switch`，`ns/op` 是上限；`clock_sync` 的 window 有 4864 ns，`gpu_start_latency` 的绝对值有约 ±2.4 us 不确定度。
- 没有锁频、没有隔离 GPU、没有停其他本机任务。`clock_ramp` 显示持续负载下 SM 时钟 2790 MHz、功耗 101 W（墙 180 W）、56 摄氏度，本次没顶到功耗墙。
- PCIe 是 Gen5 x8 而非 x16，F 组的天花板由链路决定。虚拟化长尾（1.76% 落在 130 us 簇）是 guest 环境特有的，host 侧对照等同一块卡上跑一次 `./run.sh` 后再补。

## 2026-09-22：CPU 访问主机内存的带宽

上面 CPU 版的 `memory()` 只测了单线程 libc memcpy，它自己标注了"不是整机 DRAM 带宽"。这一节补上整机 STREAM 式测量。代码：[microbench/stream.cpp](microbench/stream.cpp)，`make stream` 编译成 `stream.out`，无参数直接跑。

### 机器与实验条件

| 项目 | 本次配置 |
| --- | --- |
| 环境 | **KVM guest**，32 个 vCPU（无 SMT），`systemd-detect-virt` = kvm |
| 内存 | 12 GB 虚拟内存，运行时空闲 8.6 GB |
| 缓存 | L1D 32 KiB、L2 4 MiB x 32、**L3 16 MiB**（`lscpu` / sysfs） |
| 数组 | 512 MiB x 3 = 1.5 GiB，是 L3 的 32 倍 |
| 页 | THP 是 `madvise` 且未 `madvise`，所以是 **4 KiB 页**，与 CPU 版 memcpy 的 `MADV_NOHUGEPAGE` 同条件 |
| 编译 | 系统 g++ 15.2.1，`-O3 -std=c++20 -march=native` |
| 线程 | 每个线程 `pthread_setaffinity_np` 绑到不同 CPU，按编号顺序 |

**算不出理论峰值。** `dmidecode -t memory` 在 guest 里返回空（虚拟机不暴露 SMBIOS），物理 DIMM 的通道数、频率、位宽都在宿主机上看不到。不像显存那侧可以用 `2 × 显存时钟 × 位宽 / 8` 推出 448 GB/s，内存这侧**没有可声明的分母**，只能报实测上限。

### 结果：按实际访存流量换算，六个操作落在同一堵墙

`GB/s` 是 STREAM 口径的有效载荷，中位数，越高越好：

| 操作 | 1 线程 | 2 | 4 | 8 | 16 | 32 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| read | 16.55 | 25.01 | 28.01 | 29.01 | 28.82 | 27.25 |
| write | 12.99 | 15.10 | 16.21 | 15.56 | 13.64 | 14.03 |
| write_nt | **30.74** | 29.82 | 26.02 | 25.59 | 25.45 | 24.68 |
| copy | 17.49 | 20.98 | 23.48 | 20.32 | 18.25 | 20.21 |
| copy_nt | 26.36 | 26.77 | 25.04 | 23.28 | 22.66 | 23.22 |
| triad | 19.29 | 22.78 | 25.17 | 23.10 | 21.24 | 22.98 |

payload 的差是假象，真正的量是**实际 DRAM 流量**。普通 store 写一条不在自己手里的 cache line 要先把整条读进来（read-for-ownership），所以：

| 操作 | 每 8 字节的实际流量 | 流量系数 | 1 线程流量 | 32 线程流量 |
| --- | --- | ---: | ---: | ---: |
| read | 8 B 读 | 1.0 | 16.6 | 27.3 |
| write | 8 B RFO + 8 B 写回 | 2.0 | 26.0 | 28.1 |
| write_nt | 8 B 流式写，无 RFO | 1.0 | **30.7** | 24.7 |
| copy | 8 B 读 + 8 B RFO + 8 B 写回 | 1.5 | 26.2 | 30.3 |
| copy_nt | 8 B 读 + 8 B 流式写 | 1.0 | 26.4 | 23.2 |
| triad | 16 B 读 + 8 B RFO + 8 B 写回 | 1.33 | 25.7 | 30.6 |

**六行几乎完全一致地落在 24 到 31 GB/s**，这就是这台机器的访存上限。RFO 模型还被两个比值精确验证：

- `write / write_nt` = 12.99 / 30.74 = **2.37**，模型预测 2.0
- `copy / copy_nt` = 17.49 / 26.36 = **1.51**，模型预测 1.5

`copy` 那一对几乎是精确命中。所以"write 只有 13 GB/s 而 read 有 30"不是写带宽差，是 RFO 把写的有效载荷砍了一半。

两个读数要单独说明：

- **`read` 的单线程 16.55 是加法链的上限，不是访存上限。** 那个循环是 `sink += ...` 串行累加，FP 加法约 4 拍延迟，16 个 double 一组只有两个独立累加器。32 线程时 27.25 是靠更多线程把延迟藏起来的结果。真正的纯读上限看 `write_nt` 的 30.7（同样是单向流式流量）。
- **1 线程就能打满，多线程没有收益。** `write_nt` 从 1 线程的 30.74 反而降到 32 线程的 24.68。native 的双通道 DDR5 一般要 8 到 16 核才能推到 60 到 80 GB/s，这里 1 核就到顶、32 核反而略降，**最可疑的是 hypervisor 的内存虚拟化或宿主机侧的带宽限制**，但从 guest 里无法验证。所以不要把 30 GB/s 当成物理 DRAM 的极限，它是"guest 能看到的上限"。

### 和显存的对照

| | 实测 | 理论 | 利用率 |
| --- | ---: | ---: | ---: |
| 显存 read（1 GiB 工作集） | 412 GB/s | 448 GB/s | 92% |
| 主机内存实际流量 | 约 30 GB/s | 无法声明 | — |
| | | **显存 / 内存** | **约 13.7 倍** |

**显存比主机内存快约 14 倍。** 这就是为什么 GPU 的性能模型里"把数据搬上去一次、然后在卡上算"是唯一合理的形态：一条 16 GB/s 的 PCIe Gen5 x8 链路夹在 412 GB/s 的显存和 30 GB/s 的主机内存中间，每次搬运都是纯亏。

三方的量级排序（都按实测）：

```
显存        412 GB/s
PCIe Gen5x8  17 GB/s   (1 GiB pinned H2D，上面 F 组测的)
主机内存      30 GB/s   (实际访存流量)
```

注意主机内存的 30 比 PCIe 的 17 还快，所以**从主机内存读、算、写回主机内存**是有意义的；而**主机内存 → 显存 → 算 → 回主机内存**这条路一次搬运就要 1 GiB / 17 GB/s ≈ 62 ms，比在 CPU 上直接算同一块数据（1 GiB / 30 GB/s ≈ 35 ms 的访存时间）还慢。小批数据往返 PCIe 是 GPU 最经典的性能陷阱。

### 与 CPU 版 memcpy 的关系

CPU 版 256 MiB 单线程 libc memcpy 是 7.85 GB/s，独立复测 11.84 GB/s。本次 `copy` 单线程 17.49、`copy_nt` 26.36（都是 payload 口径）。量级对得上但差 1.5 刀 2 倍，来源有三：本次是编译器自动向量化的简单循环，libc memcpy 走 `rep movsb` 或 AVX 路径且可能用非临时存储；两次运行的机器负载不同（那条记录自己就标了 16 MiB 项波动 5.89 到 10.66）；工作集从 256 MiB 变成 512 MiB。**不要把其中任何一个当"内存带宽常数"**，这一节的 24 到 31 GB/s 是用六个不同访存模式交叉验证出来的，可信度高于任何单一数字。

### 已知测量缺陷

- 线程创建在计时区内。512 MiB 一趟约 25 ms，32 个线程的创建开销约 0.3 ms，占比约 1%，1 线程时占比 0.04%。
- `kInner = 1`，每个样本只扫一遍 512 MiB。样本区间（表格之外的 min/max）显示 4 到 8 线程有 20% 左右的抖动，是这台正在使用的机器的常态，和 CPU 版那次记录一致。
- 没有测巨大页（THP 是 `madvise` 未启用）。`madvise(MADV_HUGEPAGE)` 之后 TLB 压力下降，上限可能更高，本次没做。
- 32 个 vCPU 是 guest 呈现的合成拓扑（无 SMT、L3 只有 16 MiB），与 2026-09-21 那次记录里"8 P 核 + 16 E 核、共享 L3 36 MiB"的描述不同。两边的 CPU 相关数字不能直接拼接，除非先确认两次是不是同一个虚拟机配置。


## 2026-09-23：13900K 物理机的内存带宽上限实测

上一节确实是在 KVM guest 中测试；本节在当前 13900K 裸机上独立复测。
**程序持续纯读约 30 GB/s，最高单批 31.11 GB/s；硬件计数器测到整机约
32 GB/s。当前内存配置的理论峰值为 38.4 GB/s。**

SMBIOS 显示四条 32 GiB 光威 DDR4，当前全部运行在 **2400 MT/s**。
13900K 是双通道，每通道两条内存，不是四通道，因此
`2400e6 × 8 B × 2 = 38.4 GB/s`。

修改后的 [stream.cpp](microbench/stream.cpp) 用四个 AVX 累加器读取，
按物理核优先绑核，在线程创建、首次缺页、页合并和预热之后计时，
复用工作线程并在结束后校验全部数组。明确区分 4 KiB、THP 和 HugeTLB，
大页覆盖从本次映射的 `smaps` 核实。测试七类操作，并补测 P 核 SMT 与 E 核。

| 配置 | read 中位数 GB/s | 样本最高 GB/s |
| --- | ---: | ---: |
| 512 MiB/数组，4 KiB 页，8 P 核，独立复测 | 29.77 | 30.41 |
| 1 GiB/数组，4 KiB 页，8 P 核 | 29.49 | 30.39 |
| 512 MiB/数组，4 KiB 页，8 P 核 × 2 SMT | 29.12 | 29.35 |
| 512 MiB/数组，4 KiB 页，16 E 核 | 28.89 | 30.15 |

每配置 7 个样本；512 MiB 每样本扫描 4 遍，1 GiB 每样本扫描 8 遍。
全部正式扫描的最高单批是 **31.11 GB/s**，来自初次 512 MiB、4 KiB、8 P 核配置；
不把这个单批最大值当成持续保证。更多线程未明显突破约 30 GB/s。

**大页未获得完整的正式对照。** THP 的 512 MiB 测试实际覆盖约 25%–30%，
1 GiB 复测约 14%–15%；`MADV_COLLAPSE` 返回 `ENOMEM`。显式 HugeTLB 尝试
预留 3 GiB，实际只拿到 138 MiB，不足以容纳三个大数组。小工作集的全大页
功能验证通过，但属于缓存大小测试，不纳入 DRAM 峰值。两次临时大页池修改
均已恢复为 0；未重启、清缓存或暂停其他任务。不能从部分 THP 数据给出
“大页能提升多少”的确定结论。

**程序带宽与整机流量不同。** 不运行本 benchmark 的 5 秒对照中，IMC 仍测到
约 13.84 GB/s 的整机流量。在另一次 1 GiB 长读测试中，程序中位数为 23.40 GB/s，
同一运行的稳定读取窗口内，两个内存控制器合计 **31.64–32.67 GB/s，中位数
32.28 GB/s**，达到理论峰值的约 84%。这些系统级计数包含后台活动，不能把不同
时刻的背景流量直接加到程序读数上。普通 copy/triad 显示值还未包括典型 RFO
流量，因此也不能直接把显示值当作总线利用率。

完整的 167 配置、1171 样本、页覆盖、失败与恢复记录及复现命令见
[物理机测试报告](microbench/results/20260923-093239-stream-host/README.md)、
[汇总 CSV](microbench/results/20260923-093239-stream-host/summary.csv) 和
[检查记录](microbench/results/20260923-093239-stream-host/validation.txt)。代码通过数值校验、非整除线程分块、
CPU affinity 限制及 AddressSanitizer 检查。结果反映当前工作站负载，
不声称测得了完全空闲环境的绝对硬件极限。

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
