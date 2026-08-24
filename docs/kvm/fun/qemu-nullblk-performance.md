# QEMU 中 null_blk fio 性能与 HugeTLB 对比

测试日期：2026-08-21

## 测试目标

使用 `fio + io_uring + null_blk` 比较物理机和虚拟机中的单核块层执行效率，并确认以下问题：

1. 虚拟机相对物理机的 IOPS 损失；
2. KVM exit 是否是主要损失来源；
3. QEMU guest RAM 使用 2 MiB HugeTLB 后能否改善性能。

这个测试中的 `/dev/nullb0` 位于各自的内核中。虚拟机中的 I/O 路径为：

```text
fio
 -> io_uring
 -> guest block layer
 -> guest null_blk
```

它不经过 `virtio-blk`、QEMU block backend 或 host 的 `/dev/nullb0`，因此主要测量 guest 内核块层在虚拟 CPU 上的执行效率，而不是虚拟磁盘性能。

## fio 配置

```ini
[global]
time_based
runtime=1000
ioengine=io_uring
iodepth=128
direct=1
bs=4k

[trash]
rw=randread
filename=/dev/nullb0
numjobs=1
```

实际短测将 `runtime` 缩短为 10 秒或 30 秒，并使用 2 至 3 秒 `ramp_time`。`runtime` 只影响测试持续时间，不改变 I/O 路径。

两边 `null_blk` 的主要参数一致：

```text
queue_mode       2
submit_queues    1
hw_queue_depth   64
completion_nsec  10000
irqmode          1
poll_queues      1
scheduler        none
```

## 物理机结果

环境：

```text
CPU:    Intel Core i9-13900K
kernel: 7.1.3-201.fc44.x86_64
fio:    3.41
```

三轮 IOPS：

```text
1392703
1385091
1324080
```

平均值：

```text
1367291 IOPS
```

## yyds-fs 普通内存结果

环境：

```text
guest:  Fedora 42
kernel: 6.19.14-108.fc42.x86_64
fio:    3.37
RAM:    16 GiB
```

为了减少调度影响，fio 固定在 guest CPU0，对应的 QEMU vCPU0 固定在 host P-core 10。

普通 4 KiB QEMU memory backing 的五轮 IOPS：

```text
1222859
1341207
1340184
1305392
1309725
```

统计结果：

```text
平均值: 1303873 IOPS
中位数: 1309725 IOPS
标准差:   48250 IOPS
变异系数: 3.70%
```

30 秒长测：

```text
1327753 IOPS
```

## 表观虚拟化损失

使用物理机和 `yyds-fs` 的多轮平均值计算：

```text
host: 1367291 IOPS
VM:   1303873 IOPS

(1367291 - 1303873) / 1367291 = 4.64%
```

因此本轮测试的表观性能下降是 `4.64%`，VM 保留约 `95.36%` 的 host IOPS。

但是这个数字不能全部解释成纯虚拟化开销：

- host 和 guest 的内核版本不同；
- host 使用 fio 3.41，guest 使用 fio 3.37；
- 两组测试不是按 host/guest 交替顺序执行；
- host 自身结果为 1.324M 至 1.393M，guest 为 1.223M 至 1.341M，两组区间重叠；
- 测试机同时运行多个 QEMU，且 13900K 同时包含 P-core 和 E-core。

当前样本中，host 标准差约为 38K，guest 标准差约为 48K，而均值仅相差约 63K。因此还不能从统计上把这 4.64% 与运行噪声、内核差异完全分离。

## HugeTLB A/B 测试

### 内存映射确认

普通模式的 QEMU guest RAM：

```text
KernelPageSize: 4 KiB
MMUPageSize:    4 KiB
Shared_Hugetlb: 0
```

普通的共享 memfd 没有被 THP 自动合并，因此它确实使用 4 KiB host pages。

HugeTLB 模式：

```text
QEMU memory-backend-memfd: hugetlb=true
KernelPageSize:            2048 KiB
MMUPageSize:               2048 KiB
```

host 原本只有 4096 个 2 MiB hugepages，即 8 GiB。停止 24 GiB VM 并执行内存 compaction 后，最多只能获得 8459 页，约 16.5 GiB。为了不停止其他 VM，严格 A/B 测试统一使用 16 GiB RAM。

### 原始结果

普通 4 KiB backing：

```text
1222859
1341207
1340184
1305392
1309725
```

2 MiB HugeTLB backing：

```text
1356825
1329804
1221786
1228040
1385514
```

统计对比：

| Backing | 平均 IOPS | 中位数 IOPS | 标准差 | 变异系数 |
|---|---:|---:|---:|---:|
| 4 KiB | 1303873 | 1309725 | 48250 | 3.70% |
| 2 MiB HugeTLB | 1304394 | 1329804 | 75215 | 5.77% |

差异：

```text
按均值:   +0.04%
按中位数: +1.53%
```

两者均小于测试自身的 3.70% 至 5.77% 波动，不能视为有效提升。

30 秒长测还得到相反结果：

```text
4 KiB:          1327753 IOPS
2 MiB HugeTLB:  1229505 IOPS
差异:              -7.40%
```

单次长测和五轮短测的方向相反，说明当前 host 的时间性噪声大于 HugeTLB 效果。对这个单 job、单 vCPU、热点工作集很小的 null_blk 测试，HugeTLB 没有可测量的性能收益。

HugeTLB 主要减少 EPT 页表层级和 TLB miss 成本。这个 fio 路径的代码和数据工作集较小，大部分已经驻留在 cache 和 TLB 中，因此很难从 2 MiB backing 获益。多 vCPU、大内存随机访问或 DMA 覆盖较大 guest RAM 的负载更可能受益。

## KVM exit 调查

在 `virtme` 中对承载 fio 的 vCPU 采样 10 秒，得到：

```text
MSR_WRITE            11416
EXTERNAL_INTERRUPT   10162
PREEMPTION_TIMER     10087
```

### MSR_WRITE

按 MSR 编号统计：

```text
MSR 0x6e0  10048
MSR 0x830   1362
```

`0x6e0` 是 `MSR_IA32_TSC_DEADLINE`。guest 的 `arch/x86/kernel/apic/apic.c` 中 `lapic_next_deadline()` 会用它设置下一个 local APIC 时钟事件。KVM 在 `arch/x86/kvm/lapic.c` 中通过 `kvm_set_lapic_tscdeadline_msr()` 处理该写入。

`0x830` 是 x2APIC ICR，主要用于发送 IPI。

### EXTERNAL_INTERRUPT

物理中断 vector 统计：

```text
0xec  10087
0xfb     59
0xfc     15
0xfd     18
```

`arch/x86/include/asm/irq_vectors.h` 中的宏定义为：

```text
LOCAL_TIMER_VECTOR           0xec
CALL_FUNCTION_SINGLE_VECTOR  0xfb
CALL_FUNCTION_VECTOR         0xfc
RESCHEDULE_VECTOR            0xfd
```

绝大多数 `EXTERNAL_INTERRUPT` 是 host 的 local APIC timer。对应 KVM 处理位于 `arch/x86/kvm/vmx/vmx.c` 中的 `vmx_handle_exit_irqoff()` 和 `handle_external_interrupt_irqoff()`。

### PREEMPTION_TIMER

KVM 使用 VMX preemption timer 在 guest LAPIC deadline 到达时强制退出 guest，以便注入虚拟时钟中断。关键路径为：

```text
start_hv_timer()
 -> vmx_set_hv_timer()
 -> VMX_PREEMPTION_TIMER_VALUE
 -> handle_fastpath_preemption_timer()
 -> kvm_lapic_expired_hv_timer()
```

相关实现位于 `arch/x86/kvm/lapic.c` 和 `arch/x86/kvm/vmx/vmx.c`。

`perf kvm stat` 的 10 秒结果：

| Exit | 次数 | 平均处理时间 |
|---|---:|---:|
| MSR_WRITE | 11750 | 0.39 us |
| EXTERNAL_INTERRUPT | 10164 | 1.59 us |
| PREEMPTION_TIMER | 10004 | 0.23 us |

总计 31931 次 exit，处理时间约 23.13 ms，只占 10 秒单核时间的约 `0.23%`。这些 timer exit 是正常现象，不足以解释 4.64% 的 IOPS 差距。

`kvmexit` 输出的是整个采样窗口的计数。例如 10 秒窗口中的 10000 次对应约 1000 次/秒，不能把 10000 直接理解为每秒次数。

## debug guest 的额外陷阱

`virtme` 当前使用的 `linux-drm` 内核启用了：

```text
CONFIG_LOCKDEP=y
CONFIG_PROVE_LOCKING=y
CONFIG_DEBUG_SPINLOCK=y
CONFIG_DEBUG_MUTEXES=y
CONFIG_DEBUG_RWSEMS=y
CONFIG_DEBUG_ATOMIC_SLEEP=y
CONFIG_DEBUG_IRQFLAGS=y
```

该 guest 只能达到约 377K IOPS。guest 内 `perf` 热点为：

```text
33.37%  lock_acquire
21.91%  lock_release
 8.43%  lock_is_held_type
```

包含其他 lockdep/RCU debug 函数后，明确的调试开销超过 66%。运行时关闭 `prove_locking` 和 `lock_stat` 只能将性能从约 377K 提高到 445K，因为编译期插入的 lockdep hooks 仍然存在。

因此不能使用 debug/LOCKDEP guest 和非 debug host 比较虚拟化性能。

## 结论

1. 本轮 host 平均为 1367291 IOPS，`yyds-fs` 平均为 1303873 IOPS，表观下降为 4.64%。
2. 4.64% 与当前测试波动接近，而且 host/guest 内核和 fio 版本不同，尚不能全部归因于 KVM。
3. KVM timer exit 的实测处理时间约占单核 0.23%，不是主要性能损失来源。
4. 2 MiB HugeTLB 的五轮均值提升只有 0.04%，低于运行噪声，对该负载没有可测量的帮助。
5. LOCKDEP/debug 内核会主导这个内核态密集的 null_blk 微基准，必须使用非 debug 内核进行性能对比。

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
