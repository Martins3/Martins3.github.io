# GPU microbench

对应 [个人性能测试记录](../my-result.md)。Linux x86-64、CUDA C++20、无第三方 benchmark 库，形态与 [microbench/](../microbench/README.md) 平行：每组实验是一个独立函数，stdout 只出 CSV，样本全保存，正文只报中位数和必要的 min/max。

本机是 **KVM guest，RTX 5060 Ti（Blackwell，`sm_120`）经 VFIO 直通**，另有一块 virtio-gpu。单卡、无 NVLink。直通的 PCIe 链路是 **Gen5 x8**（上限 Gen5 x16），F 组的带宽上限由它决定，不是显存。

## 编译与运行

```bash
cd /home/martins3/data/vn/docs/benchmark/gpu-microbench
./run.sh                    # 重新编译、记录环境、全量采样，打印结果目录
make smoke                  # --quick --samples 1，只做功能检查
./gpubench.out --group compute --samples 5 > compute.csv
./gpubench.out --group memory --samples 5 > memory.csv
```

`run.sh` 在 `results/日期时间.随机后缀/` 下写 `environment.txt`、`samples.csv`、`run.log`、`load-after.txt`。不会清理系统缓存、锁频、关闭安全防护或者重启机器。二进制以 `.out` 结尾。

Makefile 用 `-ccbin /usr/bin/g++ -Xcompiler -B/usr/bin/` 保持系统编译器和链接器配套，避免仓库的 Nix shell 注入另一版本 glibc。目标架构默认 `SMS=120`，换卡时 `make SMS=89`。链接 `-lnvidia-ml` 用于遥测和 PCIe 链路状态；没有 NVML 时对应功能自动跳过并在 stderr 提示，不填零。

`--quick` 把大部分循环次数除以 10，A2 的频率爬坡缩短到 1 s，只用于功能检查。`--group NAME` 只跑一组（`device compute memory exec launch pcie`），`--device N` 选卡，`--samples N` 每个配置的批次数。**每个吞吐组开始前 `warm_clocks()` 先压 1.5 s 负载**，否则第一项测到的是从空闲 P-state 爬频的过程。

## CSV 字段

`group,case,config,working_set_bytes,sample,operations,elapsed_ns,unit,value`

`config` 用 `+` 和 `=` 连接变体名，避免与 CSV 分隔符冲突。`sample` 从 0 开始。**`operations` 是 `value` 的分母**：

| unit 方向 | 关系 | 含义 |
| --- | --- | --- |
| `Gflop/s` `Gop/s` `GB/s` | `value = operations / elapsed_ns` | 吞吐。ns→s 的 1e9 与 G 的 1e9 抵消，所以比值直接是 G 单位 |
| `ns/update` `ns/load` `ns/transfer` `ns/launch` `ns/barrier` `ns/op` | `value = elapsed_ns / operations` | 延迟 |

延迟类的 `operations` 是**一条链/一次操作**的计数，不是全批次总量：多个线程各跑一条依赖链时，它们并发推进，`elapsed_ns / 一条链的步数` 才是链上每步的推进时间。吞吐类的 `operations` 才是全批次总量。`unit` 为 `count` `MHz` `W` `C` `ratio` `warps/sm` `gen` `lane` `bool` `cc` 的行是属性或遥测，`operations=1`。

对 `(group, case, config, working_set_bytes, unit)` 分组后取 `value` 的中位数即可复现正文统计。

## 实验与计量边界

### A / `grp_device.cu` — 设备基线

| 分组 / 实验 | 测试方法 | 输出和限制 |
| --- | --- | --- |
| `attribute` | `cudaGetDeviceProperties` 与 `cudaDeviceGetAttribute` | 静态规格。`cuda_cores_per_sm_assumed` 和 `fp64_fp32_ratio_assumed` 是**公开资料假设**，不是查询所得；CUDA 不暴露每 SM 的 CUDA core 数和 tensor core 数 |
| `theoretical` | 由上述属性推算 | `dram_bandwidth = 2 * mem_clock * bus_width / 8`（GDDR 每时钟双倍率）。`fp32_peak = SM 数 * 128 * 2 * 额定时钟`。这些是后面所有利用率的分母，B1 的实测峰值应当接近它。公式的来历、计数约定和利用率的读法见 [../peak-flops.md](../peak-flops.md) |
| `clock_ramp` | 持续 FP32 负载，NVML 采样 SM/显存频率、功耗、温度、PCIe 代数和宽度 | 采样发生在批次边界，爬频过程中的中间值看不到。分 `cold_start`（前 12 个样本）和 `settled` 两段 |

### B / `grp_compute.cu` — 算力与指令延迟

| 分组 / 实验 | 测试方法 | 输出和限制 |
| --- | --- | --- |
| `fma` `mad` | 内联 PTX 的 `fma.rn.ftz.f32/f64`、`fma.rn.f16x2/bf16x2`、`mad.lo.s32`，每线程 8 条独立链 | `Gflop/s`（整数是 `Gop/s`）。`f16x2`/`bf16x2` 一条指令是两个 MAC，`operations` 按 MAC 计，所以权重是 4 flops。测的是 FMA/MAD 发射吞吐，不是单条指令延迟 |
| `sfu` | `sin/cos/rsqrt.approx.ftz.f32`，8 条独立链 | `Gop/s`。这些是近似指令，自检只验证结果有限。**`ex2`/`lg2` 没做**：迭代无界，需要中途复位，会污染计时 |
| `fma_latency` | `chain1` 一条依赖链 / `chain4` 四条独立链，与 CPU 版 `chain`/`ilp4` 同名同方法 | `ns/update`，一次 update 是一次乘加。**包含循环开销，不是单条 FFMA 的指令延迟**。`chain4` 的分母是 `iters * 4`，是 ILP=4 下摊到每次乘加的时间 |
| `mma` | `wmma.mma_sync`（编译后是 `mma.sync`），A/B 留在寄存器，累加器 `acc8` 条独立链 | `Gflop/s`。测的是 MMA 流水线发射率，不是访存。**累加器 4 条链和 8 条链结果相同，说明瓶颈不在累加器依赖上**。`acc8` 和 `acc1` 的差距在 `mma_latency` |
| `mma` / FP8 | wmma 没有 FP8 fragment 类型，走裸 PTX `mma.sync.aligned.m16n8k32.row.col.f32.e4m3/e5m2.f32`。A/B 各打包 4 个相同字节，D 与 C 同寄存器原地累加 | `Gflop/s`。`m16n8k32` 的 fragment 是 A = 4 x `.b32`、B = 2 x `.b32`、C/D = 4 x `.f32`，16x8 累加器每 warp 128 个 f32，与自检核对。FP8 精度很低，A = B = 0.5 时乘积项 `K/4 = 8` 仍然精确 |
| `mma_latency` | 同上但累加器只有 1 条，形成依赖链 | `ns/update`，一次 update 是一条 warp 级 mma。**2304 个 warp 并发共用每 SM 的 tensor core，这不是孤立的 mma 依赖延迟** |

wmma 形状是固定的 `m16n16k16`（tf32 是 `m16n16k8`），16x16 累加器；FP8 没有 wmma fragment，只能用裸 PTX `mma.sync` 的 `m16n8k32`，16x8 累加器。没有用 cute/cutlass。每种 dtype 先跑一次自检，不对就 `fail`，不输出结果：算力 kernel 的递推在 `a = b = 1` 时是 `x = x + 1`，从 `1 + lane` 出发走 7 步后精确等于 `8 + lane`，warp 内 32 个 lane 求和可核对；wmma/FP8 的 MMA 按闭式核对累加器（`A = B = 0.5` 时乘积项是 `K/4`）。

### C / `grp_memory.cu` — 存储层次

| 分组 / 实验 | 测试方法 | 输出和限制 |
| --- | --- | --- |
| `bandwidth` | `read`/`write`/`copy`，工作集 1 KiB 到 1 GiB（float 元素数 × 4），每批重复若干遍 | `GB/s` 是**有效载荷字节/时间**，十进制 GB。实际访存流量包含读、写和 RFO。CSV 的工作集是单个缓冲区，`copy` 的源+目标占两倍。**全部是稳态（热缓存）测量**，工作集扫描的台阶就是层次结构；没有逐次加载前的冷测量，因为在单次依赖加载前冲刷 L2 测到的是冲刷本身 |
| `chase` | 64 B 节点随机环指针追踪，工作集 32 KiB 到 256 MiB，另有 `ldg`/`ldca`/`ldcs`/`ldlu` 只读缓存路径对比 | `ns/load`。1 个 block × 256 线程，每线程一条严格依赖链。**256 条链并发，`elapsed_ns / steps` 是并发压力下每条链的推进时间，不是孤立加载延迟**；单线程版本会掉频，数字更不可信。节点大小与 CPU 版 `chase()` 一致，两者可并排读 |
| `smem_bandwidth` | shared memory 步长扫描 1..32，索引随迭代游走防止编译器外提 | `GB/s`。stride=1 无冲突，stride=2 两路，stride=32 对 f32 是 32 路冲突。是带宽视角，不是单次访问延迟 |
| `atomic` | `global+contended/sharded/random`、`shared+contended`、`l2+contended`（access policy window 常驻 L2） | `Gop/s` 是 RMW **吞吐**，不是单次原子操作的响应时间。contended 与 sharded 一起看才能分出一致性开销 |
| `tile_copy` | 同样大小的 tile 从 global 搬进 shared 的四条路径：`ld_st` 普通读写、`cp_async`（`cp.async.ca.shared.global`，每线程 16 B）、`tma`（`cp.async.bulk.tensor.1d`，逐 tile 串行）、`tma_p2`（双缓冲流水） | `GB/s`。TMA 的 1-D box 上限 256 个元素，4 KiB tile 是 4 次 box 发射，但 `mbarrier.arrive.expect_tx` 必须声明**整个 tile** 的字节数，只声明一个 box 会让多余完成量落进下一个 phase 而死锁。**这是纯 tile 搬运带宽，不含重叠**：TMA 的价值在释放 LSU/发射槽、地址生成不占 SM，以及与计算重叠，这些本微基准都不计价。`tma_p2` 只反映双缓冲带来的重叠收益 |

### D / `grp_exec.cu` — 执行模型

| 分组 / 实验 | 测试方法 | 输出和限制 |
| --- | --- | --- |
| `occupancy` | `cudaOccupancyMaxActiveBlocksPerMultiprocessor` | `warps/sm`，每 SM 的活动 warp 数 |
| `occupancy_sweep` | 固定每线程 4 条依赖 FFMA，扫 `tpb` 32..1024；再固定 tpb=256 扫动态 shared memory 0..128 KiB | `Gflop/s`。看延迟隐藏何时饱和。SMEM 取 2 的幂，保证 kernel 里的索引掩码精确；>48 KiB 需要 `cudaFuncSetAttribute`，被拒就跳过并在 stderr 说明 |
| `barrier` | `__syncthreads`、`__syncwarp`、cooperative `this_grid().sync()` | `ns/barrier`。是波内最慢 block 看到的屏障代价，常驻 block 并行等待，计数不求和。需要 `cooperativeLaunch`，没有就跳过 `grid_sync` |
| `divergence` | 两臂各 4 条依赖 FFMA，uniform / 静态一半 / 四分之一 / 谓词逐迭代旋转 | `ns/iter`。完全发散的 warp 大约是 uniform 的两倍（两臂串行执行），测的是这个组合代价，不是单独的分支预测失败惩罚 |
| `warp_op` | `shfl_down/xor`、`ballot`、`any`、`all`、`reduce_add`，结果串成依赖链 | `ns/op` 是 warp 指令的**延迟**，不是峰值发射率的倒数 |

### E / `grp_launch.cu` — 驱动与启动开销

| 分组 / 实验 | 测试方法 | 输出和限制 |
| --- | --- | --- |
| `submit` | 只在 CPU 侧计时 API 调用本身。每 32 次提交后同步一次，同步在计时区外，队列不会积压 | `ns/launch`。`api=chevron` / `chevron_stream` / `cudaLaunchKernel` / `cudaLaunchKernelExC` / `cuLaunchKernel` 对比包装层开销；`chevron_grid=256x256` 看网格描述符的影响。`cudaFunction_t` 与 `CUfunction` 都是 `CUfunc_st*`，所以 `cudaGetFuncBySymbol()` 就能给出 `cuLaunchKernel` 要的句柄，不必 `cuModuleLoad` 加载 cubin |
| `clock_sync` | 64 轮 ping-pong，每轮给出 `offset = cpu_ns - %globaltimer` 的可行区间 `(t0 - ts, t1 - ts)`，取交集 | `window` 是交集宽度，小说明两个时钟钉得准；`offset_arbitrary_epoch` 的**绝对值没有意义**（本程序的 `steady_clock` 从进程启动算，`%globaltimer` 从设备自己的纪元算），它只在进程内用来换算 `gpu_start_latency` |
| `tail` / `e2e_roundtrip` | 空 kernel `<<<1,1>>>` + `cudaDeviceSynchronize` 的完整往返，CPU 侧计时 | `ns`。含提交、队列、GPU 执行、回程中断和同步 |
| `tail` / `gpu_start_latency` | kernel 第一条指令读 `%globaltimer`，与 CPU 的 `t0` 相减（用上面的 offset 换算） | `ns`，从 CPU 发起到 GPU 开始执行。**依赖 `window` 足够小**；window 有几十微秒时这个数不能当绝对值读 |
| `tail_summary` | 上面两项的 p50/p90/p99/p999/max | 全部原始样本在 `tail` 里，这里只是索引。样本量 20000，p999 只有 20 个点支撑 |
| `event_record` `event_sync` `event_query` | 重复记录同一个 event / `cudaEventSynchronize` / `cudaEventElapsedTime` | `ns`。重复记录一个 event 是合法且常见的写法，所以测的就是它 |
| `alloc` `free` | `cudaMalloc`/`cudaFree`（会隐式同步，时间含排空在途工作）、`cudaMallocAsync` 分成 `async_submit`（只测提交）和 `async_ready`（到可用为止） | `ns` |
| `stream_create` `stream_destroy` | `cudaStreamCreate`/`Destroy` | `ns` |
| `graph_launch` | 捕获 64 个空 kernel 成 CUDA Graph，反复 `cudaGraphLaunch` | `ns/launch` 且 `operations=64`，即**每节点**成本，可直接和 `submit` 的每次提交成本对比 |

### F / `grp_pcie.cu` — 主机-设备传输

| 分组 / 实验 | 测试方法 | 输出和限制 |
| --- | --- | --- |
| `link` | NVML 的当前 PCIe 代数和宽度，分别在负载前、负载中、负载后采样 | 和 `environment.txt` 里 sysfs 的 `current_link_*` **不一致是正常的**：guest 的 sysfs 是虚拟链路视图，NVML 是驱动视图。以实际带宽为准 |
| `bandwidth` | `pageable`/`pinned`/`pinned_wc` × `H2D`/`D2H`/`bidi`，4 KiB 到 1 GiB | `GB/s` 是有效载荷。`bidi` 的 payload 每遍是 2×，两条流同时在飞。**pageable 的 `cudaMemcpyAsync` 并不异步**：驱动经 pinned 中转缓冲并同步，所以 pageable 行里含那次中转拷贝 |
| `small_transfer` | 同步 `cudaMemcpy`，1 B 到 4 KiB，每次单独计时 | `ns/transfer`，是完整固定开销：API 调用、同步、描述符和数据。曲线平坦段是开销，斜率是链路带宽的倒数 |
| `multistream_H2D` | 1/2/4/8 条流各传 64 MiB | `GB/s` 是聚合带宽。单 copy engine 不会因为流多而变快，这一项就是验证这一点 |
| `overlap` | 计算 kernel 与 H2D 拷贝分别单独测，再在两条流上同时跑 | `efficiency = max(计算, 拷贝) / 同时`，1.0 是完美重叠，0.5 是完全串行。三项都用 legacy 默认流上的 event 计时，它对两条工作流都有序，所以时间跨度覆盖最晚完成的那个 |
| `host_register` | `cudaHostRegister`/`Unregister`、`cudaHostAlloc`/`FreeHost` | `ns`。一次性成本，但会 pin 主机页、阻止回收 |
| `uvm_migrate` | `cudaMallocManaged` 后先主机写、再 GPU 读（`h2d`）；然后主机读（`d2h`）；另有 `uvm_prefetch` 主动迁移 | `ns`（prefetch 是 `GB/s`）。**UVM 可能无视 `madvise(MADV_HUGEPAGE/MADV_NOHUGEPAGE)`**，所以 `thp=default/huge/never` 三种都报，不假定生效 |
| `zero_copy_chase` | 同 C2 的指针追踪，但节点在 `cudaHostAllocMapped` 的主机内存里 | `ns/load`。与 C2 同形状可比。命中设备 L2 时不是 PCIe 往返，所以按工作集分档 |

## 防止测到错误东西

**不要把整条链测到 uniform 管线上。** 这是本套件踩过的最大的坑。Hopper 之后 LLVM 会做 uniformity 分析：如果一条依赖链的算子全部是线程无关值（常量、kernel 参数），整个链会被提升进 warp 内共享的 uniform 数据通路，发射 `UFFMA` 而不是每 lane 的 `FFMA`。两者是不同的单元，吞吐差近一倍。症状很典型：`chain4` 比 `chain1` 还慢、实测峰值只有理论值的 54%。修法是给累加器一个依赖 `threadIdx.x` 的初值（`1 + lane`），数学上仍可精确自检。**任何 GPU 算力微基准都该用 `cuobjdump -sass` 核对一遍发射的是 `FFMA` 还是 `UFFMA`**，本套件的每个 kernel 都核对过。

其他已经守住的边界：

- 每个算术 kernel 都有自检：`a = b = 1` 时递推是 `x = x + 1`，从 `1 + lane` 走 N 步后精确等于 `1 + lane + N`，warp 内 32 个 lane 求和可核对。打包格式按两个 half 分别解码后核对，不是比位型。wmma 按闭式核对累加器。自检失败直接 `fail`，不出 CSV。
- 内联 PTX 保证指令形态，`asm volatile` 防止被消除或合并。**热循环里的 asm 不能带 `"memory"` clobber**：它会让编译器在每条 FMA 之间把累加器数组从栈上重载，把 ILP 全部毁掉。
- 计时双路：GPU 侧 `cudaEvent`，CPU 侧 `steady_clock`。启动开销只用 CPU 侧且把同步放在计时区外；吞吐只用 GPU 侧。两个时钟的换算用 ping-pong 取可行区间，`window` 太宽时不下绝对结论。
- 吞吐组开始前 `warm_clocks()` 先压 1.5 s。consumer 卡从空闲 P-state（本机 180 MHz）爬到 boost 需要几十毫秒以上，第一项不预热测到的就是爬频过程。
- 理论峰值与实测分开报，`cuda_cores_per_sm_assumed` 这类假设单独出一行，不混进测量值。`operations` 是 `value` 的分母这一条对所有行成立，正文的比值都可从 CSV 复算。
- 指针追踪、`fma_latency`、`mma_latency` 这类并发链的延迟，都写明是并发压力下的推进时间，不叫孤立延迟。
- `f16x2`/`bf16x2` 在大迭代数下累加器会饱和到 half 的整数精度上限，但 `HFMA2` 是定延迟流水操作，饱和不影响测得的发射速率；自检用小迭代数保证精确。
- CSV 保存每次样本，正文给中位数和必要的 min/max。少量批次的最大值不是 p99；只有 `tail_summary` 的 p99/p999 配了 20000 个样本，且 p999 只有 20 个点支撑。
- 没有锁频（`nvidia-smi -lgc`）、没有隔离 GPU、没有停其他本机任务。功耗墙 180 W 会限制持续吞吐，`clock_ramp` 记录的就是这个过程。结论只适用于记录中的这次实验。
- 本机是 VM，PCIe 是 Gen5 x8 而非 x16，F 组的天花板由链路决定。`link` 行和 `environment.txt` 里的 sysfs/NVML 两个视图一起看。

## 与虚拟化对照的关系（G 组，接口已留）

`run.sh` 的 `record_environment()` 特意把 `systemd-detect-virt`、IOMMU group 列表、`dmesg` 的 IOMMU/vfio 行、NVIDIA 功能的 sysfs 链路速度与宽度、驱动绑定都写进 `environment.txt`，格式对 guest 和 host 一致。在同一块卡上于宿主机再跑一次 `./run.sh`，两份 `environment.txt` 与 `samples.csv` 可直接对比，不需要改代码。

`tail` 组的 20000 个往返样本就是为虚拟中断注入的长尾准备的：guest 里 p999 相对 p50 的放大倍数，对照 host 的同一比例，才是 hypervisor 的贡献。这部分结论等 host 侧跑完再写进 [my-result.md](../my-result.md)。

virtio-gpu（`00:0e.0`）没有 CUDA，无法进这套微基准；它的图形/计算路径对照需要另一套 Vulkan 测试，不在本套件内。

## 明确不做

- 多卡 / NVLink / NCCL（单卡）
- 端到端训练、推理、GEMM 调优、cuBLAS 对照（那是 macrobench，不是 microbench）
- 图形管线微基准（GL/Vulkan 管线、vsync、延迟到显示）
- `ex2`/`lg2` SFU（迭代无界，中途复位会污染计时）
- 冷缓存逐次加载延迟（冲刷 L2 本身会成为被测对象）
- gpu-burn 式烤机

## 与原始笔记的关系

[../benchmarks.md](../benchmarks.md) 列的是通用工具。这一套不替代 `cuda-samples` 的 `bandwidthTest`/`deviceQuery`，也不替代 `nvbench` 或 `cutlass` profiler：它要的是和本仓库 CPU 版**同方法、同 CSV 约定、可并排读**的一组数字，以及每个数字明确的"测到的是什么/不是什么"。

`fma_latency` 的 `chain1`/`chain4` 与 CPU 版 `compute()` 的两条依赖链/四条独立链同名同方法，两边的 `ns/update` 可以直接对比，只是注意 CPU 版是 64 位整数乘加、这里是 FP32 FMA。`chase` 的 64 B 节点随机环也与 CPU 版 `chase()` 同构。

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
