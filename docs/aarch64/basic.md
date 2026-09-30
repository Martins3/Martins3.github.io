# aarch64 基础

核心文档:
- https://www.kernel.org/doc/html/latest/arch/arm64/index.html

## qemu hmp info registers 解析
<!-- e6bb59f1-47eb-42df-bef4-1c1cd38404f6 -->

我知道 aarch64 会很整齐，很清晰，但是没想到，这么整齐，哭了 😭

┌─────────┬──────────────────────────────────────────────────────────────┐
│ 寄存器  │ 含义                                                         │
├─────────┼──────────────────────────────────────────────────────────────┤
│ X00-X07 │ 函数参数/返回值。X0 通常是函数返回值，X0-X7 是函数调用参数。 │
├─────────┼──────────────────────────────────────────────────────────────┤
│ X08-X18 │ 临时寄存器（caller-saved）。                                 │
├─────────┼──────────────────────────────────────────────────────────────┤
│ X19-X28 │ 被调者保存寄存器（callee-saved）。                           │
├─────────┼──────────────────────────────────────────────────────────────┤
│ X29     │ FP（Frame Pointer），栈帧指针。                              │
├─────────┼──────────────────────────────────────────────────────────────┤
│ X30     │ LR（Link Register），保存函数返回地址。                      │
└─────────┴──────────────────────────────────────────────────────────────┘

PSTATE 是处理器状态寄存器，保存当前执行状态。

分解成字段：

┌─────────┬──────────────────────────────────┬────────────────────────────────────────────┐
│ 位域    │ 含义                             │ 你的值                                     │
├─────────┼──────────────────────────────────┼────────────────────────────────────────────┤
│ N Z C V │ 条件标志（负数、零、进位、溢出） │ -Z-- 表示 Z（Zero）标志为 1，N/C/V 为 0    │
├─────────┼──────────────────────────────────┼────────────────────────────────────────────┤
│ EL      │ 异常级别（Exception Level）      │ EL1h = EL1，使用 SP_EL1 栈指针             │
├─────────┼──────────────────────────────────┼────────────────────────────────────────────┤
│ DAIF    │ 中断屏蔽位                       │ 看具体位                                   │
├─────────┼──────────────────────────────────┼────────────────────────────────────────────┤
│ M[4:0]  │ 执行状态                         │ 0x05 通常表示 EL1h（AArch64，使用 SP_EL1） │
└─────────┴──────────────────────────────────┴────────────────────────────────────────────┘

4. FPCR / FPSR

• FPCR：浮点控制寄存器（rounding mode、异常使能等）。
• FPSR：浮点状态寄存器（累积的浮点异常标志）。

你这里都是 00000000，表示没有活跃的浮点异常。


5. Q00 - Q31

这是 NEON SIMD 128 位寄存器，每个 Q 寄存器包含两个 64 位数值。

例如：

```text
  Q01=2e2e2e676e697469:6177203a6b726f77
```

把它按字节读出来（小端序）：
• 低 64 位：69 74 69 6e 67 2e 2e 2e → "iting..."
• 高 64 位：77 6f 72 6b 3a 20 77 61 → "work: wa"

合起来大概是 "working: waiting..." 这样的字符串片段。

这通常说明某个寄存器里保存了字符串指针或字符串内容，对调试很有用。




```txt
    CPU#0
    PC=ffff800080eaf590 X00=0000000000000000 X01=0000000000000000
    X02=0000000000274409 X03=0000000000000000 X04=ffff80007e18b000
    X05=4000000000000000 X06=0000078c4a34dc71 X07=0000000000000001
    X08=ffff800081f5d060 X09=ffff8000801b0d60 X10=0000000000000e40
    X11=00000000000000c0 X12=0000000000000000 X13=0000000000000000
    X14=0000000000000000 X15=0000000000000000 X16=0000000000000000
    X17=0000000000000000 X18=0000000000000000 X19=ffff800081b22008
    X20=ffff800081b22008 X21=ffff80007e18b000 X22=ffff800081f5c1c0
    X23=0000000000000000 X24=ffff800081f4e9c0 X25=ffff800081333230
    X26=ffff0000ffdb7280 X27=00000001383c3108 X28=00000001301700ac
    X29=ffff800081f43d30 X30=ffff800080eaf5c8  SP=ffff800081f43d30
    PSTATE=00000000404000c5 -Z-- EL1h     FPCR=00000000 FPSR=00000000
    Q00=0000000000000000:0000000000000000 Q01=2e2e2e676e697469:6177203a6b726f77
    Q02=0000000000000000:ffff000000000000 Q03=0000000000000000:0000000000000000
    Q04=3333333333333333:3333333333333333 Q05=0000000000000000:00000cccccc00000
    Q06=726168636f692c37:33343d6567617065 Q07=6e74726f68732c69:696373613d746573
    Q08=0000000000000000:0000000000000000 Q09=0000000000000000:0000000000000000
    Q10=0000000000000000:0000000000000000 Q11=0000000000000000:0000000000000000
    Q12=0000000000000000:0000000000000000 Q13=0000000000000000:0000000000000000
    Q14=0000000000000000:0000000000000000 Q15=0000000000000000:0000000000000000
    Q16=0000000000000000:0000000000000000 Q17=0000000000000000:0000000000000000
    Q18=0000000000000000:0000000000000000 Q19=0000000000000000:0000000000000000
    Q20=0000000000000000:0000000000000000 Q21=0000000000000000:0000000000000000
    Q22=0000000000000000:0000000000000000 Q23=0000000000000000:0000000000000000
    Q24=0000000000000000:0000000000000000 Q25=0000000000000000:0000000000000000
    Q26=0000000000000000:0000000000000000 Q27=0000000000000000:0000000000000000
    Q28=0000ffff81dd6a28:0000ffff81df4000 Q29=0000ffff81c55dd0:0000ffff81ca4798
    Q30=0000ffff81c8eb40:0000ffff81cb8e80 Q31=0000000000000000:0000006800ca90d0
```

## 也许应该在 aarch
1. 中断
2. simd

## simd
这个做什么的:
- [ ] linux/arch/arm64/kernel/fpsimd.c

```txt
[    0.212919] raid6: neonx8   gen() 14775 MB/s
[    0.281570] raid6: neonx4   gen() 35977 MB/s
[    0.350283] raid6: neonx2   gen() 40227 MB/s
[    0.418895] raid6: neonx1   gen() 35834 MB/s
[    0.487634] raid6: int64x8  gen() 15134 MB/s
[    0.556448] raid6: int64x4  gen() 14247 MB/s
[    0.625161] raid6: int64x2  gen() 12924 MB/s
[    0.693917] raid6: int64x1  gen() 11379 MB/s
[    0.694151] raid6: using algorithm neonx2 gen() 40227 MB/s
[    0.762664] raid6: .... xor() 31377 MB/s, rmw enabled
[    0.762924] raid6: using neon recovery algorithm
```

## entry

### syscall 的上下文保存

mac 物理机上
```txt
@[
    vfs_read+0
    __arm64_sys_read+36
    invoke_syscall+116
    el0_svc_common.constprop.0+72
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 125
```

虚拟机中观测到的:
```txt
- ??
  - el0t_64_sync
    - el0t_64_sync_handler
      - el0_svc
        - do_el0_svc
          - el0_svc_common
            - invoke_syscall
              - __invoke_syscall
                - __arm64_sys_read
                  - __se_sys_read
                    - __do_sys_read
                      - ksys_read
                        - vfs_read
```
### kvm 的切入和切出
有点难以观测

### 中断的上下文

在 ARM 的虚拟机中观测到的是
```txt
- el1h_64_irq
  - el1h_64_irq_handler
    - el1_interrupt
      - __el1_irq
        - irq_enter_rcu
          - tick_irq_enter
            - tick_nohz_irq_enter
              - ktime_get
                - timekeeping_get_ns
                  - timekeeping_get_delta
                    - tk_clock_read
                      - arch_counter_read
```

在 mac 中直接观测到的是:
```txt
@[
    arch_counter_read+0
    tick_irq_enter+96
    irq_enter_rcu+152
    el1_interrupt+40
    el1h_64_fiq_handler+24
    el1h_64_fiq+104
    cpuidle_enter_state+228
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+64
    secondary_start_kernel+224
    __secondary_switched+184
]: 459
```

## TODO
1. 感觉 arch/aarch64 下多了好多的代码，可以按照 diff 的行来排序一下

## 如何给 arm 的虚拟机注入一个 cache 的错误

```txt
[469.836076] {1}[Hardware Error]: Hardware error from APEI Generic Hardware Error Source: 9
[469.852780] {1}[Hardware Error]: event severity: recoverable
[469.862966] {1}[Hardware Error]:  Error 0, type: recoverable
[469.873206] {1}[Hardware Error]:   section_type: ARM processor error
[469.884267] {1}[Hardware Error]:   MIDR: 0x00000000481fd010
[469.894641] {1}[Hardware Error]:   Multiprocessor Affinity Register (MPIDR): 0x00000000810b0100
[469.913323] {1}[Hardware Error]:   error affinity level: 0
[469.924313] {1}[Hardware Error]:   running state: 0x1
[469.934815] {1}[Hardware Error]:   Power State Coordination Interface state: 0
[469.953129] {1}[Hardware Error]:   Error info structure 0:
[469.964350] {1}[Hardware Error]:   num errors: 1
[469.974606] {1}[Hardware Error]:    error_type: 0, cache error
[469.986055] {1}[Hardware Error]:    error_info: 0x0000000024400014
[469.997816] {1}[Hardware Error]:     cache level: 1
[470.008195] {1}[Hardware Error]:     the error has been corrected
[470.019785] {1}[Hardware Error]:    virtual fault address: 0x0000000000000000
[470.037840] {1}[Hardware Error]:    physical fault address: 0x000000917cabe738
[470.056404] {1}[Hardware Error]:   Vendor specific error info has 16 bytes:
[470.069368] {1}[Hardware Error]:    00000000: 00000000 00000000 00000000 00000000  ................
[470.090263] UCE: kernel recovery 0, qemu-kvm-2634199 is user-thread.
[470.103166] Internal error: Uncorrected hardware memory error in kernel-access : 96000610 [#1] SMP
```

## 那么 ARM 中各种 EL 是不是就是 x86 中的 SMM

可以简单这么理解，但是不是一个问题。

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
