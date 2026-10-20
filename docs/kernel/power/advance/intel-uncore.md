## uncore

```txt
  核心内容概括如下：

   1. 背景与目的 (Introduction):
       * Uncore: 指的是 Intel 处理器中除了 CPU 核心 (Core) 之外的部分，例如共享缓存、内存控制器、环形总线等。这部分也能消耗大量电力。
       * 问题: SoC（系统级芯片）内部有算法根据工作负载自动调节 uncore 频率以优化功耗和性能，但这可能不符合所有用户的特定需求。
       * 目标: 提供一个类似 cpufreq 的接口，让用户能够手动控制 uncore 的最小/最大频率，以满足对延迟敏感的应用或需要在不同阶段分别控制核心和 uncore
         性能的工作负载的需求。

   2. sysfs 接口 (Sysfs Interface):
       * 接口位于 /sys/devices/system/cpu/intel_uncore_frequency/。
       * 基本控制: 按照 Package (插槽) 和 Die (晶粒) 的组合创建目录（如 package_00_die_00），因为 uncore 频率控制的范围是每个 Die 或每个 Package。
       * 基本属性:
           * initial_max_freq_khz / initial_min_freq_khz: 只读，复位时的最大/最小频率。
           * max_freq_khz / min_freq_khz: 可读写，设置 uncore 的最大/最小频率。
           * current_freq_khz: 只读，获取当前 uncore 频率。
       * 高级控制 (TPMI 支持): 对于支持 TPMI (Topology Aware Register and PM Capsule Interface) 的 SoC，提供了更细粒度的控制，按 Fabric Cluster
         (互连集群) 级别进行管理。这通过 uncore00, uncore01 等子目录体现，其中包含 package_id, domain_id, fabric_cluster_id
         等属性来标识范围，并同样提供 max_freq_khz, min_freq_khz, current_freq_khz 等属性。还介绍了包级别控制与集群级别控制之间的关系和优先级。

   3. 效率与延迟权衡 (Efficiency vs. Latency Tradeoff):
       * ELC (Efficiency Latency Control) 特性: 硬件会根据 CPU 利用率自动调节 uncore 频率以优化性能功耗比，但这可能不适合对延迟敏感的应用。
       * 软件干预: 提供了额外的 sysfs 属性，允许软件覆盖或微调硬件的 ELC 行为：
           * elc_floor_freq_khz: 当 CPU 利用率低于阈值时，设置 uncore 的最低频率下限。
           * elc_low_threshold_percent: 触发 elc_floor_freq_khz 的 CPU 利用率低阈值。
           * elc_high_threshold_percent: 在高负载下，触发逐步提升频率（而非直接跳到最高）的 CPU 利用率高阈值。
           * elc_high_threshold_enable: 启用/禁用高阈值行为。
       * 示例: 文档末尾给出了一个配置示例，说明如何设置这些参数来实现特定的频率控制策略。

  总结来说，这个文档是关于 Linux 内核如何提供接口来手动控制 Intel 处理器中 "Uncore" 部分的运行频率。
  这是为了让用户能够超越硬件自动调节的范围，根据特定应用的需求（如降低延迟或精细化功耗管理）来优化系统性能和功耗。它详细描述了控制接口的位置、方式（s
  ysfs）、不同硬件架构下的差异（TPMI），以及如何通过 ELC 相关参数来调整硬件自动调节的策略。
```

/sys/devices/system/cpu/intel_uncore_frequency
```txt
├── package_00_die_00
│   ├── current_freq_khz
│   ├── initial_max_freq_khz
│   ├── initial_min_freq_khz
│   ├── max_freq_khz
│   └── min_freq_khz
└── package_01_die_01
    ├── current_freq_khz
    ├── initial_max_freq_khz
    ├── initial_min_freq_khz
    ├── max_freq_khz
    └── min_freq_khz
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
