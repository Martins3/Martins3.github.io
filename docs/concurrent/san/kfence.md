# kfence

<!-- 27cfc022-e7e8-46df-8cfe-25c4a3a36805 -->

## kfence 是默认打开的
一些发行版的内核默认启用了 KFENCE，例如 ARM64 Fedora Asahi 内核：

```txt
cat config-6.17.12-400.asahi.fc42.aarch64+16k | grep FENCE
# CONFIG_DMA_FENCE_TRACE is not set
CONFIG_HAVE_ARCH_KFENCE=y
CONFIG_KFENCE=y
CONFIG_KFENCE_SAMPLE_INTERVAL=100
CONFIG_KFENCE_NUM_OBJECTS=255
# CONFIG_KFENCE_DEFERRABLE is not set
# CONFIG_KFENCE_STATIC_KEYS is not set
CONFIG_KFENCE_STRESS_TEST_FAULTS=0
CONFIG_KFENCE_KUNIT_TEST=m
```

## 解决的问题

KFENCE 是低开销、基于抽样的内核堆内存安全检测器，主要检测：

- heap out-of-bounds；
- use-after-free；
- invalid free 和 double free；
- 未直接碰到 guard page、但破坏了 canary 的越界写。

## 核心结构：对象页和 guard page 交错

KFENCE
启动时预留一个固定大小的连续内存池。每个受保护对象独占一个页，对象页之间插入不可访问的
guard page：

```txt
guard | object page | guard | object page | guard
      | O......     |       |     ......O |
      | object      |       |      object |
      |    canary   |       | canary      |
```

guard page
通过架构相关的页表操作设置为不可访问。对象随机放在对象页的左边界或右边界：

- 放在左边界时，左侧越界立即进入 guard page；
- 放在右边界时，右侧越界立即进入 guard page；
- 对象另一侧仍位于同一个可访问页内，使用按地址变化的 `0xaa` canary 填充。

这样既能随机覆盖左、右越界，又只需要一个对象页和相邻的共享 guard page。

每个对象对应一个 `struct kfence_metadata`，记录：

- `UNUSED`、`ALLOCATED`、`RCU_FREEING`、`FREED` 状态；
- 对象地址、请求大小和所属 `kmem_cache`；
- 分配与释放时的 PID、CPU、时间和调用栈；
- 因报错而临时解除保护的页面；
- freelist 节点和并发保护锁。

## 抽样分配

KFENCE hook 位于 SLAB/SLUB 分配路径中，但绝大多数分配仍由普通 allocator 完成：

```txt
kmalloc/kmem_cache_alloc
        |
        +-- allocation gate 关闭 --> 普通 SLUB 分配
        |
        `-- allocation gate 打开 --> 从 KFENCE freelist 取一个对象页
```

`kfence.sample_interval` 的单位是毫秒。定时器每经过一个采样周期打开一次
allocation gate，之后的下一个兼容分配才会尝试进入 KFENCE pool。它不是“每 N
次分配抽一次”，因此实际采样比例取决于分配频率。

默认每个周期只尝试抽取一个对象；`kfence.burst=N` 可以让每个周期尝试连续抽取
`1 + N` 个对象。

KFENCE 只接受不超过 `PAGE_SIZE` 的对象，并会跳过 DMA/DMA32、特殊 NUMA zone
以及带 `SLAB_SKIP_KFENCE` 的 cache。pool 没有空闲槽位时也会回退到普通分配。

当 pool 使用率超过默认 75% 后，KFENCE 使用分配调用栈的 hash 和 counting Bloom
filter，避免同一个热点、长生命周期分配源占满整个
pool，从而提高代码路径覆盖的多样性。

### allocation gate 的快路径成本

没有启用 `CONFIG_KFENCE_STATIC_KEYS` 时，普通分配路径主要多出：

- 一个 static branch；
- 一次 `atomic_read(kfence_allocation_gate)`；
- 极低频命中后的 KFENCE 慢路径。

启用 `CONFIG_KFENCE_STATIC_KEYS` 后，未采样时可以跳过 gate
读取，但每次打开和关闭 static key 需要修改指令并向 CPU 发送
IPI。采样间隔很大时可能有利，间隔较短时不一定更快，需要按具体机器测试。

示例配置没有启用 `CONFIG_KFENCE_STATIC_KEYS`，所以不会每 100ms 为切换 static key
广播 IPI，但普通 slab 分配会执行 gate 判断。

## 错误检测过程

### Out-of-bounds

访问越过对象边界并进入 guard page 后产生 page fault。架构 page fault handler
先判断地址是否属于 KFENCE pool，然后交给 `kfence_handle_page_fault()`：

1. 根据 fault 地址定位相邻对象的 metadata；
2. 判断是左侧还是右侧越界以及读写类型；
3. 输出当前访问栈和对象分配栈；
4. 按 `kfence.fault=report|oops|panic` 决定继续、oops 或 panic。

默认 `report` 模式报告后会临时解除 fault
页保护，让错误代码继续执行，因此同一页面上的后续错误不保证重复报告。

没有碰到 guard page 的同页越界写会修改 canary。KFENCE 在对象释放时检查
canary，也可以通过 `kfence.check_on_panic=1` 在 panic 时检查所有已分配对象。对
canary 的越界读不会修改数据，因此无法通过这种方式检测。

### Use-after-free

释放 KFENCE 对象时：

1. 检查 canary，发现分配期间的同页越界写；
2. 保存释放调用栈并把状态设为 `FREED`；
3. 把整个对象页设置为不可访问；
4. 将对象加入 freelist 尾部。

后续 stale pointer 访问对象页会产生 page fault，并根据 `FREED` 状态报告
use-after-free，同时给出访问、分配和释放调用栈。

分配从 freelist 头部取对象、释放放到尾部，相当于一个固定大小的 FIFO
quarantine，尽量延迟最近释放对象的复用。对象一旦被重新分配，旧指针访问可能不再触发
page fault，因此 UAF 检测窗口不是无限的。

### Invalid free

释放地址属于 KFENCE pool，但不是 metadata
中记录的对象起始地址，或者对象状态已经不是 allocated 时，KFENCE 报告 invalid
free。这也覆盖 double free。

## 为什么开销低

KFENCE 不使用编译器为每次 load/store 插桩。普通访存没有 shadow memory
检查，错误只在访问受保护页产生硬件 page fault 时进入检测路径。

主要开销是：

- 普通分配路径上的 gate 判断；
- 采样命中时保存调用栈、操作页表、初始化 canary；
- 固定预留的 KFENCE pool 和 metadata；
- 非 deferrable timer 可能周期性唤醒空闲 CPU，增加功耗。

因此不能只根据配置给出固定的性能损失百分比。它取决于分配速率、采样间隔、CPU
数量、是否使用 static key 以及工作负载的 idle 特征。可通过实际 workload
对比测试，并查看：

```bash
cat /sys/kernel/debug/kfence/stats
cat /sys/kernel/debug/kfence/objects
```

## 内存开销

固定 pool 大小为：

```txt
(CONFIG_KFENCE_NUM_OBJECTS + 1) * 2 * PAGE_SIZE
```

其中额外的 `+1` 用来简化地址到 metadata 下标的映射，并形成扩展 guard 区域。

对于示例中的 `CONFIG_KFENCE_NUM_OBJECTS=255`：

| 页大小 | KFENCE pool |
| ------ | ----------: |
| 4 KiB  |       2 MiB |
| 16 KiB |       8 MiB |

此外还有：

```txt
PAGE_ALIGN(sizeof(struct kfence_metadata) * CONFIG_KFENCE_NUM_OBJECTS)
```

metadata 的大小随架构和内核配置变化，其中仅分配、释放两份 64 层调用栈在 64
位系统上就约占 1 KiB/对象，因此 255 个对象还需要数百 KiB 元数据。拆分 huge page
映射时也可能增加页表开销。

所以这份 16 KiB 页配置的主要固定成本是 8 MiB pool，外加数百 KiB metadata
和少量页表、全局统计结构。

## 与 KASAN 对比

|              | KFENCE                                      | KASAN                                          |
| ------------ | ------------------------------------------- | ---------------------------------------------- |
| 检测方式     | 少量对象抽样进入 guard-page pool            | 对大量内存访问进行编译器插桩和 shadow/tag 检查 |
| 覆盖率       | 仅被抽中的对象                              | 明显更高                                       |
| 持续运行开销 | 很低，适合生产环境                          | 较高，更适合测试环境                           |
| OOB 精度     | guard 一侧精确；另一侧依赖释放时检查 canary | 通常能在非法访问发生处立即报告                 |
| UAF 窗口     | 对象页复用前                                | 受 quarantine、tag 和模式影响，通常覆盖更强    |
| 适用场景     | 大规模部署、长时间捕捉低频问题              | 复现、测试、fuzz 和精确定位                    |

两者是互补关系：能够承受 KASAN 成本的测试环境优先使用 KASAN；生产环境无法启用
KASAN 时，使用 KFENCE 以较低成本积累检测机会。

## 相关配置和运行参数

```txt
CONFIG_KFENCE
CONFIG_KFENCE_SAMPLE_INTERVAL
CONFIG_KFENCE_NUM_OBJECTS
CONFIG_KFENCE_DEFERRABLE
CONFIG_KFENCE_STATIC_KEYS

kfence.sample_interval=<milliseconds>  # 0 表示关闭
kfence.burst=<N>
kfence.deferrable=0|1
kfence.skip_covered_thresh=<percent>
kfence.fault=report|oops|panic
kfence.check_on_panic=0|1
```

## 相关源码

- `mm/kfence/core.c`：pool、抽样 gate、分配释放和 page fault 处理；
- `mm/kfence/kfence.h`：metadata、对象状态和错误类型；
- `mm/kfence/report.c`：错误分类与报告；
- `include/linux/kfence.h`：allocator 和架构 page fault handler 接口；
- `mm/slub.c`：SLUB 分配、释放路径中的 KFENCE hook；
- `arch/*/mm/fault.c`：各架构将 KFENCE pool 内的 page fault 交给 KFENCE。

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
