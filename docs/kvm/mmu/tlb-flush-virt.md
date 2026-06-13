所以，作为 kvm 需要重点监控的是 invlpg

invvpid 的处理:
arch/x86/kvm/vmx/nested.c:handle_invvpid

对于 invpcid 的处理:
arch/x86/kvm/vmx/vmx.c:handle_invpcid
arch/x86/kvm/svm/svm.c:invpcid_interception

### kvm_mmu_invalidate_addr 是唯一的入口

关联具体的代码，也就是 amd 中刷掉 guest tlb 的操作:
```c
static void svm_flush_tlb_gva(struct kvm_vcpu *vcpu, gva_t gva)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	invlpga(gva, svm->vmcb->control.asid);
}
```

而关联 intel 中刷掉 guest tlb 的操作:
```c
void vmx_flush_tlb_gva(struct kvm_vcpu *vcpu, gva_t addr)
{
	/*
	 * vpid_sync_vcpu_addr() is a nop if vpid==0, see the comment in
	 * vmx_flush_tlb_guest() for an explanation of why this is ok.
	 */
	vpid_sync_vcpu_addr(vmx_get_current_vpid(vcpu), addr);
}
```
```c
static inline void vpid_sync_vcpu_addr(int vpid, gva_t addr)
{
	if (vpid == 0)
		return;

	if (cpu_has_vmx_invvpid_individual_addr())
		__invvpid(VMX_VPID_EXTENT_INDIVIDUAL_ADDR, vpid, addr);
	else
		vpid_sync_context(vpid);
}
```

## [ ] KVM_REQ_TLB_FLUSH
tlb flush 相关

```txt
@[
    vmx_vcpu_load_vmcs+5
    vmx_vcpu_load+23
    kvm_arch_vcpu_load+54
    finish_task_switch.isra.0+273
    __schedule+789
    schedule+97
    kvm_vcpu_block+104
    kvm_vcpu_halt+107
    kvm_arch_vcpu_ioctl_run+2419
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 46081
```

如果发现了切换成为新的 vCPU，那么需要将 pCPU 上原来的 vCPU 的上
```c
		/*
		 * Flush all EPTP/VPID contexts, the new pCPU may have stale
		 * TLB entries from its previous association with the vCPU.
		 */
		kvm_make_request(KVM_REQ_TLB_FLUSH, vcpu);
```

其实更多的调用是这个:
KVM_REQ_TLB_FLUSH_GUEST

- KVM_REQ_TLB_FLUSH_CURRENT

- KVM_REQ_HV_TLB_FLUSH

- x86 的指令存在那些指令？


## flush TLB

首先有这好几个方法:
1. KVM_REQ_TLB_FLUSH
2. KVM_REQ_TLB_FLUSH_CURRENT
3. KVM_REQ_TLB_FLUSH_GUEST
4. KVM_REQ_HV_TLB_FLUSH

其次，现在 TLB 上一共都使用了 id 来防止全部 flush 来着?

kvm_vcpu_flush_tlb_all


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
