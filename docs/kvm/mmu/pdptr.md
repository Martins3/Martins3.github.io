## PDPTR 是什么？
<!-- 4e41d22d-6fc2-4961-bf35-14af5684352e -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(我感觉基本都是正确的东西，后面可以继续细化确认一下)

PDPTR = Page Directory Pointer Table Register

这是 PAE (Physical Address Extension) 分页模式下使用的 4 个特殊寄存器，每个 64-bit。

为什么需要 PDPTR？

背景：32-bit PAE 模式
- 虚拟地址：32-bit (4GB)
- 物理地址：36-bit (64GB) ← 扩展！
- 页表项：64-bit (为了容纳 36-bit 物理地址)

页表结构：
CR3 → PDPT (4 entries) → PD (512 entries) → PT (512 entries) → Page
      ^^^^^^^^^^^^
      这 4 个 entry 就是 PDPTR！

关键点：PDPT 只有 4 个 entry，CPU 将它们缓存在内部寄存器中（PDPTR0-3），避免每次 page walk 都读内存。

何时加载 PDPTR？

触发条件：
1. MOV to CR3 - 修改页表根指针
2. MOV to CR0/CR4 - 修改某些分页相关位
3. Task switch

加载过程 (arch/x86/kvm/x86.c):
```txt
int load_pdptrs(struct kvm_vcpu *vcpu, unsigned long cr3)
{
    // 1. 读取 CR3 指向的 32 bytes (4 × 8 bytes)
    ret = kvm_vcpu_read_guest_page(vcpu, ..., pdpte,
                                   cr3 & GENMASK(11, 5), sizeof(pdpte));

    // 2. 验证 reserved bits
    for (i = 0; i < 4; ++i) {
        if ((pdpte[i] & PT_PRESENT_MASK) &&
            (pdpte[i] & pdptr_rsvd_bits(vcpu)))
            return 0;  // Invalid
    }

    // 3. 保存到 mmu->pdptrs[4]
    memcpy(mmu->pdptrs, pdpte, sizeof(mmu->pdptrs));

    return 1;
}
```

kvm_mmu 中的 pdptrs 字段

定义:
```c
struct kvm_mmu {
    u64 (*get_pdptr)(struct kvm_vcpu *vcpu, int index);
    // ...
    u64 pdptrs[4];  /* pae */
};
```

- pdptrs[4]：缓存 4 个 PDPTR 的值
- 只在 PAE 模式使用
- 避免每次都从 guest 内存读取

kvm_pdptr_read 函数

定义 (arch/x86/kvm/kvm_cache_regs.h):
```txt
static inline u64 kvm_pdptr_read(struct kvm_vcpu *vcpu, int index)
{
    might_sleep();  /* on svm */

    // 检查是否已缓存
    if (!kvm_register_is_available(vcpu, VCPU_EXREG_PDPTR))
        kvm_x86_call(cache_reg)(vcpu, VCPU_EXREG_PDPTR);  // 加载

    // 从 walk_mmu 返回（支持嵌套虚拟化）
    return vcpu->arch.walk_mmu->pdptrs[index];
}
```

功能：
1. 读取第 index 个 PDPTR (0-3)
2. 延迟加载：未缓存时先加载
3. 嵌套虚拟化支持：使用 walk_mmu 确保读 L1 或 L2 的 PDPTR

PAE 地址转换示例

32-bit 虚拟地址: 0x12345678

+--------+----------+----------+----------+
| 2 bits | 9 bits   | 9 bits   | 12 bits  |
+--------+----------+----------+----------+
    0       0x48      0x145      0x678

步骤：
1. PDPT index = 0 → kvm_pdptr_read(vcpu, 0) → 获取 PD base
2. PD index = 0x48 → Walk PD → 获取 PT base
3. PT index = 0x145 → Walk PT → 获取 page base
4. 页内偏移 = 0x678
5. 物理地址 = page_base + 0x678

为什么几乎总是赋值为 kvm_pdptr_read，只有 amd svm 的 nested mode
不一样 nested_svm_get_tdp_pdptr

context->get_pdptr = kvm_pdptr_read;

原因：
1. 通用实现：适用于所有分页模式
2. 非 PAE 模式：不会被调用，无影响
3. PAE 模式：提供正确的 PDPTR 读取
4. 嵌套虚拟化：通过 walk_mmu 支持 L2

核心要点

1. PDPTR 是 PAE 特有的：4 个 64-bit 寄存器，缓存 PDPT 的 4 个 entry
2. 性能优化：避免每次 page walk 都读内存
3. 特殊加载时机：只在 CR3/CR0/CR4 修改时重新加载
4. 32-byte 对齐：PDPT 必须在 32-byte boundary
5. kvm_pdptr_read：从 walk_mmu->pdptrs[index] 返回缓存的值

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
