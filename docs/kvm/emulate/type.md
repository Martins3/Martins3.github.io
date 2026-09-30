# 到底是什么在触发

此前对仓库根目录下的 `a`（690946 行）分析结果如下。该原始采集文件已按需移除，
以下数字是历史分析记录；它与 [emulate-insn-results.txt](emulate-insn-results.txt)
中的另一次采样不要混用。

┌────────┬────────┬───────────────────────────────────┐
│ 类型   │ 数量   │ 是否都在 call..return 之间        │
├────────┼────────┼───────────────────────────────────┤
│ CALL   │ 27364  │ —                                 │
├────────┼────────┼───────────────────────────────────┤
│ RETURN │ 27364  │ —                                 │
├────────┼────────┼───────────────────────────────────┤
│ INSN   │ 20608  │ 全部在窗口内                      │
├────────┼────────┼───────────────────────────────────┤
│ MMIO   │ 27291  │ 全部在窗口内                      │
├────────┼────────┼───────────────────────────────────┤
│ PIO    │ 588313 │ 只有 24 条在窗口内，588289 条越界 │
└────────┴────────┴───────────────────────────────────┘

所以答案是：不是。该样本的 INSN/MMIO 都在 call..return 之间，但绝大多数 PIO 不在。
原始输入和逐条检查报告当前未保留，重新采集后可使用
[check_call_return.py](check_call_return.py) 生成配对报告。

为什么这些 PIO 没有 CALL/RETURN 包裹

关键在 VMX 的 PIO 退出处理 handle_io()（arch/x86/kvm/vmx/vmx.c）：

```c
  static int handle_io(struct kvm_vcpu *vcpu)
  {
      ...
      string = (exit_qualification & 16) != 0;   // 是否为串 I/O (rep ins/outs)

      if (string)
          return kvm_emulate_instruction(vcpu, 0);  // 走完整模拟 → 有 CALL/RETURN

      ...
      return kvm_fast_pio(vcpu, size, port, in);    // 走 fast PIO 路径 → 无 CALL/RETURN
  }
```

KVM 有两条 PIO 路径：

1. 串 I/O（rep ins/rep outs）：kvm_emulate_instruction() → x86_emulate_instruction()，fentry/fexit 探针命
   中 → 产生 CALL/INSN/RETURN；模拟器内部回调 emulator_pio_in_emulated/emulator_pio_out_emulated 时
   trace_kvm_pio 触发 → PIO 落在窗口内。

2. 非串 I/O（单条 in/out）：kvm_fast_pio() → kvm_fast_pio_in/out() → 直接调
   emulator_pio_in()/emulator_pio_out()（arch/x86/kvm/x86.c），这两个函数内部 trace_kvm_pio() 直接触发，根
   本不经过 x86_emulate_instruction，所以没有 CALL/RETURN。

我用数据验证了这个对应关系，完全吻合：

```
  窗口内  PIO 的 count 分布: {4: 24}      ← 全是 count=4 的 rep insb
  窗口外  PIO 的 count 分布: {1: 588289}  ← 全是 count=1 的单条 in/out
```

（trace_kvm_pio 的 rw 字段定义在 arch/x86/kvm/trace.h：KVM_PIO_IN=0、KVM_PIO_OUT=1。）

你看到的 3f8/3f9/3fa/3fd/3fe 是什么

这是 16550 UART（COM1 串口）的寄存器：3f8=THR 发送、3f9=IER、3fa=IIR、3fd=LSR、3fe=MSR。guest 在往串口控制
台逐字节打印（out 一个字符一条 PIO，0x20 是空格、0x0d 是回车），每条都是单字节非串 I/O，走 fast PIO 路径，
所以没有 CALL/RETURN 包裹。

这里必须区分两个不同的概念：

- **fast PIO**：普通 IN/OUT 由 `kvm_fast_pio()` 完成，通常不进入
  `x86_emulate_instruction()`，所以只有 PIO 记录，没有完整模拟器的 CALL/RETURN。
- **`EMULTYPE_NO_DECODE`**：用户态 I/O 返回后，仍进入 `x86_emulate_instruction()`，
  复用此前的解码结果继续模拟，所以有 CALL/RETURN，通常没有新的 INSN start 记录。

当前 [emulate-insn.bt](emulate-insn.bt) 没有按 CALL 窗口过滤 MMIO/PIO，因此也会捕获
fast PIO；fentry/fexit 只标识完整模拟器的入口和返回。

`kvm_pio` 与 `kvm_mmio` 是两种设备访问事件。PIO 可以走专用快速路径或完整模拟器；
MMIO 可以在内核完成，也可以退出 QEMU。它们与是否设置 `NO_DECODE` 是不同维度。

脚本

[check_call_return.py](check_call_return.py) 默认输入是仓库根目录下的 `a`，
即 `/home/martins3/data/vn/a`，默认输出是脚本同目录下的 `a-check.txt`。
历史输入已移除，重新采集时应显式指定新输入文件。脚本自动识别有无时间戳两种列格式，
跳过末尾的 `@map` 汇总行；空输入或没有 CALL 的输入不再报告成功，输入不存在时返回明确错误。
发现窗口外 PIO 会让检查返回非零；这只是“不满足全部事件位于模拟器内”的约束，
对于包含 fast PIO 的采样并不等于 KVM 或采集程序发生错误。

## EMULTYPE_* 为什么没有全部出现

`arch/x86/include/asm/kvm_host.h` 的 `EMULTYPE_*` 描述模拟的触发条件与执行方式，
它们是可组合的 bitmask，不是设备类型枚举；源码没有 `EMULTYPE_PIO` 或 `EMULTYPE_MMIO`。

读取采集结果时要分清三项：

| 采集字段 | 描述什么 | 例子 |
| --- | --- | --- |
| CALL 的 `emulation_type` | 模拟入口的特殊标志 | `PF`、`NO_DECODE`、`TRAP_UD`、`SKIP` |
| CALL 的 `exit_reason` | 最近一次硬件 VM exit 原因 | EPT_VIOLATION、EPT_MISCONFIG、IO_INSTRUCTION、APIC_ACCESS |
| MMIO/PIO 记录 | 模拟过程中或专用 I/O 路径发生的设备访问 | 设备 GPA、端口、宽度、数据 |

INSN 记录的 `flags` 又是 guest 的解码模式，如 prot16/prot32/prot64，
不是 CALL 的 `emulation_type`。`NO_DECODE` 续接没有新的硬件退出，
探针记录的 `exit_reason` 此时是之前缓存的原因。

### 已保存的启动样本实际上出现了哪些标志

[emulate-insn-results.txt](emulate-insn-results.txt) 中保留的统计是：

| `emulation_type` | 次数 | 含义 |
| --- | ---: | --- |
| `64 = 0x40 = EMULTYPE_PF` | 35089 | 经 MMU 页故障路径触发，本次样本中对应 MMIO |
| `1 = EMULTYPE_NO_DECODE` | 1510 | 已解码指令的用户态 I/O 续接 |
| `0` | 55 | 完整模拟，没有设置特殊标志：48 次 string PIO、7 次 APIC access |

因此这个样本也不只包含“PIO 与 MMIO 两种标志”。`PF` 不是 MMIO 的同义词：
shadow 页表写保护等场景也会带 `PF`。`0` 也不是 PIO 的同义词：UMIP、
invalid guest state 等完整模拟入口也可以传入 `0`。

### 其他标志各需要什么条件

以下源码路径以 `arch/x86/kvm/` 为基准。

| 标志 | 设置条件与源码入口 | 为什么普通启动样本可能没有 |
| --- | --- | --- |
| `TRAP_UD` | `x86.c` 的 `handle_ud()` 处理被截获的 #UD；只允许 `EmulateOnUD` 指令进入相应兼容模拟 | 此次采样没有捕获到触发该入口的 #UD |
| `SKIP` | VMX 的 `skip_emulated_instruction()` 或 SVM 的 `__svm_skip_emulated_instruction()` 在硬件不能提供可靠下一 RIP 时，借用解码器找长度 | 普通 VMX exit 通常已有 `VM_EXIT_INSTRUCTION_LEN`，直接推进 RIP，无需进入完整模拟器 |
| `ALLOW_RETRY_PF` | `mmu/mmu.c` 的 `kvm_mmu_write_protect_fault()` 在允许拆除 shadow 映射并重试时设置，与 `PF` 组合 | 本次主要是 MMIO，不能靠拆表重试解决；普通非 nested EPT guest 也不需要 shadow guest 页表 |
| `TRAP_UD_FORCED` | `handle_ud()` 识别启用后的 KVM force-emulation prefix，用于测试模拟器 | 需要显式启用并执行特殊前缀；检查时宿主 `force_emulation_prefix=0` |
| `VMWARE_GP` | VMware backdoor 兼容路径接管特定 #GP，只处理约定的 IN/OUT、INS/OUTS、RDPMC | 需要启用 backdoor 且执行相应访问；检查时宿主 `enable_vmware_backdoor=N` |

当前源码还包含 `COMPLETE_USER_EXIT`、`WRITE_PF_TO_SP`、`SKIP_SOFT_INT` 等标志，
分别用于跳过指令后完成用户态退出相关状态、限制自修改 shadow 页表的失败重试，
以及检查软件中断指令后跳过。上面那份旧定义并未列出当前所有标志。

组合也需要按位判断，例如 `PF | ALLOW_RETRY_PF` 是 `64 + 8 = 72`，
不能通过 `emulation_type == 64` 来统计所有带 PF 的调用。`0` 则应单独统计。
`NO_DECODE` 也可以与其他标志组合：SVM 的 `gp_interception()` 先解码，
再以 `VMWARE_GP | NO_DECODE`（32 + 1 = 33）进入模拟器；这次复用解码并非用户态 I/O 续接。

### 没有 MMIO/PIO 记录，也可能发生完整模拟

例如某些 #UD 兼容指令只读取普通 guest RAM、修改寄存器或 CPU 状态，
不会产生设备访问事件，但仍可产生 CALL、INSN、RETURN。
`SKIP` 也可能只有解码和 RIP 更新，不执行内存/设备副作用。

`emulate-insn.bt` 的入口探针没有按 `emulation_type` 过滤，因而这些路径一旦发生，
相应标志仍会出现在 CALL 中；解码时也会产生 INSN 记录。
目前只监听 MMIO/PIO 两种设备访问 tracepoint，并不意味着模拟器只处理这两种操作。
若要观察 CR、MSR、段或描述符等内部动作，还需要相应的额外探针。

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
