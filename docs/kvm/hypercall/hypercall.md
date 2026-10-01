# KVM hypercall

这些 hypercall 有实际调用者，但不是每个都有现役调用者，也不是所有架构都支持。

## 各 hypercall 的调用情况

include/uapi/linux/kvm_para.h

| Hypercall                    | 哪些 guest / 场景会调用                                                                       | 当前状态及调用位置                                                                                                                   |
| ---------------------------- | --------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| `KVM_HC_VAPIC_POLL_IRQ`      | Windows XP 等旧 guest 的虚拟 APIC 加速代码；更新 TPR 后，触发 VM exit，让 host 检查待处理中断 | Linux guest 内核未找到调用；QEMU 的 [kvmvapic.S](https://github.com/qemu/qemu/blob/master/pc-bios/optionrom/kvmvapic.S) 有明确调用   |
| `KVM_HC_MMU_OP`              | 早期 x86 Linux 的半虚拟化 MMU：写 PTE、刷新 TLB、释放页表                                     | **已废弃**；当前无调用及 host 实现，编号仍保留                                                                                       |
| `KVM_HC_FEATURES`            | PPC Linux guest 查询 KVM 半虚拟化能力                                                         | [arch/powerpc/include/asm/kvm_para.h](arch/powerpc/include/asm/kvm_para.h)，第 20 行 `kvm_arch_para_features()`；x86 使用 CPUID 查询 |
| `KVM_HC_PPC_MAP_MAGIC_PAGE`  | PPC Linux guest 初始化时映射共享页，通过内存访问部分特权寄存器状态                            | [arch/powerpc/kernel/kvm.c](arch/powerpc/kernel/kvm.c)，第 403 行 `kvm_map_magic_page()`                                             |
| `KVM_HC_KICK_CPU`            | x86 Linux 启用 PV spinlock 时，唤醒因等锁而 halt 的 vCPU                                      | [arch/x86/kernel/kvm.c](arch/x86/kernel/kvm.c)，第 1067 行 `kvm_kick_cpu()`                                                          |
| `KVM_HC_MIPS_GET_CLOCK_FREQ` |                                                                                               |                                                                                                                                      |
| `KVM_HC_MIPS_EXIT_VM`        |                                                                                               |                                                                                                                                      |
| `KVM_HC_MIPS_CONSOLE_OUTPUT` |                                                                                               |                                                                                                                                      |
| `KVM_HC_CLOCK_PAIRING`       | x86 Linux 的 `ptp_kvm` 驱动初始化、读取时钟或交叉时间戳，获取 host 时间与 guest TSC 的配对    | [drivers/ptp/ptp_kvm_x86.c](drivers/ptp/ptp_kvm_x86.c)，第 53、81、114 行                                                            |
| `KVM_HC_SEND_IPI`            | x86 Linux 启用 PV IPI 后，向一组 vCPU 发送 IPI                                                | [arch/x86/kernel/kvm.c](arch/x86/kernel/kvm.c)，`__send_ipi_mask()` 第 549、560 行                                                   |
| `KVM_HC_SCHED_YIELD`         | x86 Linux 发送 call-function IPI 后，发现目标 vCPU 被 host 抢占，请求 host 尝试让目标运行     | [arch/x86/kernel/kvm.c](arch/x86/kernel/kvm.c)，第 645 行 `kvm_smp_send_call_func_ipi()`                                             |

### 存在调用代码，不代表每台 VM 都会调用

`KICK_CPU`、`SEND_IPI`、`SCHED_YIELD` 受 KVM feature、SMP 和 guest
配置等条件控制；`CLOCK_PAIRING` 要走到 `ptp_kvm` 驱动路径。

- `KICK_CPU`：需要编译 `CONFIG_PARAVIRT_SPINLOCKS`，并启用 PV spinlock。单
  vCPU、`nopvspin`、`KVM_HINTS_REALTIME` 或缺少 `KVM_FEATURE_PV_UNHALT`
  等条件会阻止启用。
- `SEND_IPI`：`pv_ipi_supported()` 要求 `KVM_FEATURE_PV_SEND_IPI`，且
  `num_possible_cpus() != 1`。
- `SCHED_YIELD`：`pv_sched_yield_supported()` 要求
  `KVM_FEATURE_PV_SCHED_YIELD`、`KVM_FEATURE_STEAL_TIME`、多个 possible
  CPU，同时没有 `KVM_HINTS_REALTIME` 和 `X86_FEATURE_MWAIT`。实际发送
  call-function IPI 后，还要发现非 idle 且被抢占的目标 vCPU 才会调用。
- `CLOCK_PAIRING`：`ptp_kvm` 初始化会探测该
  hypercall，后续读时钟或获取交叉时间戳时继续调用。普通 `kvm-clock`
  读时钟不能直接等同于调用该 hypercall。

相关条件见 [arch/x86/kernel/kvm.c](arch/x86/kernel/kvm.c) 的
`pv_ipi_supported()`、`pv_sched_yield_supported()` 和 `kvm_spinlock_init()`。

### VAPIC_POLL_IRQ 的空操作有实际用途

host 分支虽然只是 `ret = 0`，但触发 VM exit 本身就是目的：使 host 在重新进入
guest 前检查待处理中断，不能据此判断它没用。

见 [arch/x86/kvm/x86.c](arch/x86/kvm/x86.c) 第 10431 行。QEMU 的 `kvmvapic.S` 在
`mp_set_tpr_poll_irq` 和 `up_set_tpr_poll_irq` 中把编号 1 放入 EAX，再执行
hypercall。这些代码运行在 guest 中。

## 实验

x86 host 已有 `kvm:kvm_hypercall` tracepoint，


配合 ./test.c 运行，结果为:
```txt
129654.250 CPU 0/KVM/1772113 kvm:kvm_hypercall()
131557.154 CPU 0/KVM/1772113 kvm:kvm_hypercall()
132227.864 CPU 0/KVM/1772113 kvm:kvm_hypercall()
132900.642 CPU 0/KVM/1772113 kvm:kvm_hypercall()
```

KVM_HC_SEND_IPI 是可以普遍观察到的日志

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
