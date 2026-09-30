## tsc reorder

<!-- 86dd6142-0b7d-4b1f-8729-001eedb79f5a -->

`tsc-fence-test.c` 对比结束计时使用裸 `RDTSC`、`LFENCE; RDTSC`、 `RDTSCP` 和
`CPUID; RDTSC` 的结果。适用于支持 RDTSCP、CLFLUSH，且 LFENCE 具有执行 排序语义的
Intel x86-64 CPU。

程序绑定到一个允许使用的 CPU，每组采样 20000 次，轮换各组执行顺序。 起始边界均有
LFENCE；冷缓存组在计时前执行 `CLFLUSH; MFENCE`。 被测 load 和时间戳位于同一个
asm 块，避免编译器移动或删除 load。 所有结束时间戳后都保留
LFENCE，让尚未完成的工作在下一次采样前结束；
它不能补救已经提前读取的时间戳。输出是未扣除开销的 TSC tick，非 CPU 核心周期。
CPUID 组固定 EAX=0、ECX=0，初始化指令和 CPUID 均计入测量区间。
所有组使用相同的寄存器 clobber，避免 CPUID 组独有的寄存器约束影响比较；
已通过反汇编核对 load、CPUID、时间戳的顺序及起始时间戳的保存寄存器。

2026-09-11 在 i9-13900K、CPU 0 上连续运行三轮，中位数如下：

| 测量                            | 第一轮 | 第二轮 | 第三轮 |
| ------------------------------- | -----: | -----: | -----: |
| 空测量，裸 RDTSC 结束           |     18 |     18 |     18 |
| 空测量，LFENCE; RDTSC 结束      |     26 |     26 |     26 |
| 热缓存 load，裸 RDTSC 结束      |     20 |     20 |     20 |
| 热缓存 load，LFENCE; RDTSC 结束 |     28 |     28 |     28 |
| 冷缓存 load，裸 RDTSC 结束      |     18 |     20 |     20 |
| 冷缓存 load，LFENCE; RDTSC 结束 |    274 |    206 |    202 |
| 冷缓存 load，RDTSCP 结束        |    272 |    204 |    202 |
| 空测量，CPUID; RDTSC 结束       |     86 |     86 |     86 |
| 热缓存 load，CPUID; RDTSC 结束  |     86 |     86 |     86 |
| 冷缓存 load，CPUID; RDTSC 结束  |    338 |    258 |    254 |

裸 RDTSC 把冷缓存读取测得与空测量一样短，而 fence 本身在空测量中只增加 约 8
tick：差异不能用 fence 开销解释，说明结束时间戳没有等待 load 完成。

CPUID 组能区分冷、热缓存读取，支持它有效阻止结束时间戳越过未完成 load 的结论。
但空测量为 86 tick，比 LFENCE 组的 26 tick 多 60 tick；相对于裸 RDTSC
基线，CPUID 序列增加约 68 tick，LFENCE 增加约 8 tick。这是该测量序列的
开销差值，不能当作孤立指令的精确延迟。空测量 p99 在 CPUID 组为 104–106 tick，
LFENCE 组为 32–34 tick。

在本机这类短区间 load 计时中，CPUID 排序有效，但 LFENCE 的开销更低。 本实验将
CPUID 放在结束时间戳之前，其开销计入结果；原先 `tsc.c` 中
开始时间戳前、结束时间戳后的 CPUID 位于计时区间外，不能直接套用这里的开销差值。
本实验验证结束时间戳越过前面的 load，不单独验证起始边界或 store 全局可见性；
具体数值会随硬件和系统状态变化。

## 虚拟机和物理机中

虚拟机中执行:

```txt
./a.out
CPU 0, 20000 samples/case, raw TSC ticks
case                            min   median      p90      p99
empty / no end fence             30       34       44       46
empty / end fence                38       42       60       62
warm load / no end fence         32       34       44       46
warm load / end fence            40       42       60       62
cold load / no end fence         32       34       54       56
cold load / end fence           214      230      398     1220
cold load / RDTSCP              212      222      382      918
empty / CPUID                  1178     1188     3118     3148
warm load / CPUID              1176     1188     3116     3138
cold load / CPUID              1344     1428     3342     4256
```

物理机中执行:

```txt
CPU 0, 20000 samples/case, raw TSC ticks
case                            min   median      p90      p99
empty / no end fence             16       18       20       22
empty / end fence                24       26       26       30
warm load / no end fence         16       18       20       22
warm load / end fence            26       28       28       32
cold load / no end fence         16       18       20       22
cold load / end fence           186      194      380     1858
cold load / RDTSCP              186      194      374     1782
empty / CPUID                    82       84       86      100
warm load / CPUID                82       86       88      100
cold load / CPUID               238      246      464     1986
```

主要区别在 cpuid 上。

## 时间相关的 cpu feature
cat /proc/cpuinfo

通用 CPU flags :
- tsc — Time Stamp Counter
- constant_tsc — TSC 频率不随 CPU 频率变化
- nonstop_tsc — TSC 在 C-state 休眠时也不停止
- tsc_known_freq — TSC 频率已知
- tsc_deadline_timer — 支持 TSC deadline 模式的本地 APIC 定时器
- tsc_adjust — MSR_IA32_TSC_ADJUST 可调整 TSC
- rdtscp — RDTSCP 指令（读取 TSC + 处理器 ID）

VMX flags：
- tsc_offset — VMX TSC 偏移
- tsc_scaling — VMX TSC 缩放

## AMD 的机器加载 kvm 的时候，总是存在这个问题

```txt
[13305.998993] kvm: SMP vm created on host with unstable TSC;
guest TSC will not be reliable
```
主要是，不是 reliable 似乎也没关系啊

## tsc 的核心函数
```txt
arch/x86/kernel/tsc.c
arch/x86/kernel/tsc_msr.c
arch/x86/kernel/tsc_sync.c
```

## TODO
- https://lore.kernel.org/all/594A322A-8100-429A-A3E8-64362E3ED5A2@clockwork.io/
- rdtsc 原来还存在这个小技巧啊:
  - https://stackoverflow.com/questions/27693145/rdtscp-versus-rdtsc-cpuid


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
