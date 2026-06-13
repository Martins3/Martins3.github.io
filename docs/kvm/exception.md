# kvm 如何处理 exception
<!-- 401cc0e1-547a-46f0-b199-7ad9c68b650a -->

handle_exception_nmi 是 VMCS Exception Bitmap 中所有被拦截的异常的统一入口。
理论上，只要 KVM 在 VMCS 的 Exception Bitmap 中设置了对应 bit，任何 x86 异常发生 VM-Exit 后都会进入
`handle_exception_nmi`


1. mce 和 nmi 特殊处理
```c
if (is_machine_check(intr_info) || is_nmi(intr_info))
    return 1;
```

2. 在 `switch` 之前，代码对几种常见异常做了硬编码的 fast-path** 处理：

```c
if (is_nm_fault(intr_info))      → #NM (Device Not Available, 向量 7)
if (is_invalid_opcode(intr_info)) → #UD (Invalid Opcode, 向量 6)
if (is_ve_fault(intr_info))       → #VE (Virtualization Exception, 向量 20) —— 仅 WARN
if (is_gp_fault(intr_info))       → #GP (General Protection, 向量 13)
if (is_page_fault(intr_info))     → #PF (Page Fault, 向量 14)
```

| 异常    | 处理方式                                                                               |
|---------|----------------------------------------------------------------------------------------|
| **#NM** | 重新排队注入 Guest（FPU/XFD 相关懒加载）                                               |
| **#UD** | 调用 `handle_ud()` 尝试 emulate（如 cpuid、vmx 指令等）                                |
| **#VE** | `WARN_ON_ONCE` + dump VMCS（正常情况下 #VE 不该走这里）                                |
| **#GP** | VMware backdoor 模拟（`IN{S}`/`OUT{S}`/`RDPMC`）；如果有 error code 则直接注入回 Guest |
| **#PF** | `vmx_handle_page_fault()`                                                              |

3. `switch (ex_no)` 处理的异常

以上都不是的话，进入最终的 `switch`：

```c
switch (ex_no) {
case DB_VECTOR:   // → #DB (Debug, 向量 1)
case BP_VECTOR:   // → #BP (Breakpoint, 向量 3)
case AC_VECTOR:   // → #AC (Alignment Check, 向量 17)
default:          // → 其他所有异常，提交给用户态，让用户态处理
}
```

| 异常 | 处理方式 |
|------|---------|
| **#DB** | 区分 Guest debug 模式（`KVM_EXIT_DEBUG`）和普通模式（注入 Guest） |
| **#BP** | `KVM_EXIT_DEBUG`，用于 guest debugging / `int3` |
| **#AC** | 检查是否需要注入 Guest；否则处理 split lock（`handle_guest_split_lock()`） |
| **default** | 直接透传：`KVM_EXIT_EXCEPTION` → userspace |


## 为什么 handle_exception_nmi 里还有 #PF？

正常情况下，启用 EPT 时 KVM 根本不去拦截 Guest 的 #PF。Guest 里发生缺页，就在 Guest 里自己处理，不会 VM-Exit。

但 KVM 有一个特殊需求：allow_smaller_maxphyaddr。

当 Host 允许 Guest 的物理地址宽度比 Host 更小时：
- Guest 认为 0x1_0000_0000 是合法物理地址（Guest 的 MAXPHYADDR 小）。
- 但 Host 根本不支持这么大的物理地址（Host MAXPHYADDR 更小）。
- 硬件 EPT 不会自动报错（因为硬件按 Host 的宽度检查，这个地址超范围了，但走的是另一条路径）。

所以 KVM 需要主动 intercept #PF（在 VMCS Exception Bitmap 里打开 #PF 拦截），当 Guest 页表转换时触及了这种"对 Host 非法的 GPA"，KVM 捕获
#PF，修正 error code（设置 PFERR_RSVD），再注入回 Guest。

```c
static int vmx_handle_page_fault(struct kvm_vcpu *vcpu, u32 error_code)
{
	unsigned long cr2 = vmx_get_exit_qual(vcpu);

	if (vcpu->arch.apf.host_apf_flags)
		goto handle_pf;

	/* When using EPT, KVM intercepts #PF only to detect illegal GPAs. */
	WARN_ON_ONCE(enable_ept && !allow_smaller_maxphyaddr);

	/*
	 * If EPT is enabled, fixup and inject the #PF.  KVM intercepts #PFs
	 * only to set PFERR_RSVD as appropriate (hardware won't set RSVD due
	 * to the GPA being legal with respect to host.MAXPHYADDR).
	 */
	if (enable_ept) {
		kvm_fixup_and_inject_pf_error(vcpu, cr2, error_code);
		return 1;
	}

handle_pf:
	return kvm_handle_page_fault(vcpu, error_code, cr2, NULL, 0);
}

```

注释说的很清楚：EPT 启用时，KVM 拦截 #PF 的唯一理由就是检测非法 GPA。

## 实验
```diff
diff --git a/arch/x86/kvm/vmx/vmx.c b/arch/x86/kvm/vmx/vmx.c
index 47f899c23d35..b83fe9d8fe76 100644
--- a/arch/x86/kvm/vmx/vmx.c
+++ b/arch/x86/kvm/vmx/vmx.c
@@ -870,7 +870,7 @@ void vmx_update_exception_bitmap(struct kvm_vcpu *vcpu)
 	u32 eb;

 	eb = (1u << PF_VECTOR) | (1u << UD_VECTOR) | (1u << MC_VECTOR) |
-	     (1u << DB_VECTOR) | (1u << AC_VECTOR);
+	     (1u << DB_VECTOR) | (1u << AC_VECTOR) | (1u << DE_VECTOR);
 	/*
 	 * #VE isn't used for VMX.  To test against unexpected changes
 	 * related to #VE for VMX, intercept unexpected #VE and warn on it.
@@ -5371,6 +5371,10 @@ static int handle_exception_nmi(struct kvm_vcpu *vcpu)
 		if (handle_guest_split_lock(kvm_rip_read(vcpu)))
 			return 1;
 		fallthrough;
+	case DE_VECTOR:
+		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
+		kvm_queue_exception(vcpu, DE_VECTOR);
+		return 1;
 	default:
 		kvm_run->exit_reason = KVM_EXIT_EXCEPTION;
 		kvm_run->ex.exception = ex_no;
```

在 guest os 中触发 DE，可以看到日志

如果将 `kvm_queue_exception` 去掉，那么:
1. guest 会永远停留在除法指令上
2. 可以观测到高频的 handle_exception_nmi

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
