## 先搞搞最基本的问题吧

- 如果不 idle，是不是可以完全没有 kvm_exit，还是会因为 host 的 timer 退出?

- 时钟中断可以直接注入到 guest 中间吗? 还是需要让 guest 推出一下

  - 我猜测这个是可以控制的
  - idle 的时候退出我猜测是因为 halt 指令模拟
  - 测试下，当如果打开 guest NO_HZ 和关闭 NO_HZ 的时候，exit 的差别

- 本来 guest 因为 halt 退出到 host，如果 guest 忽然变忙了，host 如何知道?

  - host 必然会接受到应该注入到 guest 的中断。
  - 只要 kernel_args+=" nohz_full=0-7" 会导致 8 core 的 exit 为 2000/s，去掉之后，exit 的数量 200/s ，有趣啊

- 为什么 kvm_task_switch 从来不会有人调用
  - 我还以为只要 kvm 出现 process switch 的时候，就会发生调用的啊


## 看看其中关于 kvm 模块参数的说明

/home/martins3/core/linux/Documentation/admin-guide/kernel-parameters.txt

## KVM_GET_STATS_FD 是做什么的?

在 qemu 中:

```c
    if (kvm_check_extension(kvm_state, KVM_CAP_BINARY_STATS_FD)) {
        add_stats_callbacks(STATS_PROVIDER_KVM, query_stats_cb,
                            query_stats_schemas_cb);
    }
```

其具体实现:

Documentation/virt/kvm/api.rst


## kvm_handle_wfx

## 难道 kvm_x2apic_msr_read 现在才是默认的状态吗?

完全可以用来测试下性能差别

## [ ] 嵌套虚拟化如何处理中断的


## 在进行对于 vcpu 的 ioctl 的时候，vcpu 是继续运行的状态吗?

```txt
@[
    kvm_vcpu_write_tsc_offset+1
    __kvm_synchronize_tsc+78
    kvm_synchronize_tsc+232
    kvm_set_msr_common+2202
    vmx_set_msr+1262
    __kvm_set_msr+145
    kvm_arch_vcpu_ioctl+3121
    kvm_vcpu_ioctl+896
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 8
```
我倾向于认为 vcpu 执行不受 vcpu ioctl 影响。
因为最后还是修改 vmcs 之类的，然后让 vcpu 下次进入的时候使用全新的
内容。


看这个注释，的确是异步的
```c
static void kvm_vcpu_write_tsc_multiplier(struct kvm_vcpu *vcpu, u64 l1_multiplier)
{
	vcpu->arch.l1_tsc_scaling_ratio = l1_multiplier;

	/* Userspace is changing the multiplier while L2 is active */
	if (is_guest_mode(vcpu))
		vcpu->arch.tsc_scaling_ratio = kvm_calc_nested_tsc_multiplier(
			l1_multiplier,
			static_call(kvm_x86_get_l2_tsc_multiplier)(vcpu));
	else
		vcpu->arch.tsc_scaling_ratio = l1_multiplier;

	if (kvm_caps.has_tsc_control)
		static_call(kvm_x86_write_tsc_multiplier)(vcpu);
}
```

## 为什么到来 2024 年，arm kvm 还是没有办法 insmod !


## 想了下，还是把 sdm 好好读读吧，至少其他的问题会少很多

到时候在切入到 memory 之后，问题会简单很多吧!

## 似乎 arm 的 cpu feature 也是有的
https://qemu-project.gitlab.io/qemu/system/arm/cpu-features.html


## 如果 arm 环境中配置 pci=nomsi ，观察下整个工作
中断的整个流程是什么样子的
1. cat /proc/interrupts
2. cat /sys/kernel/debug/irq/irqs/0
4. qemu 会 kvm_vm_ioctl_irq_line 吗?

## 为什么 13900k 中没有这个呀

```c
static inline bool cpu_has_secondary_exec_ctrls(void)
{
	return vmcs_config.cpu_based_exec_ctrl &
		CPU_BASED_ACTIVATE_SECONDARY_CONTROLS;
}
```

## 应该看看一个 vmcs 长成什么样子的?

```txt
[ 2311.499051] kvm_intel: VMCS 00000000336523a9, last attempted VM-entry on CPU 8
[ 2311.499052] kvm_intel: *** Guest State ***
[ 2311.499052] kvm_intel: CR0: actual=0x0000000080050033, shadow=0x0000000080050033, gh_mask=fffffffffffefff7
[ 2311.499053] kvm_intel: CR4: actual=0x0000000000752ef0, shadow=0x0000000000750ef0, gh_mask=fffffffffffef871
[ 2311.499053] kvm_intel: CR3 = 0x0000000004442000
[ 2311.499054] kvm_intel: PDPTR0 = 0x000000005f18a001  PDPTR1 = 0x000000005f189001
[ 2311.499054] kvm_intel: PDPTR2 = 0x000000005f188001  PDPTR3 = 0x000000005f187001
[ 2311.499055] kvm_intel: RSP = 0xffffc90000017418  RIP = 0xffffffff828e3a0c
[ 2311.499055] kvm_intel: RFLAGS=0x00000286         DR7 = 0x000000000000060a
[ 2311.499056] kvm_intel: Sysenter RSP=fffffe0000003000 CS:RIP=0010:ffffffff82a01e60
[ 2311.499056] kvm_intel: CS:   sel=0x0010, attr=0x0a09b, limit=0xffffffff, base=0x0000000000000000
[ 2311.499057] kvm_intel: DS:   sel=0x0000, attr=0x1c001, limit=0xffffffff, base=0x0000000000000000
[ 2311.499057] kvm_intel: SS:   sel=0x0000, attr=0x1c000, limit=0xffffffff, base=0x0000000000000000
[ 2311.499058] kvm_intel: ES:   sel=0x0000, attr=0x1c001, limit=0xffffffff, base=0x0000000000000000
[ 2311.499058] kvm_intel: FS:   sel=0x0000, attr=0x1c000, limit=0xffffffff, base=0x0000000000000000
[ 2311.499059] kvm_intel: GS:   sel=0x0000, attr=0x1c001, limit=0xffffffff, base=0xffff888236e00000
[ 2311.499059] kvm_intel: GDTR:                           limit=0x0000007f, base=0xfffffe0000001000
[ 2311.499060] kvm_intel: LDTR: sel=0x0000, attr=0x1c000, limit=0xffffffff, base=0x0000000000000000
[ 2311.499060] kvm_intel: IDTR:                           limit=0x00000fff, base=0xfffffe0000000000
[ 2311.499061] kvm_intel: TR:   sel=0x0040, attr=0x0008b, limit=0x00004087, base=0xfffffe0000003000
[ 2311.499061] kvm_intel: EFER= 0x0000000000000d01 (effective)
[ 2311.499062] kvm_intel: PAT = 0x0407050600070106
[ 2311.499062] kvm_intel: DebugCtl = 0x0000000000000000  DebugExceptions = 0x0000000000000000
[ 2311.499063] kvm_intel: Interruptibility = 00000000  ActivityState = 00000000
[ 2311.499063] kvm_intel: InterruptStatus = ec00
[ 2311.499064] kvm_intel: *** Host State ***
[ 2311.499064] kvm_intel: RIP = 0xffffffffc11accb0  RSP = 0xffffacd087de7d28
[ 2311.499064] kvm_intel: CS=0010 SS=0018 DS=0000 ES=0000 FS=0000 GS=0000 TR=0040
[ 2311.499065] kvm_intel: FSBase=00007f5088e006c0 GSBase=ffff90ecbec00000 TRBase=fffffe4d122bd000
[ 2311.499066] kvm_intel: GDTBase=fffffe4d122bb000 IDTBase=fffffe0000000000
[ 2311.499066] kvm_intel: CR0=0000000080050033 CR3=000000035dde6000 CR4=0000000000f52ef0
[ 2311.499066] kvm_intel: Sysenter RSP=fffffe4d122bd000 CS:RIP=0010:ffffffffa7e02020
[ 2311.499067] kvm_intel: PAT = 0x0407050600070106
[ 2311.499067] kvm_intel: *** Control State ***
[ 2311.499068] kvm_intel: CPUBased=0xb5a26dfa SecondaryExec=0x061017fa TertiaryExec=0x0000000000000000
[ 2311.499068] kvm_intel: PinBased=0x000000ff EntryControls=000053ff ExitControls=000befff
[ 2311.499069] kvm_intel: ExceptionBitmap=00060042 PFECmask=00000000 PFECmatch=00000000
[ 2311.499069] kvm_intel: VMEntry: intr_info=000000ec errcode=00000000 ilen=00000002
[ 2311.499070] kvm_intel: VMExit: intr_info=00000000 errcode=00000000 ilen=00000003
[ 2311.499070] kvm_intel:         reason=00000030 qualification=0000000000000981
[ 2311.499071] kvm_intel: IDTVectoring: info=800000ec errcode=00000000
[ 2311.499071] kvm_intel: TSC Offset = 0xfffff9be78c9dd49
[ 2311.499072] kvm_intel: TSC Multiplier = 0x0001000000000000
[ 2311.499072] kvm_intel: SVI|RVI = ec|00 TPR Threshold = 0x00
[ 2311.499073] virt-APIC addr = 0x0000000360524000
[ 2311.499073] kvm_intel: PostedIntrVec = 0xf2
[ 2311.499073] kvm_intel: EPT pointer = 0x0000000299e9305e
[ 2311.499074] kvm_intel: PLE Gap=00000080 Window=00001000
[ 2311.499074] kvm_intel: Virtual processor ID = 0x0001
```
- [ ] 如何理解 Sysenter 的


## 有趣的问题，也许可以看看
https://web.njit.edu/~dingxn/papers/TPDS21.pdf


## CONFIG_KVM_PRIVATE_MEM
virt/kvm/guest_memfd.c 中:
```txt
static struct file_operations kvm_gmem_fops = {
	.open		= generic_file_open,
	.release	= kvm_gmem_release,
	.fallocate	= kvm_gmem_fallocate,
};
```
看看这个是如何实现的吧。

## x86-64 的环境中，可以使用 kvm 运行 32 位内核吗

虽然没有什么意义，但是似乎 kvm 为了解决这个问题，把
kvm 变的非常复杂。

## 忽然意识到，嵌套虚拟化中，这个 A/D bit 也是会被硬件自动设置的吗
https://patchwork.kernel.org/project/kvm/patch/1490867732-16743-5-git-send-email-pbonzini@redhat.com/

## 嵌套虚拟化真的很 nb ，在 L1 中可以继续使用 qemu 来调试 kernel 的

## 嵌套虚拟化中，如何实现的，当 kill L1 的时候，L2 自动被杀掉，如何实现的?

具体的钩子函数在哪里?

## 给 L1 中使用 32 个 core ，但是在 L2 中使用 64 个 core

### L2 中使用多少个 core ，可以在 L1 直接观测到吗？


## 我相信，如果在嵌套中使用 vmware workstation ，一定问题很多吧

## 容器有可信计算这个说法吗?
### 那些使用 k8s + 容器的用户，cpu 的扩缩容是如何实现的

## 我的印象中，如果 qemu 始终拿不到 CPU ，guest os 会 softlock 的

这是 guest os kernel 的 bug 吗?
其实也不算

## 仔细分析下 kernel-irqchip=on 的影响

## kvm kvm-intel 以及 kvm-amd 的 所有的命令行参数

## 和 kernel 下的是重复的吗?
https://github.com/kvm-x86/kvm-unit-tests?tab=readme-ov-file

## 回答这个问题
https://stackoverflow.com/questions/35698919/how-to-view-nested-page-table-entries-in-qemu


## 这个有什么关系吗?
```c
/*
 * Check for any event (interrupt or exception) that is ready to be injected,
 * and if there is at least one event, inject the event with the highest
 * priority.  This handles both "pending" events, i.e. events that have never
 * been injected into the guest, and "injected" events, i.e. events that were
 * injected as part of a previous VM-Enter, but weren't successfully delivered
 * and need to be re-injected.
 *
 * Note, this is not guaranteed to be invoked on a guest instruction boundary,
 * i.e. doesn't guarantee that there's an event window in the guest.  KVM must
 * be able to inject exceptions in the "middle" of an instruction, and so must
 * also be able to re-inject NMIs and IRQs in the middle of an instruction.
 * I.e. for exceptions and re-injected events, NOT invoking this on instruction
 * boundaries is necessary and correct.
 *
 * For simplicity, KVM uses a single path to inject all events (except events
 * that are injected directly from L1 to L2) and doesn't explicitly track
 * instruction boundaries for asynchronous events.  However, because VM-Exits
 * that can occur during instruction execution typically result in KVM skipping
 * the instruction or injecting an exception, e.g. instruction and exception
 * intercepts, and because pending exceptions have higher priority than pending
 * interrupts, KVM still honors instruction boundaries in most scenarios.
 *
 * But, if a VM-Exit occurs during instruction execution, and KVM does NOT skip
 * the instruction or inject an exception, then KVM can incorrecty inject a new
 * asynchronous event if the event became pending after the CPU fetched the
 * instruction (in the guest).  E.g. if a page fault (#PF, #NPF, EPT violation)
 * occurs and is resolved by KVM, a coincident NMI, SMI, IRQ, etc... can be
 * injected on the restarted instruction instead of being deferred until the
 * instruction completes.
 *
 * In practice, this virtualization hole is unlikely to be observed by the
 * guest, and even less likely to cause functional problems.  To detect the
 * hole, the guest would have to trigger an event on a side effect of an early
 * phase of instruction execution, e.g. on the instruction fetch from memory.
 * And for it to be a functional problem, the guest would need to depend on the
 * ordering between that side effect, the instruction completing, _and_ the
 * delivery of the asynchronous event.
 */
static int kvm_check_and_inject_events(struct kvm_vcpu *vcpu,
				       bool *req_immediate_exit)
```



## 有什么好办法获取 kvm_memslots_have_rmaps 的结果
类似的 kvm 结构体中各种东西如何导出

crash ?

bpftrace 的 kfunc 吗?


## apicv 为什么需要一个额外的 page 来模拟

## 综合的 TODO
./preemption_timer.md
./tlb.md
./fpu.md
./exit-reason.md
./features/pv-sched-yield.md
./docs/kvm/halt-polling.md : 其实如果可以不退出，那不就是不需要 halt-polling 吗?
./dirty.md
./cr.md
./ple.md
./interrupt-windowd.md : apicv 和 windows 的

## 这个是做什么的?
arch/x86/kvm/vmx/posted_intr.c

## flexpriority 是什么东西?

## dm 是什么意思 ?

具体含义未知，不过应该是来自于 userspace 的:

kvm_cpu_accept_dm_intr
dm_request_for_irq_injection

request_irq_exits 这个字段也是和这个有关系的

看上去是用户的 irq windows 而已

## 这里好几个参数

windows 启动：
```txt
[root@bogon kvm]# cat io_exits
246771
[root@bogon kvm]# cat irq_exits
1482148
[root@bogon kvm]# cat irq_injections
2260
[root@bogon kvm]# cat irq_window_exits
31
```

linux 启动:
```txt
[root@bogon kvm]# cat io_exits
141644
[root@bogon kvm]# cat irq_exits
11050
[root@bogon kvm]# cat irq_injections
35
[root@bogon kvm]# cat irq_window_exits
11
```
apicv 打开后，irq_injections 的注入其实都不高

## 如何理解 kvm_lapic_enabled

```c
	if (kvm_check_request(KVM_REQ_EVENT, vcpu) || req_int_win ||
	    kvm_xen_has_interrupt(vcpu)) {
		++vcpu->stat.req_event;
		r = kvm_apic_accept_events(vcpu);
		if (r < 0) {
			r = 0;
			goto out;
		}
		if (vcpu->arch.mp_state == KVM_MP_STATE_INIT_RECEIVED) {
			r = 1;
			goto out;
		}

		r = kvm_check_and_inject_events(vcpu, &req_immediate_exit);
		if (r < 0) {
			r = 0;
			goto out;
		}
		if (req_int_win)
			kvm_x86_call(enable_irq_window)(vcpu);

    // 为什么 kvm_lapic_enabled 了，才可以 kvm_lapic_sync_to_vapic ?
		if (kvm_lapic_enabled(vcpu)) {
			update_cr8_intercept(vcpu);
			kvm_lapic_sync_to_vapic(vcpu);
		}
	}
```

## 当使用 apicv 的 posted interrupt 之后，如果那个 vCPU 正好屏蔽中断了，中断请求保存到那里?

## 如果虚拟机屏蔽了中断，那么物理机 CPU 还是可以接受中断吧

## 看看 vmcs 中的内容

如何理解 GUEST_SYSENTER_EIP
```c
	pr_err("RSP = 0x%016lx  RIP = 0x%016lx\n",
	       vmcs_readl(GUEST_RSP), vmcs_readl(GUEST_RIP));
	pr_err("RFLAGS=0x%08lx         DR7 = 0x%016lx\n",
	       vmcs_readl(GUEST_RFLAGS), vmcs_readl(GUEST_DR7));
	pr_err("Sysenter RSP=%016lx CS:RIP=%04x:%016lx\n",
	       vmcs_readl(GUEST_SYSENTER_ESP),
	       vmcs_read32(GUEST_SYSENTER_CS), vmcs_readl(GUEST_SYSENTER_EIP));
```

甚至连 host 也有:
```c
	pr_err("Sysenter RSP=%016lx CS:RIP=%04x:%016lx\n",
	       vmcs_readl(HOST_IA32_SYSENTER_ESP),
	       vmcs_read32(HOST_IA32_SYSENTER_CS),
	       vmcs_readl(HOST_IA32_SYSENTER_EIP));
```


## 在 Inlel 手册中提到的 SPP-related 的


## 看看这个 feautre
X86_FEATURE_EPT_AD ，想不到这个 feature 这么晚才加入啊

看看有这个 feature 和没有这个 feautre ，大家都是如何维持生活的


## 为什么 l2 中再去启动虚拟机就那么慢了

9.9S vs 1.6S
```txt
[    9.419293] Run /tmp/vmtest-initsYgqe.sh as init process
[    9.531151] mount (94) used greatest stack depth: 12984 bytes left
[    9.592371] grep (96) used greatest stack depth: 12912 bytes left
[    9.604443] mount (95) used greatest stack depth: 12168 bytes left
[    9.604568] vmtest: Mounting tmpfs at /dev/shm
[    9.728853] vmtest: Mounting tmpfs at /run
[    9.811744] vmtest: Mounting sysfs at /sys
[    9.853498] vmtest: Mounting debugfs at /sys/kernel/debug
[    9.904198] vmtest: Mounting tracefs at /sys/kernel/debug/tracing
```

## 神奇的嵌套

似乎很多 l1 被 l0 disable 的 feature ，在 l2 中可以继续使用
例如 l0 不打开 ept ，但是 l1 可以让 l2 继续使用 ept 的。

## 如果是 rmap ，那么 mmu notifier 如何工作？

现在维护的是 gva 到 hpa 的映射的，所以，出现的

## 记录一下

有实话 kvm module 无法被 remove
有点难复现
有办法知道 module 被什么 reference 吗?

原来是打开了 bpftrace 跟踪 module

## 终极一战，为什么有了 ept ，还要这么傻的 rmap 才可以
- https://www.linux-kvm.org/images/f/fd/2012-forum-yoshikawa.pdf

不可以

## kvm 支持 gdb 的方法


## 测试下，给 window 的一个驱动

Documentation/virt/kvm/api.rst
```txt
4.87 KVM_SET_GUEST_DEBUG
------------------------

:Capability: KVM_CAP_SET_GUEST_DEBUG
:Architectures: x86, s390, ppc, arm64
:Type: vcpu ioctl
:Parameters: struct kvm_guest_debug (in)
:Returns: 0 on success; -1 on error

::

  struct kvm_guest_debug {
       __u32 control;
       __u32 pad;
       struct kvm_guest_debug_arch arch;
  };
```

## 这个显然都需要看看，如果再去看新的东西
https://docs.kernel.org/virt/index.html

## host 和 guest 关闭中断是独立的吧，找到证据


## svm_check_emulate_instruction

看看是不是真的有

```txt
* Detect and workaround Errata 1096 Fam_17h_00_0Fh.
```

复现方法，在 qemu 中，virtio-blk 给 dpdk 使用


## 看看
https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/commit/?id=72fdfc75d4217b32363cc80def3de2cb3fef3f02

## 其实中断虚拟化最不可以理解的事情就是，为什么 APICv 的各种寄存器需要额外的模拟

例如 kvm:kvm_cr 可以观察到大量的 cr0 的 exit ，应该是都是中断打开关闭，这个直接更新到
vmcs 中或者到寄存器中，退出的时候更新到寄存器，不可以吗?

## kvm_stat 的两个模式 debugfs 和 tracepoint 模式都用用

## 测试一下 linux mov cr8 的效果
https://bugzilla.kernel.org/show_bug.cgi?id=218267#c9

## 又来一波大的重构啊
https://lore.kernel.org/all/20241128013424.4096668-1-seanjc@google.com/

## 这个说做什么的?
- arch/arm64/kvm/hyp/nvhe/mem_protect.c                                            |  884 +++----

## 做一个总结
记录那些虚拟化的内容已经被硬件实现了:

- kvm-clock : MSR TSC AUX 的
- 中断虚拟化 : APICv

其实可以去检查初始化的 feature 的判断，大致可以知道有那些硬件 offload

带着这个视角来分析代码，理解起来会更加容易一些

## 这里的 hypercall 看看都有机器在调用吗?
```c
#define KVM_HC_VAPIC_POLL_IRQ		1
#define KVM_HC_MMU_OP			2
#define KVM_HC_FEATURES			3
#define KVM_HC_PPC_MAP_MAGIC_PAGE	4
#define KVM_HC_KICK_CPU			5
#define KVM_HC_MIPS_GET_CLOCK_FREQ	6
#define KVM_HC_MIPS_EXIT_VM		7
#define KVM_HC_MIPS_CONSOLE_OUTPUT	8
#define KVM_HC_CLOCK_PAIRING		9
#define KVM_HC_SEND_IPI		10
#define KVM_HC_SCHED_YIELD		11
```

## 关注一下，有趣的
```c
	[EXIT_REASON_GDTR_IDTR]		      = handle_desc,
```

## 这个错误的原因是什么

```txt
[ 5480.384943] kvm: vcpu 13: requested 11962 ns lapic timer period limited to 200000 ns
```

## 为什么 kvm_vcpu 需要持有 pid 结构体
```txt
struct kvm_vcpu {
	struct pid __rcu *pid;
```

## 如何理解 int kvm::created_vcpus

引用的这个变量的地方都是看这个 vcpus 是不是为 0 的。

但是，难道 kvm::created_vcpus 的数量不是总是大于 0 吗?

## 使用 suberror 1 来整理一下 KVM_INTERNAL_ERROR_EMULATION 的原因
已经很接近问题了

好好利用
/home/martins3/core/vn/code/src/m/arch/x86_64/suberror.c


https://oenhan.com/kvm_shared_msrs

## 可以看懂 fast_page_fault 的逻辑吗?

这两个函数是什么意思?

sptep = kvm_tdp_mmu_fast_pf_get_last_sptep(vcpu, fault->gfn, &spte);

sp = sptep_to_sp(sptep);


## 和 kvm selftests 类似的东西

- https://hackernoon.com/x86-handling-exceptions-lds3uxc
- https://gitlab.com/mvuksano/kvm-playground

## 这个到时候整理一下
这个东西可以实现吗?
https://liujunming.top/2020/12/01/VT-x-Information-for-VM-Exits-During-Event-Delivery/

## 这个做什么的?
arch/arm64/kvm/config.c                                                                | 1085 +++++++++
arch/arm64/tools/sysreg                                                                | 1012 ++++++++-

## 对于 qemu 注册的内存为 userfaultfd ，如果 userfaultfd
msg.arg.pagefault.flags 显示为  write ，那么 kvm 已经可以记录的到这个 dirty

```txt
[<0>] handle_userfault+0x447/0x8f0
[<0>] shmem_get_folio_gfp+0x3b3/0x610
[<0>] shmem_fault+0x86/0x300
[<0>] __do_fault+0x30/0x180
[<0>] do_fault+0xbe/0x4d0
[<0>] __handle_mm_fault+0x7d1/0xfe0
[<0>] handle_mm_fault+0x17f/0x2e0
[<0>] __get_user_pages+0x23d/0x1410
[<0>] get_user_pages_unlocked+0xe6/0x390
[<0>] hva_to_pfn+0x2bd/0x400 [kvm]
[<0>] __kvm_faultin_pfn+0x62/0xa0 [kvm]
[<0>] kvm_mmu_faultin_pfn+0x27b/0x6b0 [kvm]
[<0>] kvm_tdp_page_fault+0x97/0xf0 [kvm]
[<0>] kvm_mmu_do_page_fault+0x1ec/0x240 [kvm]
[<0>] kvm_mmu_page_fault+0x82/0x6f0 [kvm]
[<0>] vmx_handle_exit+0x21a/0x880 [kvm_intel]
[<0>] vcpu_enter_guest.constprop.0+0x64d/0x1270 [kvm]
[<0>] kvm_arch_vcpu_ioctl_run+0x357/0x6d0 [kvm]
[<0>] kvm_vcpu_ioctl+0x122/0xa20 [kvm]
[<0>] __x64_sys_ioctl+0xa0/0xe0
[<0>] do_syscall_64+0xc1/0x220
[<0>] entry_SYSCALL_64_after_hwframe+0x77/0x7f
```

## 在调用 ioctl 的时候，居然是可以

```c
/*
 * Forcibly leave nested mode in order to be able to reset the VCPU later on.
 */
static void vmx_leave_nested(struct kvm_vcpu *vcpu)
{
	if (is_guest_mode(vcpu)) {
		to_vmx(vcpu)->nested.nested_run_pending = 0;
		nested_vmx_vmexit(vcpu, -1, 0, 0);
	}
	free_nested(to_vmx(vcpu));
}
```

## sreg2 是意思

```txt
    ret = has_sregs2 ? kvm_put_sregs2(x86_cpu) : kvm_put_sregs(x86_cpu);
```

- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - qemu_wait_io_event
          - process_queued_cpu_work
            - do_kvm_cpu_synchronize_state
              - do_kvm_cpu_synchronize_state
                - kvm_arch_get_registers
                  - kvm_get_sregs2


## 看看 qemu 不去处理 nested 状态，还会有问题吗?

https://lkml.iu.edu/1804.1/05279.html 这里引入的
```txt
History:        #0
Commit:         8fcc4b5923af5de58b80b53a069453b135693304
Author:         Jim Mattson <jmattson@google.com>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Tue 10 Jul 2018 05:27:20 PM CST
Committer Date: Mon 06 Aug 2018 11:58:30 PM CST

kvm: nVMX: Introduce KVM_CAP_NESTED_STATE

For nested virtualization L0 KVM is managing a bit of state for L2 guests,
this state can not be captured through the currently available IOCTLs. In
fact the state captured through all of these IOCTLs is usually a mix of L1
and L2 state. It is also dependent on whether the L2 guest was running at
the moment when the process was interrupted to save its state.

With this capability, there are two new vcpu ioctls: KVM_GET_NESTED_STATE
and KVM_SET_NESTED_STATE. These can be used for saving and restoring a VM
that is in VMX operation.

Cc: Paolo Bonzini <pbonzini@redhat.com>
Cc: Radim Krčmář <rkrcmar@redhat.com>
Cc: Thomas Gleixner <tglx@linutronix.de>
Cc: Ingo Molnar <mingo@redhat.com>
Cc: H. Peter Anvin <hpa@zytor.com>
Cc: x86@kernel.org
Cc: kvm@vger.kernel.org
Cc: linux-kernel@vger.kernel.org
Signed-off-by: Jim Mattson <jmattson@google.com>
[karahmed@ - rename structs and functions and make them ready for AMD and
             address previous comments.
           - handle nested.smm state.
           - rebase & a bit of refactoring.
           - Merge 7/8 and 8/8 into one patch. ]
Signed-off-by: KarimAllah Ahmed <karahmed@amazon.de>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

```c
struct kvm_x86_nested_ops vmx_nested_ops = {
	.leave_nested = vmx_leave_nested,
	.is_exception_vmexit = nested_vmx_is_exception_vmexit,
	.check_events = vmx_check_nested_events,
	.has_events = vmx_has_nested_events,
	.triple_fault = nested_vmx_triple_fault,

	.get_state = vmx_get_nested_state,
	.set_state = vmx_set_nested_state,
  // 这个是和 hyperv 相关的
	.get_nested_state_pages = vmx_get_nested_state_pages,

	.write_log_dirty = nested_vmx_write_pml_buffer,
#ifdef CONFIG_KVM_HYPERV
	.enable_evmcs = nested_enable_evmcs,
	.get_evmcs_version = nested_get_evmcs_version,
	.hv_inject_synthetic_vmexit_post_tlb_flush = vmx_hv_inject_synthetic_vmexit_post_tlb_flush,
#endif
};
```

## 似乎质量不错

https://tandasat.github.io/Hypervisor-101-in-Rust/introduction/types-of-hypervisors.html
https://news.ycombinator.com/item?id=45283731


## kvm_arch_vcpu_ioctl_run 的调用频率非常低
<!-- 6aff7bfd-ef04-41fe-856a-3973fea1d905 -->

总体来说，这是预期的，只有那些导致需要 exit 到 userspace 的 io
才会最后导致 kvm_arch_vcpu_ioctl_run 。不过观察到 1 分钟可能只有几次
kvm_arch_vcpu_ioctl_run 的调用，这还是让人有点惊讶的。

观测下，到底是什么导致 kvm_arch_vcpu_ioctl_run 被调用

TODO : 可以观察这个的 function graph ，看看主要都是在哪里循环，构建一个基本印象。

## fred 是什么
https://www.phoronix.com/news/Intel-FRED-Incompatible-ENDBR64

https://lore.kernel.org/lkml/e7dd1510-6ffa-429a-9b07-55ad83d40d7b@zytor.com/#r

https://liujunming.top/2023/09/10/Intel-FRED-feature/

## li wangpeng

还是有点东西的

https://mp.weixin.qq.com/s/jsuCq8FFyVddi-oLhWcgTw

## 这个文档最后最后查漏补缺一下
https://www.kernel.org/doc/html/latest/virt/index.html

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
