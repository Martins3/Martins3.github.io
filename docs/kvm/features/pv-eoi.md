# pv eoi
## 如何初始化

```c
int kvm_set_msr_common(struct kvm_vcpu *vcpu, struct msr_data *msr_info)
{
  // ...
	case MSR_KVM_PV_EOI_EN:
		if (!guest_pv_has(vcpu, KVM_FEATURE_PV_EOI))
			return 1;

		if (kvm_lapic_set_pv_eoi(vcpu, data, sizeof(u8)))
			return 1;
		break;
  // ...
```

```txt
@[
    kvm_lapic_set_pv_eoi+5
    kvm_set_msr_common+2860
    vmx_set_msr+2660
    __kvm_set_msr+171
    kvm_set_msr_ignored_check+24
    kvm_emulate_wrmsr+78
    vmx_handle_exit+1113
    kvm_arch_vcpu_ioctl_run+6726
    kvm_vcpu_ioctl+1562
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 2
@[
    kvm_lapic_set_pv_eoi+5
    kvm_set_msr_common+2860
    vmx_set_msr+2660
    __kvm_set_msr+171
    kvm_set_msr_ignored_check+24
    kvm_arch_vcpu_ioctl+4247
    kvm_vcpu_ioctl+1481
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 4
```
这个路径应该是很清楚的了，就是 guest 初始化，来告诉 host 物理地址的。

## host 的处理

全部都是在 vcpu_enter_guest 中:

1. 进入 guest moe 之前
 - kvm_check_request(KVM_REQ_EVENT, vcpu) : 如果检查有 event ，才会向下:
  - kvm_lapic_sync_to_vapic : 当进入到 guest 的时候，并且 kvm_check_request(KVM_REQ_EVENT, vcpu) 的时候
    - apic_sync_pv_eoi_to_guest
      - pv_eoi_set_pending -> pv_eoi_put_user -> kvm_write_guest_cached 修改 guest os 的内存
      - 给 kvm_vcpu_arch::apic_attention 设置上 `KVM_APIC_PV_EOI_PENDING`


2. 从 guest mode 离开后:
  - kvm_lapic_sync_from_vapic : 从 guest 的退出的时候，如果检查到了 kvm_vcpu_arch::apic_attention 有 bit `KVM_APIC_PV_EOI_PENDING`
    - apic_sync_pv_eoi_from_guest
      - pv_eoi_test_and_clr_pending : 清理 guest 的 flag
      - 如果 guest 中请求 apic_set_eoi : 模拟 apic_set_eoi 的行为

总体的原理 guest 将 eoi 延迟执行，当其他的原因 exit 出现的时候，来检查 bit ，如果发现 bit 被 clear 了，说明 guest 有 eoi 的请求。

### 如果没有 enable_apicv ，并且没有 pv eoi ，那么路径是如下
```txt
@[
    apic_set_eoi+1
    kvm_lapic_reg_write+1139
    vmx_set_msr+2660
    __kvm_set_msr+171
    kvm_set_msr_ignored_check+24
    kvm_emulate_wrmsr+78
    vmx_handle_exit+1113
    kvm_arch_vcpu_ioctl_run+6726
    kvm_vcpu_ioctl+1562
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 66
```
如果一次 exit 之前，有两个 eoi 请求，那么也是走的这个路径

## guest os 的处理过程

显然，pv eoi 是一个 apicv 还没有出现的时候有的

`kvm_guest_apic_eoi_write` 就是当虚拟机中实现的 eoi 的实现模式:

```txt
#0  kvm_guest_apic_eoi_write () at arch/x86/kernel/kvm.c:345
#1  0xffffffff81139a31 in apic_eoi () at ./arch/x86/include/asm/apic.h:415
#2  __sysvec_apic_timer_interrupt (regs=0xffffc9000015fe38) at arch/x86/kernel/apic/apic.c:1047
#3  0xffffffff828ccc61 in instr_sysvec_apic_timer_interrupt (regs=0xffffc9000015fe38) at arch/x86/kernel/apic/apic.c:1043
#4  sysvec_apic_timer_interrupt (regs=0xffffc9000015fe38) at arch/x86/kernel/apic/apic.c:1043
```

```c
static notrace __maybe_unused void kvm_guest_apic_eoi_write(void)
{
	/**
	 * This relies on __test_and_clear_bit to modify the memory
	 * in a way that is atomic with respect to the local CPU.
	 * The hypervisor only accesses this memory from the local CPU so
	 * there's no need for lock or memory barriers.
	 * An optimization barrier is implied in apic write.
	 */
	if (__test_and_clear_bit(KVM_PV_EOI_BIT, this_cpu_ptr(&kvm_apic_eoi))) // 如果 kvm 关闭，enable_apicv ，那么走这里
		return;
	apic_native_eoi(); // 如果 kvm 打开了 enable_apicv ，那么走这里
}
```


## 和 apicv 的关系
如果 enable_apicv = 1 ，那么这些东西都不会触发，因为 kvm_check_request 不会通过

## 如果真正的启用 eoi : kvm enable_apicv=0

似乎慢速路径的处理是:
- handle_apic_access
  - kvm_lapic_set_eoi
    - kvm_lapic_reg_write
      - apic_set_eoi

## 问题

总是无法调用到这个上
```c
	[EXIT_REASON_EOI_INDUCED]             = handle_apic_eoi_induced,
```
## 参考

引入 eoi 的原始 patch :
```diff
History:        #0
Commit:         ab9cf4996bb989983e73da894b8dd0239aa2c3c2
Author:         Michael S. Tsirkin <mst@redhat.com>
Committer:      Avi Kivity <avi@redhat.com>
Author Date:    Mon 25 Jun 2012 12:24:34 AM CST
Committer Date: Mon 25 Jun 2012 05:38:06 PM CST

KVM guest: guest side for eoi avoidance

The idea is simple: there's a bit, per APIC, in guest memory,
that tells the guest that it does not need EOI.
Guest tests it using a single est and clear operation - this is
necessary so that host can detect interrupt nesting - and if set, it can
skip the EOI MSR.
```

- https://liujunming.top/2023/08/26/Notes-about-PV-EOI/
- https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2020/09/10/kvm-performance-1

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
