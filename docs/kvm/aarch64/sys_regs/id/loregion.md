# aarch64 loregion
<!-- fd42ff0b-1614-409c-8b9f-e482540326dc -->

偶然发现 sys_reg_descs 中定义了很多类似的:
```c
static const struct sys_reg_desc sys_reg_descs[] = {
	// ...
	{ SYS_DESC(SYS_LORSA_EL1), trap_loregion },
	{ SYS_DESC(SYS_LOREA_EL1), trap_loregion },
	{ SYS_DESC(SYS_LORN_EL1), trap_loregion },
	{ SYS_DESC(SYS_LORC_EL1), trap_loregion },
	{ SYS_DESC(SYS_MPAMIDR_EL1), undef_access },
	{ SYS_DESC(SYS_LORID_EL1), trap_loregion },
	// ...
```

## kimi 解释
1. 一共有那些寄存器:

ARMv8.1 引入的可选特性 FEAT_LOR ，用于给操作系统提供一个“受限排序区域”的起始物理地址。
该寄存器在系统寄存器编码空间中的位置是:

┌───────────┬─────────────────────────────┐
│ 寄存器    │ 编码（Op0/Op1/CRn/CRm/Op2） │
├───────────┼─────────────────────────────┤
│ LORSA_EL1 │ 3 / 0 / 10 / 4 / 0          │
├───────────┼─────────────────────────────┤
│ LOREA_EL1 │ 3 / 0 / 10 / 4 / 1          │
├───────────┼─────────────────────────────┤
│ LORN_EL1  │ 3 / 0 / 10 / 4 / 2          │
├───────────┼─────────────────────────────┤
│ LORC_EL1  │ 3 / 0 / 10 / 4 / 3          │
├───────────┼─────────────────────────────┤
│ LORID_EL1 │ 3 / 0 / 10 / 4 / 7          │
└───────────┴─────────────────────────────┘

所以 KVM 表里那五行正好是一组：LORSA、LOREA、LORN、LORC、LORID。

2. LORegion 到底是做什么的

LORegion = Limited Ordering Region（受限排序区域），是 ARMv8.1 为大系统设计的一种内存序优化机制。

背景

在多核系统中，不同内存区域可能需要同程度的排序保证：

• 普通内存：性能优先，弱排序。
• 设备内存：需要强排序，但全系统屏障（DSB）开销很大。
• 某些外设或共享数据结构：只需要局部排序——即保证对该区域内的访问有序，但不必影响全局所有内存访问。

LORegion 就是用来表达这种“局部排序”需求的。

核心思想

软件可以划定一块物理地址区域作为 LORegion，然后使用特殊的加载/存储指令：

• LDLAR（Load-LOAcquire）
• STLLR（Store-LORelease）

这些指令的行为类似于普通的 LDAR/STLR，但排序作用范围被限制在同一个 LORegion 内，而不是全局所有观察者。

带来的好处：
• 比 DSB 轻量。
• 比普通 LDAR/STLR 更灵活，可以避免不必要的全局排序开销。
• 适合 NUMA、大型 SoC 中对某个 MMIO 区域或共享缓冲区的精细排序控制。

一个 LORegion descriptor 包含什么

┌──────────┬───────────────────────────┬───────────────────────────────┐
│ 字段     │ 寄存器                    │ 含义                          │
├──────────┼───────────────────────────┼───────────────────────────────┤
│ 起始地址 │ LORSA_EL1.SA              │ 区域起始物理地址（64KB 对齐） │
├──────────┼───────────────────────────┼───────────────────────────────┤
│ 结束地址 │ LOREA_EL1.EA              │ 区域结束物理地址              │
├──────────┼───────────────────────────┼───────────────────────────────┤
│ 区域编号 │ LORN_EL1.Num              │ 选择当前配置的 descriptor     │
├──────────┼───────────────────────────┼───────────────────────────────┤
│ 有效位   │ LORC_EL1.EN + LORC_EL1.DS │ 是否启用、选择哪个 descriptor │
└──────────┴───────────────────────────┴───────────────────────────────┘

典型配置流程：

```asm
  MOV  x0, #0x2            // 选择 LORegion descriptor 2
  MOV  x1, #0x80000000     // 起始地址
  MOV  x2, #0xC0000000     // 结束地址
  MOV  x3, #0x1            // 启用 (EN=1)

  MSR  LORN_EL1, x0        // 选择 descriptor
  ISB
  MSR  LORSA_EL1, x1       // 写起始地址
  MSR  LOREA_EL1, x2       // 写结束地址
  MSR  LORC_EL1, x3        // 启用
  ISB
```

之后对该区域内的地址使用 LDLAR/STLLR，就会按 LORegion 规则排序。

限制

• 只用于 Non-secure 物理内存。
• 只支持 AArch64。
• 如果 CPU 没有实现 FEAT_LOR，访问这些寄存器会 UNDEFINED。
• 如果实现了但 descriptor 数量为 0，那么这些 RW 寄存器是 RES0，行为等价于 RAZ/WI。

────────────────────────────────────────────────────────────────────────────────

3. 为什么 Linux/KVM 里这么多 LORegion 寄存器都走 trap_loregion

看 arch/arm64/kvm/sys_regs.c 里的注释：

```c
  /*
   * ARMv8.1 mandates at least a trivial LORegion implementation, where all the
   * RW registers are RES0 (which we can implement as RAZ/WI). On an ARMv8.0
   * system, these registers should UNDEF. LORID_EL1 being a RO register, we
   * treat it separately.
   */
```

意思是：

1. ARMv8.1 强制要求至少有一个 trivial 的 LORegion 实现：即使硬件不真正支持多个区域，也可以实现成 descriptor 数量为 0，所有 RW
寄存器为 RES0。
2. KVM 不打算把真实 LORegion 功能暴露给 Guest。
    • 如果 Host CPU 不支持 FEAT_LOR（ARMv8.0 或禁用了），Guest 访问 UNDEF。
    • 如果 Host CPU 支持，Guest 看到的 RW 寄存器全是 RAZ/WI，只读寄存器 LORID_EL1 返回一个“零区域”的合法值。
3. 这五个寄存器都是 LORegion 配置接口，所以统一用 trap_loregion 处理，代码复用。

EL2 启动时也清除了 LORegion

arch/arm64/include/asm/el2_setup.h：

```asm
  .macro __init_el2_lor
     mrs   x1, id_aa64mmfr1_el1
     ubfx   x0, x1, #ID_AA64MMFR1_EL1_LO_SHIFT, 4
     cbz   x0, .Lskip_lor_\@
     msr_s   SYS_LORC_EL1, xzr
  .Lskip_lor_\@:
  .endm
```

进入 EL2 时，如果支持 LORegion，就把 LORC_EL1 写 0，禁用所有 LORegion。这是为了防止上一个阶段遗留的配置影响虚拟机。

────────────────────────────────────────────────────────────────────────────────

一句话总结
- SYS_LORSA_EL1 是 ARMv8.1 LORegion 特性的“区域起始地址”寄存器。
- LORegion 是一种“局部内存排序”机制，用 LDLAR/STLLR 指令对指定物理区域做比全局屏障更轻量的排序控制。
- KVM 不把它暴露给 Guest，所以那一组寄存器统一用 trap_loregion 处理成 UNDEF 或 RAZ/WI。


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
