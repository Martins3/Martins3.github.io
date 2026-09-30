# 内核虚拟机地址空间
<!-- d843cd2a-cb47-4e1c-8ba6-c9112557dbcf -->

- https://www.kernel.org/doc/html/latest/arch/arm64/memory.html
- https://www.kernel.org/doc/html/latest/arch/x86/x86_64/mm.html

> [!NOTE]
> 参考神奇海螺的意见，有待验证

 方面              x86-64                                                       ARM64
━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 页表根            主要通过 CR3                                                 用户、内核常分别使用 TTBR0_EL1、TTBR1_EL1
────────────────  ───────────────────────────────────────────────────────────  ────────────────────────────────────────────────────
 地址宽度          4 级 48 位或 LA57 57 位                                      由 TCR_EL1、VA_BITS 和页粒度共同决定
────────────────  ───────────────────────────────────────────────────────────  ────────────────────────────────────────────────────
 内存属性          PAT/MTRR                                                     MAIR_EL1
────────────────  ───────────────────────────────────────────────────────────  ────────────────────────────────────────────────────
 二阶段转换        EPTP/EPT                                                     VTTBR_EL2、VTCR_EL2
────────────────  ───────────────────────────────────────────────────────────  ────────────────────────────────────────────────────
 内核布局随机化    page_offset_base、vmalloc_base、vmemmap_base 可动态随机化    同样支持 KASLR，但大量布局边界由架构配置和宏公式共同决定
────────────────  ───────────────────────────────────────────────────────────  ────────────────────────────────────────────────────
 历史包袱          segmentation、GDT/TSS、4/5 级兼容、KPTI 等                   EL 分层和双 TTBR 模型相对规整

arch/x86/mm/dump_pagetables.c 中定义了:
```c
enum address_markers_idx {
	USER_SPACE_NR = 0,
	KERNEL_SPACE_NR,
#ifdef CONFIG_MODIFY_LDT_SYSCALL
	LDT_NR,
#endif
	LOW_KERNEL_NR,
	VMALLOC_START_NR,
	VMEMMAP_START_NR,
#ifdef CONFIG_KASAN
	KASAN_SHADOW_START_NR,
	KASAN_SHADOW_END_NR,
#endif
	CPU_ENTRY_AREA_NR,
#ifdef CONFIG_X86_ESPFIX64
	ESPFIX_START_NR,
#endif
#ifdef CONFIG_EFI
	EFI_END_NR,
#endif
	HIGH_KERNEL_NR,
	MODULES_VADDR_NR,
	MODULES_END_NR,
	FIXADDR_START_NR,
	END_OF_SPACE_NR,
};
```

## 问题
1. 4-level 和 5-level 在 layout 的区分只是 start address 和 length 的区别
5. *只是 ioremap 的开始位置为什么和 vmalloc_base 使用的位置相同*
6. cpu_entry_area : https://unix.stackexchange.com/questions/476768/what-is-cpu-entry-area

- FDT 可以通过早期 fixmap 建立永久或半永久映射，因此 fixmap 边界不能被 vmalloc 分配器侵入。



## 如何直观的观察
打开选项 : CONFIG_PTDUMP=y

x86 debugfs 接口均可用：

```txt
/sys/kernel/debug/page_tables/kernel
/sys/kernel/debug/page_tables/current_kernel
/sys/kernel/debug/page_tables/current_user
/sys/kernel/debug/page_tables/efi
```

kernel 输出约 1600 行，marker 和运行时地址正确，例如：

```txt
---[ Low Kernel Mapping ]---
0xffff888000001000-...

---[ vmalloc() Area ]---
0xffffc90000000000-...

---[ Vmemmap ]---
0xffffea0000000000-...

---[ High Kernel Mapping ]---
0xffffffff80000000-...
```

这些基址与 drgn 读取的 page_offset_base、vmalloc_base、vmemmap_base 完全一致。

## 功能历史演化
```txt
 时间    内核版本    变化
━━━━━━  ━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 2008      2.6.26    x86 引入 X86_PTDUMP 页表 dump
──────  ──────────  ────────────────────────────────────────────────
 2020         5.6    引入通用 ptdump、PTDUMP_CORE 和 PTDUMP_DEBUGFS
──────  ──────────  ────────────────────────────────────────────────
 2025        6.15    PTDUMP_CORE 重命名为现在的 PTDUMP
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
