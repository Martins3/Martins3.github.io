# Nsight Systems / Nsight Compute 实验

关键参考:
- https://docs.nvidia.com/cuda/parallel-thread-execution/#scalable-data-parallel-computing-using-gpus


## 实验

这里只保留一个 CUDA 文件和一个 Bash 入口：

```bash
./demo.sh
```

默认启用 green context；只使用普通 CUDA context 时执行：

```bash
USE_GREEN_CONTEXT=0 ./demo.sh
```

脚本依次完成：

1. 用 `-O2 -lineinfo` 编译 `gpu_debug_demo.out`；
2. `run_nsys()` 采集 CUDA、NVTX 和 OS runtime 时间线；
3. `run_ncu()` 采集 DRAM、L2、L1/TEX、Compute 和 cluster kernel；
4. 在终端打印两份报告的摘要。

| kernel | 目标 | 本机实测峰值占比 |
| --- | --- | --- |
| `bench_dram()` | DRAM | 91% |
| `bench_l2()` | L2 | 75% |
| `bench_l1()` | L1/TEX | 95% |
| `bench_compute()` | FP32 Compute | 98% |

所有 kernel 都在由 32 个 SM（本机共 36 个 SM）组成的 green context 中运行。
`bench_cluster()` 每个 cluster 包含 2 个 thread block，并通过
`cluster.sync()` 和 distributed shared memory 读取同一 cluster 内另一个 block
的 shared memory。

在 NCU 的 Launch Statistics 和 Occupancy 中检查：

```text
Context 2 (GC)
# SMs                         32
Uses Green Context            1
Cluster Size                  2
Max Active Clusters          36
Waves Per SM                  2
Cluster Occupancy          8.33%
Achieved Occupancy          ~29%
```

`Cluster Size` 只在 `bench_cluster()` 中非零；普通 kernel 即使运行在 green context
中，该项仍为 0。`Cluster Occupancy` 是 cluster 放置约束造成的 active block 比例，
不是 grid 工作量；把 grid 从 64 个 block 增加到 384 个 block 后，`Waves Per SM`
从 0.33 增加到 2，而该值仍为 8.33%。

`Duration` 和 `SM Active Cycles` 是耗时与周期计数，不是利用率，循环次数只控制它们的大小。

报告位于：

```text
reports/gpu-debug-demo.nsys-rep
reports/gpu-debug-demo.ncu-rep
```

打开 GUI：

```bash
nsys-ui reports/gpu-debug-demo.nsys-rep
ncu-ui reports/gpu-debug-demo.ncu-rep
```

本机驱动只允许管理员读取 GPU performance counter，因此脚本只在执行 `ncu` 时调用
`sudo`；`nsys`、编译和报告解析都使用当前用户。

- [Nsight Systems User Guide](https://docs.nvidia.com/nsight-systems/UserGuide/)
- [Nsight Compute CLI](https://docs.nvidia.com/nsight-compute/NsightComputeCli/)

## 关键结论

- `cuda-gdb` 使用 debug build：`-g -G -O0`。
- `ncu/nsys` 使用 profile build：`-O2 -lineinfo`。
- 不要用 `-G` 得出的结果判断性能。
- 当前机器上不要继续使用 `nvprof`，用 `nsys` 替代 timeline，用 `ncu` 替代 kernel metrics。

## GUI 打开报告

```sh
/usr/local/cuda/bin/nsys-ui /tmp/cuda-debug-basic/vector_add_nsys.nsys-rep
/usr/local/cuda/bin/ncu-ui /tmp/cuda-debug-basic/vector_add_ncu.ncu-rep
```

## 官方文档

- CUDA-GDB: https://docs.nvidia.com/cuda/cuda-gdb/index.html
- Compute Sanitizer: https://docs.nvidia.com/compute-sanitizer/ComputeSanitizer/index.html
- Nsight Systems: https://docs.nvidia.com/nsight-systems/UserGuide/
- Nsight Compute CLI: https://docs.nvidia.com/nsight-compute/NsightComputeCli/

## 这里还有一个
https://github.com/mikesart/gpuvis

## bpf 方法也是可以的
https://mp.weixin.qq.com/s/-K4QkPXY1DKPd6bq5rdrKA

类似有不少类似的项目: https://github.com/eunomia-bpf/eGPU

## 基本命令
```txt
	# Nsight Systems 采集进程 timeline。报告里应该能看到 cudaLaunchKernel、
	# cudaMalloc、cudaMemcpy、cudaDeviceSynchronize 等 CUDA API，以及
	# 捕获到的 vector_add_kernel launch。
	"${NSYS}" profile \
		--force-overwrite=true \
		--trace=cuda,nvtx,osrt \
		--sample=none \
		--output="${NSYS_OUTPUT}" \
		"${DEFAULT_BIN}"

	# Nsight Compute 聚焦单个 kernel。vector_add demo 故意使用很小的
	# grid/block 配置，所以低 occupancy 或 block size 不是 32 倍数的提示
	# 是教学 demo 的预期现象，不是驱动或 GPU 识别问题。
	#
	# 真实程序里常用的变体：
	#   ncu --set basic ./app
	#   ncu --set full ./app
	#   ncu --kernel-name regex:my_kernel ./app
	#   ncu --launch-skip 10 --launch-count 1 ./app
	#   ncu --metrics sm__throughput.avg.pct_of_peak_sustained_elapsed ./app
	#
	# 如果硬件计数器权限阻止采集，可以只对该命令使用 sudo 运行 ncu；
	# 或者明确修改 NVIDIA 驱动的 profiling 策略。后者在当前物理机上属于
	# 系统配置变更。
	"${NCU}" \
		--target-processes all \
		--set basic \
		--kernel-name-base function \
		--kernel-name regex:vector_add_kernel \
		--launch-count 1 \
		--force-overwrite \
		--export "${NCU_OUTPUT}" \
		"${DEFAULT_BIN}"
```

## ncu-stats 结果分析

```txt
env -u LD_LIBRARY_PATH -u LIBRARY_PATH -u NIX_LDFLAGS -u NIX_CFLAGS_COMPILE -u NIX_ENFORCE_NO_NATIVE PATH=/usr/local/cuda-13.1/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin "/usr/local/cuda-13.1/bin/ncu" --import "reports/gpu-debug-demo.ncu-rep" --page=details
[135962] gpu_debug_demo.out@127.0.0.1
  vector_add(const float *, const float *, float *, unsigned long) (65536, 1, 1)x(256, 1, 1), Context 1, Stream 7, Device 0, CC 12.0
    Section: GPU Speed Of Light Throughput
    ----------------------- ----------- ------------
    Metric Name             Metric Unit Metric Value
    ----------------------- ----------- ------------
    DRAM Frequency                  Ghz        13.79
    SM Frequency                    Ghz         2.40
    Elapsed Cycles                cycle    1,095,336
    Memory Throughput                 %        91.28
    DRAM Throughput                   %        91.28
    Duration                         us       455.58
    L1/TEX Cache Throughput           %        13.76
    L2 Cache Throughput               %        26.78
    SM Active Cycles              cycle 1,079,197.22
    Compute (SM) Throughput           %        10.65
    ----------------------- ----------- ------------

    INF   This workload is utilizing greater than 80.0% of the available compute or memory performance of this device.
          To further improve performance, work will likely need to be shifted from the most utilized to another unit.
          Start by analyzing DRAM in the Memory Workload Analysis section.

    Section: Launch Statistics
    -------------------------------- --------------- ---------------
    Metric Name                          Metric Unit    Metric Value
    -------------------------------- --------------- ---------------
    Block Size                                                   256
    Cluster Scheduling Policy                           PolicySpread
    Cluster Size                                                   0
    Function Cache Configuration                     CachePreferNone
    Grid Size                                                 65,536
    Preferred Cluster Size                                         0
    Registers Per Thread             register/thread              16
    Shared Memory Configuration Size           Kbyte           16.38
    Driver Shared Memory Per Block       Kbyte/block            1.02
    Dynamic Shared Memory Per Block       byte/block               0
    Static Shared Memory Per Block        byte/block               0
    # SMs                                         SM              36
    Stack Size                                                 1,024
    Threads                                   thread      16,777,216
    # TPCs                                                        18
    Enabled TPC IDs                                              all
    Uses Green Context                                             0
    Waves Per SM                                              303.41
    -------------------------------- --------------- ---------------

    Section: Occupancy
    ------------------------------- ----------- ------------
    Metric Name                     Metric Unit Metric Value
    ------------------------------- ----------- ------------
    Max Active Clusters                 cluster            0
    Max Cluster Size                      block            8
    Overall GPU Occupancy                     %            0
    Cluster Occupancy                         %            0
    Block Limit Barriers                  block           24
    Block Limit SM                        block           24
    Block Limit Registers                 block           16
    Block Limit Shared Mem                block           16
    Block Limit Warps                     block            6
    Theoretical Active Warps per SM        warp           48
    Theoretical Occupancy                     %          100
    Achieved Occupancy                        %        81.40
    Achieved Active Warps Per SM           warp        39.07
    ------------------------------- ----------- ------------

    OPT   Est. Local Speedup: 18.6%
          The difference between calculated theoretical (100.0%) and measured achieved occupancy (81.4%) can be the
          result of warp scheduling overheads or workload imbalances during the kernel execution. Load imbalances can
          occur between warps within a block as well as across blocks of the same kernel. See the CUDA Best Practices
          Guide (https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/index.html#occupancy) for more details on
          optimizing occupancy.

    Section: GPU and Memory Workload Distribution
    -------------------------- ----------- ------------
    Metric Name                Metric Unit Metric Value
    -------------------------- ----------- ------------
    Average DRAM Active Cycles       cycle    5,733,776
    Total DRAM Elapsed Cycles        cycle   25,124,864
    Average L1 Active Cycles         cycle 1,079,197.22
    Total L1 Elapsed Cycles          cycle   39,395,832
    Average L2 Active Cycles         cycle   969,830.38
    Total L2 Elapsed Cycles          cycle   15,664,688
    Average SM Active Cycles         cycle 1,079,197.22
    Total SM Elapsed Cycles          cycle   39,395,832
    Average SMSP Active Cycles       cycle 1,078,741.17
    Total SMSP Elapsed Cycles        cycle  157,583,328
    -------------------------- ----------- ------------
```

### GPU Speed Of Light Throughput

```txt
    Section: GPU Speed Of Light Throughput
    ----------------------- ----------- ------------
    Metric Name             Metric Unit Metric Value
    ----------------------- ----------- ------------
    DRAM Frequency                  Ghz        13.65
    SM Frequency                    Ghz         2.35
    Elapsed Cycles                cycle        9,509
    Memory Throughput                 %        29.96
    DRAM Throughput                   %        29.96
    Duration                         us         4.03
    L1/TEX Cache Throughput           %        11.64
    L2 Cache Throughput               %        12.18
    SM Active Cycles              cycle     5,864.67
    Compute (SM) Throughput           %        12.84
    ----------------------- ----------- ------------
```

### Launch Statistics

```txt
Section: Launch Statistics
-------------------------------- --------------- ---------------
Metric Name                          Metric Unit    Metric Value
-------------------------------- --------------- ---------------
Block Size                                                   256
Cluster Scheduling Policy                           PolicySpread
Cluster Size                                                   0
Function Cache Configuration                     CachePreferNone
Grid Size                                                    192
Preferred Cluster Size                                         0
Registers Per Thread             register/thread              16
Shared Memory Configuration Size           Kbyte           32.77
Driver Shared Memory Per Block       Kbyte/block            1.02
Dynamic Shared Memory Per Block       byte/block               0
Static Shared Memory Per Block       Kbyte/block            1.02
# SMs                                         SM              32
Stack Size                                                 1,024
Threads                                   thread          49,152
# TPCs                                                        16
Enabled TPC IDs                                       0-2,4,6-17
Uses Green Context                                             1
Waves Per SM                                                   1
-------------------------------- --------------- ---------------
```




### Occupancy

```txt
   Section: Occupancy
    ------------------------------- ----------- ------------
    Metric Name                     Metric Unit Metric Value
    ------------------------------- ----------- ------------
    Max Active Clusters                 cluster           36
    Max Cluster Size                      block            8
    Overall GPU Occupancy                     %         8.33
    Cluster Occupancy                         %         8.33
    Block Limit Barriers                  block           24
    Block Limit SM                        block           24
    Block Limit Registers                 block           16
    Block Limit Shared Mem                block           16
    Block Limit Warps                     block            6
    Theoretical Active Warps per SM        warp           48
    Theoretical Occupancy                     %          100
    Achieved Occupancy                        %        29.08
    Achieved Active Warps Per SM           warp        13.96
    ------------------------------- ----------- ------------

```

```txt
Section: Occupancy
------------------------------- ----------- ------------
Metric Name                     Metric Unit Metric Value
------------------------------- ----------- ------------
Max Active Clusters                 cluster            0
Max Cluster Size                      block            8
Overall GPU Occupancy                     %            0
Cluster Occupancy                         %            0
Block Limit Barriers                  block           24
Block Limit SM                        block           24
Block Limit Registers                 block           16
Block Limit Shared Mem                block           16
Block Limit Warps                     block            6
Theoretical Active Warps per SM        warp           48
Theoretical Occupancy                     %          100
Achieved Occupancy                        %        81.40
Achieved Active Warps Per SM           warp        39.07
------------------------------- ----------- ------------

OPT   Est. Local Speedup: 18.6%
      The difference between calculated theoretical (100.0%) and measured achieved occupancy (81.4%) can be the
      result of warp scheduling overheads or workload imbalances during the kernel execution. Load imbalances can
      occur between warps within a block as well as across blocks of the same kernel. See the CUDA Best Practices
      Guide (https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/index.html#occupancy) for more details on
      optimizing occupancy.
```

1. limit 占用问题

结合理论每个 SM 有 48 个 warp，每个 block 是 256 thread ，
一个 thread 中有 8 个 warp

一个 SM 最多同时驻留 6 个 block，主要限制因素是 `Block Limit Warps = 6`。
每个 SM 最多允许 48 个 active warp，实际平均约有 39 个。

2. 为什么理论 100%，实际只有 81.4%？

理论 occupancy 假设：
* 有足够多的 block 可以持续调度；
* 所有 SM 都能长期保持 6 个 block；
* 不考虑 kernel 开头、结尾的填充和排空阶段；
* 不考虑总 grid 大小不能均匀分配到 SM；
* 不考虑 block 执行时间差异。

实际 occupancy 降低的常见原因包括：
* grid 中 block 数量偏少；
* block 数量不是 SM 数量的良好倍数；
* kernel 执行很短，启动和结束阶段占比较大；
* 不同 block 的执行时间不一致；
* 某些时刻没有足够的可调度工作。

3. cluster 指标分析
* `Overall GPU Occupancy = 0`
* `Cluster Occupancy = 0`

`Max Cluster Size = 8` 只是说硬件最多支持每个 cluster 包含 8 个 block。


## 选中特定的 metric 来看

```txt
sudo /usr/local/cuda-13.1/bin/ncu \
        --kernel-name=vector_add \
        --launch-count=1 \
        --metrics launch__barrier_count,launch__occupancy_limit_barriers,launch__occupancy_limit_blocks \
        ./gpu_debug_demo.out

[sudo] password for martins3:
==PROF== Connected to process 176907 (/home/martins3/data/vn/docs/trace/gpu/gpu_debug_demo.out)
==PROF== Profiling "vector_add": 0%....50%....100% - 1 pass
c[0] = 3.0
==PROF== Disconnected from process 176907
[176907] gpu_debug_demo.out@127.0.0.1
  vector_add(const float *, const float *, float *, int) (256, 1, 1)x(256, 1, 1), Context 1, Stream 7, Device 0, CC 12.0
    Section: Command line profiler metrics
    -------------------------------- ----------- ------------
    Metric Name                      Metric Unit Metric Value
    -------------------------------- ----------- ------------
    launch__barrier_count                                   1
    launch__occupancy_limit_barriers       block           24
    launch__occupancy_limit_blocks         block           24
    -------------------------------- ----------- ------------
```

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
