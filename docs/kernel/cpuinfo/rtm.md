# rtm

- 不建议使用，因为有 bug ，也没人使用，
- https://www.kernel.org/doc/html/latest/admin-guide/hw-vuln/tsx_async_abort.html
- https://docs.kernel.org/x86/tsx_async_abort.html
- https://access.redhat.com/articles/tsx-asynchronousabort

```txt
- KVM: x86: Allow guests to see MSR_IA32_TSX_CTRL even if tsx=off (Paolo Bonzini) [1912448]
```

[4/5] KVM: vmx: implement MSR_IA32_TSX_CTRL disable RTM functionality

- https://patchwork.kernel.org/project/kvm/patch/1574101067-5638-5-git-send-email-pbonzini@redhat.com/

- 寄存器 MSR_IA32_TSX_CTRL 是做啥的？


## 想不到其实是支持的
```c
void tm_increment(TMCounter *tm_counter) {
  __transaction_atomic { tm_counter->counter++; }
}
```

```md
Dump of assembler code for function tm_increment:
   0x00000000004011a6 <+0>:     push   %rbp
   0x00000000004011a7 <+1>:     mov    %rsp,%rbp
   0x00000000004011aa <+4>:     sub    $0x10,%rsp
   0x00000000004011ae <+8>:     mov    %rdi,-0x8(%rbp)
   0x00000000004011b2 <+12>:    mov    $0x2b,%edi
   0x00000000004011b7 <+17>:    mov    $0x0,%eax
   0x00000000004011bc <+22>:    call   0x401070 <_ITM_beginTransaction@plt>
   0x00000000004011c1 <+27>:    and    $0x2,%eax
   0x00000000004011c4 <+30>:    test   %eax,%eax
   0x00000000004011c6 <+32>:    jne    0x4011ee <tm_increment+72>
   0x00000000004011c8 <+34>:    mov    -0x8(%rbp),%rax
   0x00000000004011cc <+38>:    mov    %rax,%rdi
   0x00000000004011cf <+41>:    call   0x4010b0 <_ITM_RU4@plt>
   0x00000000004011d4 <+46>:    add    $0x1,%eax
   0x00000000004011d7 <+49>:    mov    %eax,%edx
   0x00000000004011d9 <+51>:    mov    -0x8(%rbp),%rax
   0x00000000004011dd <+55>:    mov    %edx,%esi
   0x00000000004011df <+57>:    mov    %rax,%rdi
   0x00000000004011e2 <+60>:    call   0x401030 <_ITM_WU4@plt>
   0x00000000004011e7 <+65>:    call   0x401060 <_ITM_commitTransaction@plt>
   0x00000000004011ec <+70>:    jmp    0x401202 <tm_increment+92>
   0x00000000004011ee <+72>:    mov    -0x8(%rbp),%rax
   0x00000000004011f2 <+76>:    mov    (%rax),%eax
   0x00000000004011f4 <+78>:    lea    0x1(%rax),%edx
   0x00000000004011f7 <+81>:    mov    -0x8(%rbp),%rax
   0x00000000004011fb <+85>:    mov    %edx,(%rax)
   0x00000000004011fd <+87>:    call   0x401060 <_ITM_commitTransaction@plt>
   0x0000000000401202 <+92>:    nop
   0x0000000000401203 <+93>:    leave
   0x0000000000401204 <+94>:    ret
```

## 虽然 rtm=on ，但是为什么在 /proc/cpuinfo 中看不到 rtm 呀


```c
void __init tsx_init(void)
{
	char arg[5] = {};
	int ret;

	tsx_dev_mode_disable();

	/*
	 * Hardware will always abort a TSX transaction when the CPUID bit
	 * RTM_ALWAYS_ABORT is set. In this case, it is better not to enumerate
	 * CPUID.RTM and CPUID.HLE bits. Clear them here.
	 */
	if (boot_cpu_has(X86_FEATURE_RTM_ALWAYS_ABORT)) {
		tsx_ctrl_state = TSX_CTRL_RTM_ALWAYS_ABORT;
		tsx_clear_cpuid();
		setup_clear_cpu_cap(X86_FEATURE_RTM);
		setup_clear_cpu_cap(X86_FEATURE_HLE);
		return;
	}

	/*
	 * TSX is controlled via MSR_IA32_TSX_CTRL.  However, support for this
	 * MSR is enumerated by ARCH_CAP_TSX_MSR bit in MSR_IA32_ARCH_CAPABILITIES.
	 *
	 * TSX control (aka MSR_IA32_TSX_CTRL) is only available after a
	 * microcode update on CPUs that have their MSR_IA32_ARCH_CAPABILITIES
	 * bit MDS_NO=1. CPUs with MDS_NO=0 are not planned to get
	 * MSR_IA32_TSX_CTRL support even after a microcode update. Thus,
	 * tsx= cmdline requests will do nothing on CPUs without
	 * MSR_IA32_TSX_CTRL support.
	 */
	if (x86_read_arch_cap_msr() & ARCH_CAP_TSX_CTRL_MSR) {
		setup_force_cpu_cap(X86_FEATURE_MSR_TSX_CTRL); // <- 可以确定，是从这里走的，其实 cmdline 根本没用
	} else {
		tsx_ctrl_state = TSX_CTRL_NOT_SUPPORTED;
		return;
	}
```

```c
u64 my_x86_read_arch_cap_msr(void) {
  u64 ia32_cap = 0;

  if (boot_cpu_has(X86_FEATURE_ARCH_CAPABILITIES))
    rdmsrl(MSR_IA32_ARCH_CAPABILITIES, ia32_cap);

  return ia32_cap;
}

static int vermagic_init(void) {
  if (boot_cpu_has(X86_FEATURE_RTM_ALWAYS_ABORT)) {
    pr_info("[huxueshi:%s:%d] yes always abort \n", __FUNCTION__, __LINE__);
  } else {
    pr_info("[huxueshi:%s:%d] no always abort\n", __FUNCTION__, __LINE__);
    pr_info("[huxueshi:%s:%d] %llx\n", __FUNCTION__, __LINE__,
            my_x86_read_arch_cap_msr() & ARCH_CAP_TSX_CTRL_MSR);
  }
  return 0;
}
```
```txt
[161090.159700] [huxueshi:vermagic_init:19] no always abort
[161090.159703] [huxueshi:vermagic_init:20] 0
```

无论如何，在我的机器上，这个东西是存在 bug 的。

看看是否能够复现 TAA 的这个 bug 吧

## 同时硬件上也是看不到的

```txt
      RTM: restricted transactional memory     = false
```

## 基于 rtm 的功能: https://github.com/oneapi-src/oneTBB
