## fpu 引入的 context switch
<!-- b692015a-57f6-41f4-8849-90b30c377f82 -->

### kvm 中 guest 和 host 的切换

kvm_vcpu_arch::guest_fpu 被加载到 vmcs 中的时机

就奢侈在
```c
/* Swap (qemu) user FPU context for the guest FPU context. */
static void kvm_load_guest_fpu(struct kvm_vcpu *vcpu)
{
	/* Exclude PKRU, it's restored separately immediately after VM-Exit. */
	fpu_swap_kvm_fpstate(&vcpu->arch.guest_fpu, true);
	trace_kvm_fpu(1);
}

/* When vcpu_run ends, restore user space FPU context. */
static void kvm_put_guest_fpu(struct kvm_vcpu *vcpu)
{
	fpu_swap_kvm_fpstate(&vcpu->arch.guest_fpu, false);
	++vcpu->stat.fpu_reload;
	trace_kvm_fpu(0);
}
```

kvm_arch_vcpu_ioctl_run 执行的时候，会无条件的执行 kvm_load_guest_fpu

这非常的合理，大多数情况下，fpu 都是一个 thread 一份，但是 qemu 的 vcpu thread 需要有两份:
1. kvm_arch_vcpu_ioctl_run 就是 qemu 上下文切换到 guest os 的过程
2. 虚拟机 kvm_arch_vcpu_ioctl_run 中，可能切换去做其他的事情，那种情况的 fpu 切换走通用基础即可

### signal 导致的切换
- http://lastweek.io/notes/linux/linux-x86-fpu/

> Kernel needs to setup a `sigframe` for user level signal handlers. `sigframe` is a
> contiguous stack memory consists of the general purpose registers AND FPU
> registers. So signal handling part has to call back to FPU code to setup and
> copy the FPU registers to the in stack `sigframe`. Signal handling is another beast.


### process 之间的切换

tracepoint 的统计
```txt
  tracepoint:x86_fpu:x86_fpu_xstate_check_failed
  tracepoint:x86_fpu:x86_fpu_regs_deactivated
  tracepoint:x86_fpu:x86_fpu_regs_activated
  tracepoint:x86_fpu:x86_fpu_before_restore
  tracepoint:x86_fpu:x86_fpu_after_restore
  tracepoint:x86_fpu:x86_fpu_before_save
  tracepoint:x86_fpu:x86_fpu_init_state
  tracepoint:x86_fpu:x86_fpu_after_save
  tracepoint:x86_fpu:x86_fpu_copy_src
  tracepoint:x86_fpu:x86_fpu_copy_dst
  tracepoint:x86_fpu:x86_fpu_dropped
```

简单看看统计的东西
```txt
0     x86_fpu:x86_fpu_xstate_check_failed
3K    x86_fpu:x86_fpu_copy_dst
3K    x86_fpu:x86_fpu_copy_src
3K    x86_fpu:x86_fpu_dropped
0     x86_fpu:x86_fpu_init_state
544K  x86_fpu:x86_fpu_regs_deactivated
204K  x86_fpu:x86_fpu_regs_activated
0     x86_fpu:x86_fpu_after_restore
0     x86_fpu:x86_fpu_before_restore
0     x86_fpu:x86_fpu_after_save
0     x86_fpu:x86_fpu_before_save
```

```txt
@[
        fpu_clone+587
        copy_thread+328
        copy_process+3087
        kernel_clone+189
        __do_sys_clone+102
        do_syscall_64+127
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

```txt
+ sudo bpftrace -e 'tracepoint:x86_fpu:x86_fpu_regs_deactivated { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
Attaching 2 probes...
^C

@[
        __switch_to+904
]: 1637
```

```txt
sudo bpftrace -e 'tracepoint:x86_fpu:x86_fpu_regs_activated { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
```

```txt
@[
        switch_fpu_return+157
        switch_fpu_return+157
        vcpu_enter_guest.constprop.0+2514
        vcpu_run+47
        kvm_arch_vcpu_ioctl_run+667
        kvm_vcpu_ioctl+277
        __x64_sys_ioctl+148
        do_syscall_64+127
        entry_SYSCALL_64_after_hwframe+118
]: 38
@[
        switch_fpu_return+157
        switch_fpu_return+157
        syscall_exit_to_user_mode+469
        do_syscall_64+140
        entry_SYSCALL_64_after_hwframe+118
]: 231
```

```txt
@[
        __switch_to_xtra+5
        __switch_to+516
        __schedule+696
        schedule_idle+35
        do_idle+163
        cpu_startup_entry+41
        start_secondary+301
        common_startup_64+318
]: 564
```

观察了下，基本上所有的程序都是会走到 tracepoint:x86_fpu:x86_fpu_regs_activated


## arch/x86/kernel/fpu/xstate.c

> [!NOTE]
> 参考 Deepseeek ，有待验证

现代 x86 CPU 除了传统的通用寄存器（如 RAX、RBX）外，还包含大量扩展寄存器用于 SIMD（单指令多数据）计算：
- SSE（128 位）
- AVX（256 位）
- AVX-512（512 位）
- MPX（内存保护扩展）
- PKRU（保护密钥）
- AMX（高级矩阵扩展，用于 AI 加速）
- 这些寄存器统称为 "Extended State"。早期 Linux 仅保存/恢复 x87 FPU 和 SSE 状态，但随着指令集扩展，需要一种可扩展的机制来动态管理这些状态。

Intel 引入了 XSAVE/XRSTOR 指令集（以及 XSAVEOPT、XSAVES 等变体），允许 CPU 按需保存/恢复指定的扩展状

xstate.c 的核心功能

1. 初始化 XSAVE 支持
在系统启动时探测 CPU 支持哪些扩展状态组件（通过 CPUID 指令）
构建 XSAVE 特性掩码（xfeatures_mask）
分配 per-task 的 FPU state 缓冲区（大小根据启用的扩展状态动态调整）

2. 管理 XSAVE 区域布局
定义 `struct xregs_state`（对应硬件 XSAVE 区域）
支持 compacted format（节省空间，仅保存非零组件）

3. 提供保存/恢复接口
xsaves() / xrstors()：封装 XSAVE/XRSTOR 指令

支持动态启用/禁用扩展状态
例如：当系统检测到 AVX-512 可能导致降频（thermal throttling），可通过 clearcpuid=... 禁用，xstate.c 会相应调整保存的组件。

4. 支持动态启用/禁用扩展状态
例如：当系统检测到 AVX-512 可能导致降频（thermal throttling），可通过 clearcpuid=... 禁用，xstate.c 会相应调整保存的组件。


## 核心结构体

具体参考: https://docs.google.com/document/d/1ZW8-uoUHDLQGSAAiACEt8MZQg804K-_kjREN5_1wGVw/edit?tab=t.0

### enum xfeature
```c
/*
 * List of XSAVE features Linux knows about:
 */
enum xfeature {
	XFEATURE_FP,
	XFEATURE_SSE,
	/*
	 * Values above here are "legacy states".
	 * Those below are "extended states".
	 */
	XFEATURE_YMM,
	XFEATURE_BNDREGS,
	XFEATURE_BNDCSR,
	XFEATURE_OPMASK,
	XFEATURE_ZMM_Hi256,
	XFEATURE_Hi16_ZMM,
	XFEATURE_PT_UNIMPLEMENTED_SO_FAR,
	XFEATURE_PKRU,
	XFEATURE_PASID,
	XFEATURE_CET_USER,
	XFEATURE_CET_KERNEL,
	XFEATURE_RSRVD_COMP_13,
	XFEATURE_RSRVD_COMP_14,
	XFEATURE_LBR,
	XFEATURE_RSRVD_COMP_16,
	XFEATURE_XTILE_CFG,
	XFEATURE_XTILE_DATA,
	XFEATURE_APX,

	XFEATURE_MAX,
};
```

### fpu
```c
/*
 * This is our most modern FPU state format, as saved by the XSAVE
 * and restored by the XRSTOR instructions.
 *
 * It consists of a legacy fxregs portion, an xstate header and
 * subsequent areas as defined by the xstate header.  Not all CPUs
 * support all the extensions, so the size of the extended area
 * can vary quite a bit between CPUs.
 */
struct xregs_state {
	struct fxregs_state		i387;
	struct xstate_header		header;
	u8				extended_state_area[];
} __attribute__ ((packed, aligned (64)));

/*
 * This is a union of all the possible FPU state formats
 * put together, so that we can pick the right one runtime.
 *
 * The size of the structure is determined by the largest
 * member - which is the xsave area.  The padding is there
 * to ensure that statically-allocated task_structs (just
 * the init_task today) have enough space.
 */
union fpregs_state {
	struct fregs_state		fsave;
	struct fxregs_state		fxsave;
	struct swregs_state		soft;
	struct xregs_state		xsave;
	u8 __padding[PAGE_SIZE];
};

struct fpstate {
	/* @kernel_size: The size of the kernel register image */
	unsigned int		size;

	/* @user_size: The size in non-compacted UABI format */
	unsigned int		user_size;

	/* @xfeatures:		xfeatures for which the storage is sized */
	u64			xfeatures;

	/* @user_xfeatures:	xfeatures valid in UABI buffers */
	u64			user_xfeatures;

	/* @xfd:		xfeatures disabled to trap userspace use. */
	u64			xfd;

	/* @is_valloc:		Indicator for dynamically allocated state */
	unsigned int		is_valloc	: 1;

	/* @is_guest:		Indicator for guest state (KVM) */
	unsigned int		is_guest	: 1;

	/*
	 * @is_confidential:	Indicator for KVM confidential mode.
	 *			The FPU registers are restored by the
	 *			vmentry firmware from encrypted guest
	 *			memory. On vmexit the FPU registers are
	 *			saved by firmware to encrypted guest memory
	 *			and the registers are scrubbed before
	 *			returning to the host. So there is no
	 *			content which is worth saving and restoring.
	 *			The fpstate has to be there so that
	 *			preemption and softirq FPU usage works
	 *			without special casing.
	 */
	unsigned int		is_confidential	: 1;

	/* @in_use:		State is in use */
	unsigned int		in_use		: 1;

	/* @regs: The register state union for all supported formats */
	union fpregs_state	regs;

	/* @regs is dynamically sized! Don't add anything after @regs! */
} __aligned(64);
```

> [!NOTE]
> 参考 Deepseeek ，有待验证

| 字段            | 作用                                                                                                                                                                                                                                         |
|-----------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| size            | 内核内部使用的寄存器状态缓冲区大小（单位：字节）。根据 CPU 支持的扩展特性（如 AVX-512）动态计算，可能使用 compacted 格式以节省空间。                                                                                                         |
| user_size       | 用户空间 ABI（UABI）要求的非 compacted 格式大小。用于信号处理、ptrace、get/put_fpstate() 等需要与用户空间交换 FPU 状态的场景。                                                                                                               |
| xfeatures       | 当前 `fpstate` 缓冲区实际支持的扩展特性位掩码"（如 bit 1=SSE, bit 2=AVX, bit 5=AVX-512 等）。决定哪些寄存器组件被包含在 "regs 中。                                                                                                           |
| user_xfeatures  | 用户空间 ABI 中有效的扩展特性掩码。可能与 xfeatures 不同（例如内核禁用了某些特性对用户可见）。                                                                                                                                               |
| xfd             | eXception Feature Disable (XFD)：用于 按需启用 AVX-512/AMX 等高功耗指令集。当用户程序首次使用被禁用的特性时触发 #NM 异常，内核动态扩展 fpstate 并启用该特性。                                                                                |
| is_valloc       | 标志位：表示 regs 是 动态分配的（vmalloc），而非嵌入在 task_struct 中的静态缓冲区（适用于大型状态如 AVX-512）。                                                                                                                              |
| is_guest        | 标志位：表示这是 KVM 虚拟机 guest 的 FPU 状态，由 KVM 管理。                                                                                                                                                                                 |
| is_confidential | 标志位：用于 KVM confidential computing（如 SEV-SNP、TDX）。此时 FPU 状态由硬件/固件在 VM entry/exit 时自动保存/恢复到加密内存，内核无需保存真实内容，但保留结构体以支持抢占和软中断中的 FPU 使用。                                          |
| in_use          | 标志位：表示该 fpstate 当前 正在被某个任务使用（用于 lazy FPU 优化）。                                                                                                                                                                       |
| regs            | 联合体（union），包含所有可能的寄存器状态格式：<br> - struct fregs_state（x87 FPU）<br> - struct fxregs_state（FXSAVE 格式，含 SSE）<br> - struct xregs_state（XSAVE 格式，支持 AVX/AVX-512/AMX 等）<br> 实际使用哪个成员由 xfeatures 决定。 |
| __aligned(64)   | 强制 64 字节对齐，满足 XSAVE 指令对内存地址对齐的要求（通常需 64 字节对齐）。                                                                                                                                                                |

似乎是那么回事哦，不过有两个事情需要测试下下:
1. struct fpstate::is_guest ，也就是 kvm 是如何具体参与进来的
2. struct fpstate::in_use ，真的吗?

UABI 就是这个区域拷贝到 qemu 中，是一个固定的格式，qemu 可以解析这个区域。

## 各种 features 的对比
```txt
	pr_info("[+] %llx %llx %llx %llx %llx %llx %llx\n", vcpu->arch.xcr0,
		vcpu->arch.guest_supported_xcr0, kvm_caps.supported_xcr0,
		vcpu->arch.guest_fpu.xfeatures,
		vcpu->arch.guest_fpu.fpstate->user_xfeatures,
		vcpu->arch.guest_fpu.fpstate->xfeatures,
		vcpu->arch.guest_fpu.fpstate->regs.xsave.header.xfeatures);
    # [+] 在 kvm_vcpu_ioctl_x86_set_xsave 为 set martins3
    #     在 kvm_vcpu_ioctl_x86_set_xsave 为 xsetbv martins3
```

单个虚拟机启动而发过程中就是的
```txt
[49819.233299] kvm: [set martins3] 1 207 207 1a07 207 1a07 0
[49819.298492] kvm: [set martins3] 1 207 207 1a07 207 1a07 0
[49819.367051] kvm: [get martins3] 1 207 207 1a07 207 1a07 0
[49819.431806] kvm: [get martins3] 1 207 207 1a07 207 1a07 0
[49819.497662] kvm: [set martins3] 1 207 207 1a07 207 1a07 0
[49819.562248] kvm: [set martins3] 1 207 207 1a07 207 1a07 0
[49819.657331] kvm: [get martins3] 1 207 207 1a07 207 1a07 200
[49819.724241] kvm: [set martins3] 1 207 207 1a07 207 1a07 200
[49820.084040] kvm: [xsetbv martins3] 207 207 207 1a07 207 1a07 201
[49820.157865] kvm: [xsetbv martins3] 207 207 207 1a07 207 1a07 200
[49820.244325] kvm: [xsetbv martins3] 207 207 207 1a07 207 1a07 1a03
```
这里的 header 有时候 1a03 (其实是很奇怪的，既然不支持，为什么还是会保存?)

core2duo 启动
```txt
[52945.357600] kvm: [set martins3] 1 0 207 1a07 207 1a07 0
[52945.420962] kvm: [set martins3] 1 0 207 1a07 207 1a07 0
[52945.487210] kvm: [get martins3] 1 0 207 1a07 207 1a07 0
[52945.549967] kvm: [get martins3] 1 0 207 1a07 207 1a07 0
[52945.613467] kvm: [set martins3] 1 0 207 1a07 207 1a07 0
[52945.676053] kvm: [set martins3] 1 0 207 1a07 207 1a07 0
[52945.766681] kvm: [get martins3] 1 0 207 1a07 207 1a07 200
[52945.831554] kvm: [set martins3] 1 0 207 1a07 207 1a07 200
```
注意，这里的 user_xfeatures 是 207 而不是 0 ，也就是 user_xfeatures
本来就是不会在于

### vcpu->arch.guest_fpu.fpstate->regs.xsave.header.xfeatures

在 kvm 表现为 vcpu->arch.guest_fpu.fpstate->regs.xsave.header.xfeatures

fpu 的 lazy 保存是有关系的，只有 dirty 的时候才有必要，
保存这些东西，不然是没必要的，所以其中的取值是变化的。

```txt
  50.20%  202
  37.94%  201
   9.40%  200
   2.17%  203
   0.19%  206
   0.07%  207
   0.03%  1a02
   0.01%  1a03
```

运行这个可以导致 exit 的结果不同
```txt
stress-ng --cpu 1 --cpu-method int128decimal128 --timeout 10s
```

添加一个 include/trace/events/kvm.h 中的就可以了:
```c
static void kvm_load_guest_fpu(struct kvm_vcpu *vcpu)
{
	/* Exclude PKRU, it's restored separately immediately after VM-Exit. */
	fpu_swap_kvm_fpstate(&vcpu->arch.guest_fpu, true);
	trace_kvm_fpu(1);
	trace_kvm_fpu_reg(vcpu->arch.guest_fpu.fpstate->regs.xsave.header.xfeatures);
}

TRACE_EVENT(kvm_fpu_reg,
	TP_PROTO(u64 load),
	TP_ARGS(load),

	TP_STRUCT__entry(
		__field(	u64,	        load		)
	),

	TP_fast_assign(
		__entry->load		= load;
	),

	TP_printk("%llx", __entry->load)
);
```

### fpstate->user_xfeatures

```txt
@[
        restore_fpregs_from_user+1
        __fpu_restore_sig+175
        fpu__restore_sig+121
        restore_sigcontext+352
        __x64_sys_rt_sigreturn+208
        do_syscall_64+127
        entry_SYSCALL_64_after_hwframe+118
]: 28

+ sudo bpftrace -e 'kprobe:restore_fpregs_from_user { @[curtask->comm] = count() } interval:s:1000 { exit(); }'
Attaching 2 probes...

@[bpftrace]: 1
@[zsh]: 9
```

所以，fpstate::user_xfeatures 和 fpstate::xfeature 的关系就是，

user_xfeatures 对于用户可见的，而 fpstate::xfeature 是提供给 xsave 和 xstore 用的

### xfeatures
两个 xfeatures 的含义:
vcpu->arch.guest_fpu.xfeatures,
vcpu->arch.guest_fpu.fpstate->xfeatures

取值 1a07 是含义是 xsave area 的支持最大范围:

额外添加了 11 和 12 位，其含义是 shadow stack 相关:
https://en.wikipedia.org/wiki/Control_register#XCR0_and_XSS
https://docs.kernel.org/arch/x86/shstk.html


0001-x86-kvm-fpu-Limit-guest-user_xfeatures-to-supported-.patch
> The mask for xsave(s), the guest's fpstate->xfeatures, is defined on
> kvm_arch_vcpu_create(), which (in summary) sets it to all features
> supported by the cpu which are enabled on kernel config.

fpu_alloc_guest_fpstate() 有对于其唯一的赋值:
```txt
	gfpu->xfeatures		= guest_default_cfg.features;
```

感觉还是和 fpstate 在一起的通用概念吧，
实际上和虚拟化的角度没有关系的。

### vcpu->arch.guest_supported_xcr0
<!-- a52f4bf0-003e-4de7-ae7c-9bdf4ce41f79 -->

通过 XSETBV 来理解 kvm_vcpu_arch::guest_supported_xcr0 和 kvm_vcpu_arch::xcr0

https://en.wikipedia.org/wiki/Control_register#XCR0_and_XSS
这里的表格记录了很多东西。


xcr0 ，类似 cr0 ，来控制 fpu 相关的寄存器，利用 XSETBV 和 XGETBV 来访问
控制是否可以使用 XSAVE/XRSTOR 指令。 xcr0 中保存的都是一些黑魔法。

```c
static int (*kvm_vmx_exit_handlers[])(struct kvm_vcpu *vcpu) = {
  // ...
	[EXIT_REASON_XSETBV]                  = kvm_emulate_xsetbv,
```

```c
static int (*const svm_exit_handlers[])(struct kvm_vcpu *vcpu) = {
  // ...
	[SVM_EXIT_XSETBV]			= kvm_emulate_xsetbv,
```
vmx 和 kvm 两个的处理函数都是一样，就是用来设置 kvm_vcpu_arch::xcr0 的
在其中会使用 guest_supported_xcr0 会来做检查

而 vcpu->arch.guest_supported_xcr0 的来源为:
```sh
	vcpu->arch.guest_supported_xcr0 = cpuid_get_supported_xcr0(vcpu);
```

```c
/*
 * Calculate guest's supported XCR0 taking into account guest CPUID data and
 * KVM's supported XCR0 (comprised of host's XCR0 and KVM_SUPPORTED_XCR0).
 */
static u64 cpuid_get_supported_xcr0(struct kvm_vcpu *vcpu)
{
	struct kvm_cpuid_entry2 *best;

	best = kvm_find_cpuid_entry_index(vcpu, 0xd, 0);
	if (!best)
		return 0;

	return (best->eax | ((u64)best->edx << 32)) & kvm_caps.supported_xcr0;
}
```

### xcr0

- xcr0

表示客户机当前实际设置的 XCR0 值。
即客户机通过执行 `XSETBV` 指令写入 XCR0 寄存器的值（在虚拟化下被 KVM 拦截并模拟）。

- guest_supported_xcr0

表示客户机被允许使用的 XCR0 功能位掩码（capability mask）。
它由宿主机 CPU 能力、KVM 配置、QEMU 设置共同决定，是客户机可以合法启用的扩展寄存器组件的“上限”。

那么 xcr0 和 guest_supported_xcr0 什么时候不想等?
很容易，guest_supported_xcr0() 是 kvm 提供的，如果 guest os 不去使用 XSETBV 来配置
fpu 相关功能，那么就可以观察到 xcr0 的数值为 0

### vcpu->arch.guest_fpu.fpstate->user_xfeatures
一般就是 kvm_caps.supported_xcr0 ，不受 cpuid 的影响

## doc

### how-debuggers-work-getting-and-setting-x86-registers-part-2
https://www.moritz.systems/blog/how-debuggers-work-getting-and-setting-x86-registers-part-2

现在 xsave 的结构和图中说的一样的:
```c
/*
 * This is our most modern FPU state format, as saved by the XSAVE
 * and restored by the XRSTOR instructions.
 *
 * It consists of a legacy fxregs portion, an xstate header and
 * subsequent areas as defined by the xstate header.  Not all CPUs
 * support all the extensions, so the size of the extended area
 * can vary quite a bit between CPUs.
 */
struct xregs_state {
	struct fxregs_state		i387;
	struct xstate_header		header;
	u8				extended_state_area[];
} __attribute__ ((packed, aligned (64)));
```

### 其他
- Intel® AVX-512 - Packet Processing with Intel® AVX-512 Instruction Set
https://builders.intel.com/docs/networkbuilders/intel-avx-512-packet-processing-with-intel-avx-512-instruction-set-solution-brief-1678190247.pdf

- https://stackoverflow.com/questions/51805127/how-to-test-avx-512-instructions-w-o-supported-hardware

- https://gab-menezes.github.io/2025/01/13/using-the-most-unhinged-avx-512-instruction-to-make-the-fastest-phrase-search-algo.html#results

### Documentation/arch/x86/xstate.rst
https://www.kernel.org/doc/html/latest/arch/x86/xstate.html

## xfd

> [!NOTE]
> 参考 Deepseeek ，有待验证

Intel 引入了 XFD（eXtended Feature Disable） 机制，作为对 XSAVE/XRSTOR 指令集扩展的一部分，用于更精细地控制高级 CPU 特性（尤其是大型向量指令集，如 AMX 或 AVX-512）的启用状态。

当某个扩展特性（如 Intel AMX）未被使用时，通过 XFD 可以将其上下文状态设为“懒惰保存”（lazy）甚至完全禁用。
如果程序尝试使用被 XFD 禁用的特性，会触发一个 #NM（Device Not Available）异常，此时操作系统可以介入，动态启用该特性并恢复执行。

## 问题
### fpstate_realloc 根本就是没有调用的

### 如果 simd 的上下文开销太高， 将功能都放到 GPU 中，是不是可以减轻 os 的压力?

###  函数 xsaves() 和 os_xsave() 的区别是什么?
不过，为什么 xsaves() 只有 lbr() 调用

似乎真的用于上下文切换的函数是 os_xsave() 和 os_xrstor()

### avx2 在
当 qemu 配置虚拟机的 cpu model 为 -cpu SandyBridge ，虚拟机中执行 avx2 指令，并不会异常，但是虚拟机执行 cpuid ，显示 avx2 指令不支持。如果将 cpu model 调整为 core2duo f，虚拟机中执行 含有 avx2 指令的程序，该程序会 crash 。

似乎实际上，关心的是 vcpu->arch.xcr0 而非 cpuid 的结果


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
