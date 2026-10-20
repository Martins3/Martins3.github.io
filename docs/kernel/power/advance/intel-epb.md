## epb

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```txt
1. 功能概述 (Overview):
    * EPB 是 Intel 处理器提供的一种机制，允许操作系统向处理器发出信号，指示在性能和能耗之间应偏向哪一方。它不直接控制频率，而是给硬件一个“提示”，让
      硬件的电源管理单元根据这个提示在可用的 P-states (性能状态) 中做选择时有所侧重。

2. sysfs 接口:
    * 接口位于每个逻辑 CPU 的 sysfs 目录下：/sys/devices/system/cpu/cpu<N>/power/。
    * `energy_perf_bias` 文件:
        * 读取: 显示当前 CPU 的 EPB 值，范围是 0 到 15 的数字。
            * 0: 最高性能偏好 (performance)
            * 15: 最大节能偏好 (power)
            * 中间值 (如 6 对应 normal, 4 对应 balance-performance, 8 对应 balance-power) 代表不同程度的折衷。
        * 写入: 可以写入 0-15 的数字，或者对应的字符串 (performance, balance-performance, normal, balance-power, power) 来修改 EPB 设置。
    * 适用性: 此属性存在于所有支持 EPB 功能的在线 CPU 上。

3. 注意事项:
    * 虽然 EPB 接口是按逻辑 CPU 级别定义的，但其背后的物理寄存器可能被多个 CPU 共享（例如同核心的超线程兄弟 CPU，或同一封装内的多个核心）。
    * 因此，修改一个 CPU 的 EPB 值，可能会导致其他共享同一寄存器的 CPU 的 EPB 值也发生改变。

总结来说，这个文档是关于 Linux 如何通过 `sysfs` 接口设置 Intel 处理器的 EPB 寄存器。 这个寄存器是一个“软”提示，告诉处理器的硬件电源管理单元，在性能
和功耗之间应该如何权衡。值越小越偏向性能，越大越偏向节能。用户或管理员可以根据工作负载的特性来调整这个值，以期获得更好的性能或功耗表现。
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
