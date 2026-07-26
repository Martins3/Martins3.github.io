# 拷贝的成本

## GPU 拷贝成本

### PCIe 的带宽和延迟


## CPU 单页拷贝的成本

先测一个最小问题：如果无法 zero-copy，只是拷贝一个 4KiB page，大概需要多久。

测试程序：

```sh
gcc -O3 -march=native -Wall -Wextra -o docs/kernel/topics/page-copy.out docs/kernel/topics/page-copy.c
```

当前测试机器：

```txt
CPU: 13th Gen Intel(R) Core(TM) i9-13900K
CPU1: P-core sibling, max 5500MHz
```

### 同一个热 page

命令：

```sh
taskset -c 1 docs/kernel/topics/page-copy.out 4096 1 10000000
```

结果：

```txt
copy_size:  4096 bytes
pages:      1
iterations: 10000000
total:      38.147 GiB
elapsed:    0.154856 s
latency:    15.49 ns/copy
bandwidth:  246.34 GiB/s
counter:    46.38 ticks/copy
```

另一个 P-core 上结果类似：

```txt
taskset -c 8 ...
latency:    15.49 ns/copy
bandwidth:  246.28 GiB/s
counter:    46.40 ticks/copy
```

这个数字代表的是 L1/cache 非常热的上限，不代表真实 I/O 或网络路径里每个 page 都能这么便宜。

### 1MiB 工作集

命令：

```sh
taskset -c 1 docs/kernel/topics/page-copy.out 4096 256 10000000
```

结果：

```txt
latency:    79.96 ns/copy
bandwidth:  47.70 GiB/s
counter:    239.51 ticks/copy
```

### 32MiB 工作集

命令：

```sh
taskset -c 1 docs/kernel/topics/page-copy.out 4096 8192 2000000
```

结果：

```txt
latency:    327.14 ns/copy
bandwidth:  11.66 GiB/s
counter:    979.86 ticks/copy
```

其实拷贝的速度相当快
```txt
taskset -c 1 docs/kernel/topics/page-copy.out 4096 800192 1
copy_size:  4096 bytes
pages:      800192
iterations: 1
total:      0.000 GiB
elapsed:    0.000001 s
latency:    555.00 ns/copy
bandwidth:  6.87 GiB/s
counter:    56974.00 ticks/copy
checksum:   1390000
```

### 初步结论

单个热 page 的 memcpy 不是贵到不可接受：当前机器上约 15.5ns，
但是只要工作集扩大，成本会很快被 cache/TLB/memory 层级放大。

所以讨论 zero-copy 时不能只说“少拷贝一次 4KiB page”。需要同时说明这个 page 是不是热的、工作集多大、访问模式是否顺序、是否跨核/跨 NUMA，以及 copy 是否处在请求关键路径上。

```txt
Caches (sum of all):
  L1d:                       896 KiB (24 instances)
  L1i:                       1.3 MiB (24 instances)
  L2:                        32 MiB (12 instances)
  L3:                        36 MiB (1 instance)
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
