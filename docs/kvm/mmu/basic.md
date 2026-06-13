## EXIT_REASON_EPT_VIOLATION vs EXIT_REASON_EPT_MISCONFIG
<!-- 7ab504a5-aa30-493d-bc12-f1ec9c7a431b -->

EXIT_REASON_EPT_MISCONFIG 就是给 mmio 的优化，mmio 需要配置特殊的 page table
handle_ept_misconfig 走到，不然每次 mmio 都需要 page table walk 非常的慢，

触发过程:
```c
	if (unlikely(error_code & PFERR_RSVD_MASK)) {
		if (WARN_ON_ONCE(error_code & PFERR_PRIVATE_ACCESS))
			return -EFAULT;

		r = handle_mmio_page_fault(vcpu, cr2_or_gpa, direct);
		if (r == RET_PF_EMULATE)
			goto emulate;
	}
```

配置过程 : 应该是在 make_mmio_spte

```c
u64 make_mmio_spte(struct kvm_vcpu *vcpu, u64 gfn, unsigned int access)
{
	u64 gen = kvm_vcpu_memslots(vcpu)->generation & MMIO_SPTE_GEN_MASK;
	u64 spte = generation_mmio_spte_mask(gen);
	u64 gpa = gfn << PAGE_SHIFT;

	access &= shadow_mmio_access_mask;
	spte |= vcpu->kvm->arch.shadow_mmio_value | access;
	spte |= gpa | shadow_nonpresent_or_rsvd_mask;
	spte |= (gpa & shadow_nonpresent_or_rsvd_mask)
		<< SHADOW_NONPRESENT_OR_RSVD_MASK_LEN;

	return spte;
}
```

如果遇到不在 memslot 的访问，就会填充上这个特殊的结构体:
```txt
@[
        make_mmio_spte+5
        tdp_mmu_map_handle_target_level+363
        kvm_tdp_mmu_map+1087
        kvm_tdp_page_fault+196
        kvm_mmu_do_page_fault+473
        kvm_mmu_page_fault+134
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+973
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+736
        kvm_vcpu_ioctl+279
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 18
```

## x86 page table entry flags 的定义
<!-- 22463d4c-a5ae-4fa6-9356-2c839d73f04a -->
定义在 arch/x86/include/asm/pgtable_types.h

```c
#define _PAGE_BIT_PRESENT	0	/* is present */
#define _PAGE_BIT_RW		1	/* writeable */
#define _PAGE_BIT_USER		2	/* userspace addressable */
#define _PAGE_BIT_PWT		3	/* page write through */
#define _PAGE_BIT_PCD		4	/* page cache disabled */
#define _PAGE_BIT_ACCESSED	5	/* was accessed (raised by CPU) */
#define _PAGE_BIT_DIRTY		6	/* was written to (raised by CPU) */
#define _PAGE_BIT_PSE		7	/* 4 MB (or 2MB) page */
#define _PAGE_BIT_PAT		7	/* on 4KB pages */
#define _PAGE_BIT_GLOBAL	8	/* Global TLB entry PPro+ */
#define _PAGE_BIT_SOFTW1	9	/* available for programmer */
#define _PAGE_BIT_SOFTW2	10	/* " */
#define _PAGE_BIT_SOFTW3	11	/* " */
#define _PAGE_BIT_PAT_LARGE	12	/* On 2MB or 1GB pages */
#define _PAGE_BIT_SOFTW4	57	/* available for programmer */
#define _PAGE_BIT_SOFTW5	58	/* available for programmer */
#define _PAGE_BIT_PKEY_BIT0	59	/* Protection Keys, bit 1/4 */
#define _PAGE_BIT_PKEY_BIT1	60	/* Protection Keys, bit 2/4 */
#define _PAGE_BIT_PKEY_BIT2	61	/* Protection Keys, bit 3/4 */
#define _PAGE_BIT_PKEY_BIT3	62	/* Protection Keys, bit 4/4 */
#define _PAGE_BIT_NX		63	/* No execute: only valid after cpuid check */
```

关于 XD 位:
定义位置： arch/x86/include/asm/msr-index.h

```txt
#define MSR_IA32_MISC_ENABLE_XD_DISABLE_BIT           34
#define MSR_IA32_MISC_ENABLE_XD_DISABLE                       (1ULL << MSR_IA32_MISC_ENABLE_XD_DISABLE_BIT)
```

硬件语义： 与 NX 相同，都是控制页面是否可执行的位，Intel 称之为 XD（Execute Disable），AMD 称之为 NX（No-Execute）

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
