## 如何理解 msr exit 的优化?

如果都不去打开 apicv ，如果是 linux 虚拟机

```sh
sudo bpftrace -e "tracepoint:kvm:kvm_msr { @[kstack] = count(); }"
```

跑 linux 的时候:
```txt
@[
    kvm_emulate_rdmsr+271
    kvm_emulate_rdmsr+271
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+1673
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 12
@[
    handle_fastpath_set_msr_irqoff+420
    handle_fastpath_set_msr_irqoff+420
    kvm_arch_vcpu_ioctl_run+1376
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 265
```

但是 guest 是 windows 打开 hv-vapic 的时候:
```txt
@[
    kvm_hv_set_msr_common+5
    vmx_set_msr+3194
    __kvm_set_msr+145
    kvm_emulate_wrmsr+81 : 这个就是最近的了
    vmx_handle_exit+1829
    kvm_arch_vcpu_ioctl_run+1673
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 467
```
但是走到了这里。

可以看到 `handle_fastpath_set_msr_irqoff` 是 APIC_ICR 特殊优化的。

linux 虚拟机中，主要是 handle_fastpath_set_msr_irqoff ，是在对于 timer 进行编程:
```txt
   2.78%  msr_read 3b = 0x0
   1.85%  msr_write 6e0 = 0x3d744cd9e3
   1.85%  msr_write 6e0 = 0x3d751a9891
   0.93%  msr_write 6e0 = 0x3d647d4955
   0.93%  msr_write 6e0 = 0x3d7460dc13
   0.93%  msr_write 6e0 = 0x3d75456001
   0.93%  msr_write 6e0 = 0x3d7573141d
   0.93%  msr_write 6e0 = 0x3d75a0c7f7
   0.93%  msr_write 6e0 = 0x3d75ce7bf7
   0.93%  msr_write 6e0 = 0x3d84fb3ff3
   0.93%  msr_write 6e0 = 0x3d84fb401f
   0.93%  msr_write 6e0 = 0x3d8528f2a5
   0.93%  msr_write 6e0 = 0x3da70bbfbf
   0.93%  msr_write 6e0 = 0x3da70bc08b
   0.93%  msr_write 6e0 = 0x3da70bc113
   0.93%  msr_write 6e0 = 0x3da71487f1
   0.93%  msr_write 6e0 = 0x3dafd3fbf1
   0.93%  msr_write 6e0 = 0x3dafd3fce7
   0.93%  msr_write 6e0 = 0x3db32d7a91
   0.93%  msr_write 6e0 = 0x3db32d7b29
   0.93%  msr_write 6e0 = 0x3db33856f3
   0.93%  msr_write 6e0 = 0x3db41cdaf5
   0.93%  msr_write 6e0 = 0x3db41cdb1b
```

为什么这两个 MSR 可以特殊处理? 其他的不可以呀?

## 在虚拟机中观测 msr

arch/x86/lib/msr.c

可以找到三个 tracepoint

```txt
msr:write_msr
msr:read_msr
msr:rdpmc
```
## kvm 可以选择忽略掉对于不存在的 msr 访问的警告
<!-- 0db8062b-bf18-49df-a6cc-4ae5fa63ff97 -->

/sys/module/kvm/parameters 下存在两个参数

-  ignore_msrs : 默认为 0 ，如果为 1 ，那么对于不支持的 msr 访问，直接 #GP
-  report_ignored_msrs : 默认为 1 ，也就是如果遇到了不支持的 msr 访问，那么需要提供一个警告


```txt
	kvm.ignore_msrs=[KVM] Ignore guest accesses to unhandled MSRs.
			Default is 0 (don't ignore, but inject #GP)
```

两个选择的作用就是在 kvm_do_msr_access 函数中
```c
static __always_inline int kvm_do_msr_access(struct kvm_vcpu *vcpu, u32 msr,
					     u64 *data, bool host_initiated,
					     enum kvm_msr_access rw,
					     msr_access_t msr_access_fn)
{
  // ...

	if (!ignore_msrs) {
		kvm_debug_ratelimited("unhandled %s: 0x%x data 0x%llx\n",
				      op, msr, *data);
		return ret;
	}

	if (report_ignored_msrs)
		kvm_pr_unimpl("ignored %s: 0x%x data 0x%llx\n", op, msr, *data);

	return 0;
```

和 report_ignored_msrs 的关系
```txt
	if (ignore_msrs && !report_ignored_msrs) {
		pr_warn_once("Running KVM with ignore_msrs=1 and report_ignored_msrs=0 is not a\n"
			     "a supported configuration.  Lying to the guest about the existence of MSRs\n"
			     "may cause the guest operating system to hang or produce errors.  If a guest\n"
			     "does not run without ignore_msrs=1, please report it to kvm@vger.kernel.org.\n");
	}
```

也就是，默认情下，访问不存在的 msr ，那么就会有 GP 异常。

不要和 svm_set_msr 中的这部分逻辑搞混了
```c
	case MSR_IA32_DEBUGCTLMSR:
		if (!lbrv) {
			kvm_pr_unimpl_wrmsr(vcpu, ecx, data);
			break;
		}

		/*
		 * Suppress BTF as KVM doesn't virtualize BTF, but there's no
		 * way to communicate lack of support to the guest.
		 */
		if (data & DEBUGCTLMSR_BTF) {
			kvm_pr_unimpl_wrmsr(vcpu, MSR_IA32_DEBUGCTLMSR, data);
			data &= ~DEBUGCTLMSR_BTF;
		}

		if (data & DEBUGCTL_RESERVED_BITS)
			return 1;

		if (svm->vmcb->save.dbgctl == data)
			break;

		svm->vmcb->save.dbgctl = data;
		vmcb_mark_dirty(svm->vmcb, VMCB_LBR);
		svm_update_lbrv(vcpu);
		break;
```

这部分会通过这个打印日志:
```c
static inline void kvm_pr_unimpl_wrmsr(struct kvm_vcpu *vcpu, u32 msr, u64 data)
{
	if (report_ignored_msrs)
		vcpu_unimpl(vcpu, "Unhandled WRMSR(0x%x) = 0x%llx\n", msr, data);
}
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
