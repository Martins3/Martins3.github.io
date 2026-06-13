# kvm 嵌套虚拟化

## 思考 nested 的模型
<!-- edefb734-c71c-47cc-a766-e5e8bf016789 -->

只有一个事情，就是 is_guest_mode()

在任何状态下，exit 都是首先 exit 物理机中的，然后让物理机中判断，到底该如何解决:

```c
static inline bool is_guest_mode(struct kvm_vcpu *vcpu)
{
	return vcpu->arch.hflags & HF_GUEST_MASK;
}
```
设置上 HF_GUEST_MASK 只有两个地方:
- nested_vmx_enter_non_root_mode : arch/x86/kvm/vmx/nested.c
- nested_vmcb02_prepare_control : arch/x86/kvm/svm/nested.c

当前物理 CPU 执行了 vmenter 之后 到底在执行哪一个 vCPU 的代码
实现方法就是看 vmenter 那个 vmcs 就可以了。


is_guest_mode 是问题关键:
1. 如果是 is_guest_mode , L0 不去区分当前运行的是 L2, L3 还是 L4 ，都是 is_guest_mode 。当 handle exit ，那么就通过
nested_vmx_reflect_vmexit 判断到底直接处理还是注入让 l1 处理，接下来是 l1 的工作来确定如何模拟
换言之， 如果是 is_guest_mode ，解决他的各种问题，如果直接解决了，继续运行，如果无法解决，那么 sync vmcs12 ，
进入到 l1 ，让 l1 帮解决
2. 如果不是 is_guest_mode ，那就是普通的 trap and emulate ，说明就是我们熟悉的内容了。

当知道要进入到 L2 ，是一定可以被物理机观察到的，也就是遇到了 vmexit
exit 的原因是 vmenter 所以，开始制作 vmcs02

```txt
@[
    kvm_init_mmu+5
    nested_vmx_load_cr3+92
    prepare_vmcs02.constprop.0+746
    nested_vmx_enter_non_root_mode+4489
    nested_vmx_run+264
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 1297386
```

怎么不算是一种 context switch 呢?
- function call (切换几个寄存器)
- setjmp longjump (切换更多的)
- syscall (很多东西了)
- vmenter (所有的东西) 从 context switch 的角度思考嵌套虚拟化，也就是当加载 vmcs01 的时候，就是执行 L1 虚拟机，
当执行 vmcs02 的时候，就是执行 L2 虚拟机。

具体问题:
1. 无穷嵌套下影子页表如何工作?

## 嵌套 vmcs 的同步
<!-- b9cdbd7a-6e16-4194-85c5-6688f3aebc76 -->

```c

/*
 * prepare_vmcs02 is called when the L1 guest hypervisor runs its nested
 * L2 guest. L1 has a vmcs for L2 (vmcs12), and this function "merges" it
 * with L0's requirements for its guest (a.k.a. vmcs01), so we can run the L2
 * guest in a way that will both be appropriate to L1's requests, and our
 * needs. In addition to modifying the active vmcs (which is vmcs02), this
 * function also has additional necessary side-effects, like setting various
 * vcpu->arch fields.
 * Returns 0 on success, 1 on failure. Invalid state exit qualification code
 * is assigned to entry_failure_code on failure.
 */
static int prepare_vmcs02(struct kvm_vcpu *vcpu, struct vmcs12 *vmcs12,
			  bool from_vmentry,
			  enum vm_entry_failure_code *entry_failure_code);

/*
 * Update the guest state fields of vmcs12 to reflect changes that
 * occurred while L2 was running. (The "IA-32e mode guest" bit of the
 * VM-entry controls is also updated, since this is really a guest
 * state bit.)
 */
static void sync_vmcs02_to_vmcs12(struct kvm_vcpu *vcpu, struct vmcs12 *vmcs12);
```

## [ ] Documentation/virt/kvm/x86/nested-vmx.rst
https://www.kernel.org/doc/html/latest/virt/kvm/x86/nested-vmx.html

## 嵌套虚拟化对于热迁移的支持
<!-- 358d0692-b375-49e8-9039-cb6bfe8dcc8e -->

[Documentation/virt/kvm/x86/running-nested-guests.rst](https://www.kernel.org/doc/html/latest/virt/kvm/x86/running-nested-guests.html)

- Migrating an L1 guest, with a live nested guest in it, to another bare metal host, works as of Linux kernel 5.3 and QEMU 4.2.0 for Intel x86 systems, and even on older versions for s390x.
- On AMD systems, once an L1 guest has started an L2 guest, the L1 guest should no longer be migrated or saved (refer to QEMU documentation on “savevm”/”loadvm”) until the L2 guest shuts down.

Migrating an L2 guest is always expected to succeed, so all the following scenarios should work even on AMD systems:
- Migrating a nested guest (L2) to another L1 guest on the same bare metal host.
- Migrating a nested guest (L2) to another L1 guest on a different bare metal host.
- Migrating a nested guest (L2) to a bare metal host.

总之，intel 为所欲为，AMD 文档说是不能在背着 L2 的情况下热迁移。
唯一区别就是，是否可以背着 L2 热迁移。
估计这里还是由于 nested 的状态需要 QEMU / kvm 来额外保存。

(在 hygon 机器上测试下过，来回热迁移好几次，L1 可以背着 L2 热迁移，所以这里的文档是错误的，
也就是在任何情况下都是可以热迁移的)

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(基本上是对的，因为当前嵌套的设计，L1 和 L2 在 kvm 中都是维护了一套 vmcs ，所以现在热迁移的时候，在 target 端
就也需要填充这些内容。如果是一个 L2 热迁移到 L1 上，或者物理机上，那就普通的热迁移没有什么区别
)

如果你说的是 x86 + KVM 场景里，把一个正在作为 L1 hypervisor 运行、且里面还有活跃 L2 的 VM 做热迁移，那么 QEMU 额外要补的核心不是 “迁移 L2 的
RAM/磁盘”，而是把 KVM 内核里那部分不在普通 guest RAM 里的嵌套执行状态 一起迁走。当前 QEMU master 这条主线其实已经有了，重点是这几件事：

1. 从 KVM 取回并恢复 nested state
   QEMU 需要检查 KVM_CAP_NESTED_STATE，并在 vCPU save/load 时调用 KVM_GET_NESTED_STATE / KVM_SET_NESTED_STATE。迁移流里要序列化 struct
   kvm_nested_state，包括 VMX 的 vmcs12/shadow_vmcs12 或 SVM 的 vmcb12。

accel/kvm/kvm-all.c
```c
       s->max_nested_state_len = kvm_check_extension(s, KVM_CAP_NESTED_STATE);
```
target/i386/kvm/kvm.c 中
```txt

	max_nested_state_len = kvm_max_nested_state_length();
```
target/i386/machine.c 中:
```c
static const VMStateDescription vmstate_vmx_nested_state = {
    .name = "cpu/kvm_nested_state/vmx",
    .version_id = 1,
    .minimum_version_id = 1,
    .needed = vmx_nested_state_needed,
    .fields = (const VMStateField[]) {
        VMSTATE_U64(hdr.vmx.vmxon_pa, struct kvm_nested_state),
        VMSTATE_U64(hdr.vmx.vmcs12_pa, struct kvm_nested_state),
        VMSTATE_U16(hdr.vmx.smm.flags, struct kvm_nested_state),
        VMSTATE_END_OF_LIST()
    },
    .subsections = (const VMStateDescription * const []) {
        &vmstate_vmx_vmcs12,
        &vmstate_vmx_shadow_vmcs12,
        NULL,
    }
};
```

2. 处理 “L2 正在运行时” 的隐藏 CPU 事件状态
   单靠普通寄存器不够。尤其 VMX 下，如果 L2 里有 pending exception，QEMU 还需要 KVM_CAP_EXCEPTION_PAYLOAD，否则无法区分 pending/injected，也
   拿不到正确的 CR2/DR6 payload，迁移应直接 fail。对应实现见 target/i386/machine.c:251 和 target/i386/kvm/kvm.c:3231。

3. 在目的端做严格校验并按正确顺序恢复
   QEMU 要验证 nested state 的 size/format 是否被目的端内核支持；不支持就拒绝迁移。恢复顺序上也要先放好 FEATURE_CONTROL、sregs/EFER.SVME，再
   KVM_SET_NESTED_STATE，否则状态不一致。见 target/i386/machine.c:1201 和 target/i386/kvm/kvm.c:5722。
4. 保证 CPU ABI 可迁移
   如果 L1 需要跑 nested virt，QEMU 还得保证源/目的端暴露给 L1 的 CPU 特性一致。官方建议是跨主机迁移时用 named CPU model，再显式开 vmx=on/
   svm=on，不要直接 -cpu host 做异构迁移。

总结:
- QEMU 不需要“单独迁移 L2 VM 对象”。L2 的内存/磁盘内容本质上都是 L1 看到的普通内存和存储，普通 live migration 已经覆盖；真正额外要补的是 KVM 持有的 nested CPU state。


## L1 中有没有 /sys/module/kvm_intel/parameters/enable_apicv
<!-- 67703651-b70a-4419-8673-78ce0008c330 -->

有的

恐怕是需要对应的代码支持的。

## 在开始之前，我们先将需要虚拟化的内容都罗列出来，然后看是如何虚拟化的

1. timer
2. vfio
3. pmu : 如何保证观测到性能数据没有问题。
4. vmcs
5. 中断控制器的优化加速
6. 热迁移 ！

## 问题
1. 如果在 qemu 中调试 kvm ，那些地方会失真 ?

## [Nested Virtualization : State of the art and future directions](https://www.linux-kvm.org/images/3/33/02x03-NestedVirtualization.pdf)

这个内容有点老了，一些展望目前已经实现了，例如 nested iommu 以及 arm

![](docs/kvm/img/intel-nested-work-flow.png)


## [KVM MMU Virtualization](https://31main.cn-bj.ufileos.com//Uploads/Files/2012/10/19/10106/634862700236047573.pdf)

清晰明了，强烈推荐 👍

简单介绍背景，然后分析三种处理嵌套的 MMU 场景，目前就是硬件辅助了。

## [Improving KVM nVMX](https://www.linux-kvm.org/images/8/8e/Improving_KVM_nVMX.pdf)

关于一些基本的问题

# nest

- https://www.usenix.org/legacy/events/osdi10/tech/full_papers/Ben-Yehuda.pdf : 文章
- https://www.usenix.org/conference/osdi10/turtles-project-design-and-implementation-nested-virtualization : 视频
- [ ] https://www.kernel.org/doc/html/latest/virt/kvm/nested-vmx.html
- [ ] https://www.kernel.org/doc/html/latest/virt/kvm/running-nested-guests.html


1. L1 RDMSR bad_msr
2. RDMSR exits to L0
3. L0 emulates RDMSR and queues `#GP`:
   - Save pending exception in struct kvm_vcpu_arch
   - Set KVM_REQ_EVENT
4. Before host entry to guest, KVM_REQ_EVENT evaluates queued events:
   1. Inject pending `#GP` to guest by VMCS
   2. _If (vCPU in guest-mode && Has event which should L2→L1) ⇒ Emulate L2→L1_

- [ ] event injection 的模拟 : 在没有 nested 的时候，需要靠 event injection 告知
  - [ ] kvm_inject_page_fault : 居然存在 async page fault
    - [ ] kvm_queue_exception_e
      - [ ] kvm_multiple_exception
        - [ ] kvm_make_request : **各种地方调用 : step 1**
  - [ ] vcpu_enter_guest **在此处检查 : step 2**
    - [ ] kvm_check_request && inject_pending_event
      - [x] kvm_x86_ops.set_nmi(vcpu); 和 kvm_x86_ops.set_irq(vcpu); 来注入，vmx 注册函数最后会写入 vmcs 来实现
        - `kvm_x86_ops.nested_ops->check_events(vcpu);` : _Call check_nested_events() even if we reinjected a previous event in order for caller to determine if it should require immediate-exit from L2 to L1 due to pending L1 events which require exit from L2 to L1._
        - [ ] vmx_check_nested_events : 当在 nested mode 的时候才会调用
          - [ ] nested_vmx_check_exception : Has event which should L2→L1
          - [ ] nested_vmx_inject_exception_vmexit(vcpu, exit_qual);
            - [ ] nested_vmx_vmexit : 其实大多数事情利用 L0 处理 L1 的 vm exit handler 都是可以处理的，但是部分事情需要 L1 的处理，所以会退到 L1, 让 L1 辅助处理
              - [ ] nested_vmx_vmexit 完成的事情 : 将 vmcs 切换为管理 L1 的 vmcs，然后一些事情（是什么事情 ？）的处理，进入到 L1 中间
              - sync_vmcs02_to_vmcs12 : 准备基本内容
              - 比如利用 prepare_vmcs12 将 event (比如 page fault ) 写入到 vmcs12 中间
                - [ ] 但是注入的消息，L1 如何读去 vmcs12 中间的 `vmcs12->vm_exit_reason` `vmcs12->exit_qualification` `vmcs12->vm_exit_intr_info`
                  - 应该是利用指令 VMREAD/VMWRITE/VMCLEAR/VMPTRLD 访问，然后从 guest L1 中间弹出来，最后进入到 L1 中间，L1 检查到的原因就是这些
                    - [ ] check 一下代码，如何让 L0 的 vmlaunch 正好进入到 L1 的 vmexit 的处理位置
            - [ ] L0 是处理力所能及的事情, 既然已经到了 host 中间，减少在虚拟化中间的执行（L1 中执行）才是最高效的
              - [ ] 如果处理力所能及的事情，会不会导致 L1 的逻辑不正确

```c
static int vmx_check_nested_events(struct kvm_vcpu *vcpu)
{
  // ...
	/*
	 * Process any exceptions that are not debug traps before MTF.
	 */
	if (vcpu->arch.exception.pending && !vmx_pending_dbg_trap(vcpu)) {
		if (block_nested_events)
			return -EBUSY;
		if (!nested_vmx_check_exception(vcpu, &exit_qual))
			goto no_vmexit;
		nested_vmx_inject_exception_vmexit(vcpu, exit_qual);
		return 0;
	}
  // ...
```

- [ ] 理解下面这段的注释 :

```c
// inject_pending_event 的注释 :
	/*
	 * Do not inject an NMI or interrupt if there is a pending
	 * exception.  Exceptions and interrupts are recognized at
	 * instruction boundaries, i.e. the start of an instruction.
	 * Trap-like exceptions, e.g. #DB, have higher priority than
	 * NMIs and interrupts, i.e. traps are recognized before an
	 * NMI/interrupt that's pending on the same instruction.
	 * Fault-like exceptions, e.g. #GP and #PF, are the lowest
	 * priority, but are only generated (pended) during instruction
	 * execution, i.e. a pending fault-like exception means the
	 * fault occurred on the *previous* instruction and must be
	 * serviced prior to recognizing any new events in order to
	 * fully complete the previous instruction.
	 */
```

- [x] 一个 vcpu 进入 guest mode 意味着什么 ?
  - [x] handle_vmlaunch/nested_vmx_run ==> nested_vmx_run ==> nested_vmx_enter_non_root_mode ==> enter_guest_mode ==> is_guest_mode
    - vcpu_enter_guest 和 enter_guest_mode 在两个调用路线上
  - 首先，在 guest 中间运行 hypervisor 并不能知道自己在虚拟机中间，is_guest_mode 是 L0 在执行代码的时候, 用于表示当前 load 的 vmcs 是 vmcs02
  - 表示有些事情，本来是 L1 处理的，但是现在需要 L0 模拟给 L1 了
  - 表示当前 L0 在替 L1 执行 L2, 如果不是 guest mode，那么就是 L0 在执行 L1 的代码，比如 L1 的用户态程序
  - [x] is_guest_mode 的情况下执行代码，存在很有意思的事情，那就是本事就是在 L0 的状态下执行代码模拟 L1，但是依旧按照道理需要从 L1 退出，当然是模拟退出的
    - 这一个模拟工作就是 : nested_vmx_vmexit

```c
	/* SMM related state */
	struct {
		/* in VMX operation on SMM entry? */
		bool vmxon;
		/* in guest mode on SMM entry? */
		bool guest_mode;
	} smm;

static struct kvm_x86_ops vmx_x86_ops __initdata = {
  // ...
	.pre_enter_smm = vmx_pre_enter_smm,
	.pre_leave_smm = vmx_pre_leave_smm,
  // ...
}
```
- [ ] vmx_pre_enter_smm
- [ ] vmx_pre_leave_smm
- vcpu_enter_guest ==> inject_pending_event ==> enter_smm

- [ ] 对比一下 struct vmcs 和 struct vmcs12 的内容

- [ ] check shadow vmcs 的工作方法 ?

  - copy_shadow_to_vmcs12 : 有些是 writable 的,
  - [ ] 所以，shadow 和 enlightened vmcs 的并行的两种方法吗?

- [ ] enlightened_vmcs_enabled : nested_enable_evmcs() 中间调用

  - [x] `vmx->nested.enlightened_vmcs_enabled && !vmx->nested.hv_evmcs` : 当 enable, 但是具体的数据不在
  - [ ] nested_get_vmcs12_pages() : 似乎是 L2 的 vmcs 到 L1 的 vmcs 装换的
  - [ ] nested_vmx_handle_enlightened_vmptrld : This is an equivalent of the nested hypervisor executing the vmptrld instruction.
  - [ ] copy_enlightened_to_vmcs12
  - [ ] nested_sync_vmcs12_to_shadow : 分别调用下面*前两个*函数, 被 vmx_prepare_switch_to_guest 唯一调用
    - [ ] copy_vmcs12_to_enlightened
    - [ ] copy_enlightened_to_vmcs12
    - [ ] copy_vmcs12_to_shadow
    - [ ] copy_shadow_to_vmcs12
    - [ ] shadow 或者 enlightened 指的是 vmcs02 吗 ?
      - [ ] sync_vmcs02_to_vmcs12 和 copy_enlightened_to_vmcs12 的作用分别是什么呀 ?
      - [ ] sync_vmcs02_to_vmcs12_rare : 似乎 vmcs02 就是 L2 运行所需要的 L2
    - 从 shadow 和 enlightened 出现的位置，两者必然互相补充的
    - [ ] vcpu_run ==> vcpu_enter_guest ==> vmx_prepare_switch_to_guest : 从调用路线看，没有 nested 也会调用用到这里 ?
      - [x] kvm 在执行 guest 的代码的时候，guest 如果想要继续运行虚拟机，其实他是不知道的, 只有到 guest 执行 vmlaunch 的时候， vcpu 才知道 guest 的真正意图。
      - [x] 当 vcpu 处于 guest mode 的时候，对于各种 vm instruction(比如 vmlaunch 之类的) 自然可以随意截断，但是对于其他的原因，需要靠 in_guest_mode 进行区分从而, 从而考虑是在 L0 中间处理，还是 L1 中间

- [ ] nested_vmx_vmexit : 显然不存在 VMEXIT 指令，只是当 L1 执行各种 vmx 指令的时候, 需要进行模拟
  - 想要达到的效果，L1 意味自己靠 vmlaunch 维持了 L2 的运行(其 ept, vmcs 之类的)，其实这些事情 L0 给 L1 模拟的
  - [ ] 为什么类似于 ept violation 不需要调用 nested_vmx_vmexit
  - [ ] sync_vmcs02_to_vmcs12 :
  - [ ] prepare_vmcs12 : Note that we do not have to copy here all VMCS fields, just those that could have changed by the L2 guest or the exit - i.e.,
  - [ ] vmx*switch_vmcs : *迷惑\_

```c
/*
 * Emulate an exit from nested guest (L2) to L1, i.e., prepare to run L1
 * and modify vmcs12 to make it see what it would expect to see there if
 * L2 was its real guest. Must only be called when in L2 (is_guest_mode())
 */
void nested_vmx_vmexit(struct kvm_vcpu *vcpu, u32 vm_exit_reason,
		       u32 exit_intr_info, unsigned long exit_qualification)
```

- [x] vmcs 是在内存中间，还是在 cpu 的寄存器中间，类似 msr 之类的

  - [x] 从 vmcs_load() 的实现看，利用 vmptrld 指令加载一块内存
    - [ ] 但是为什么访问 vmcs 内容的时候需要特殊的指令, 可以实现原子性，可以保护该区域，可以用于触发 vmexit 从而实现 nested，可以加载不同 vmcs, 从而访问的代码不需要修改（给我一个无法拒绝的理由）
    - **一个 vcpu 只能让一个 vmcs 是被 cpu "认可的", 也即是最近的 vmptrld 的那个 vmcs**

- [ ] 进入到 L2 guest 中间的准备工作是什么 ?

  - [ ] vmx_check_nested_events

- [x] vmx_get_nested_state : ioctl [KVM_GET_NESTED_STATE](https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-get-nested-state) 直接到达此处
  - sync_vmcs02_to_vmcs12 && sync_vmcs02_to_vmcs12_rare : 拷贝到 vmcs12 中间
  - get_shadow_vmcs12(vcpu) : 拷贝到 shadow_vmcs12 中间
  - [ ] 所以 shadow 和 vmcs02 到底是什么关系 ?

## host state

- [ ] load_vmcs12_host_state : 当前运行的 vmcs 是 L1 的, 其中 `kvm_rsp_write(vcpu, vmcs12->host_rsp);` 对于 vmcs12 而言，其 host_rsp 其实是 L1，但是对于 L0 而言，kvm_rsp_write 就是在写 guest 的 rsp
  - [ ] 猜测这一个函数的作用其实是完成设置 L1 的状态，从而让 vmlaunch 进入 L1 的时候，让 L1 以为自己是从 L2 中间退出的

## vmcs12

- [ ] 理解 struct vmcs12 的注释:

  1. vmcs12 放在 L1 guest 的内存中间，但是访问方法靠 VMREAD/VMWRITE/VMCLEAR/VMPTRLD, 而不可以直接访问
  2. L1 guest 中间是可以运行多个不同的 L2 gues 的，所以在 L1 中间存在多个 vmcs12
  3. 在运行 nested_vmx_run() 的时候，把 vmcs12 加载到 vmcs02 中间

     - [ ] 所以 shadow 和 enlightened 的作用是什么 ？
     - [ ] load_vmcs12_host_state 来完成这个工作吗 ?
     - [ ] 如何切换到正确的 vmcs12 ？
     - [ ] 注意，一个 CPU 只有一个 vmcs

## vcpu->arch.regs

从 `__vmx_vcpu_run` 中间看:

```c
	vmx->fail = __vmx_vcpu_run(vmx, (unsigned long *)&vcpu->arch.regs,
				   vmx->loaded_vmcs->launched);
```

在 enter vmcs mode 之前，各种寄存器手动的从 `vcpu->arch.regs` 中间恢复

- [ ] 在 VMCS 中间，只有 segment register ，没有普通的 ？
  - [ ] 那么 host 的如何保存和回复
  - [ ] guest 如何 ?
  - [ ] 为什么只是在 vmcs 中间存放 segment register
- [ ] kvm_register_mark_dirty 有什么意义，不是所有的 register 都会标注吗 ?

## nest.c 的代码的阅读记录(should be clear)

- [ ] nested_vmx_hardware_setup : 最开始的时候初始化
  - [ ] 在 level one 的时候的注册 exit handler 比此处多很多
  - 在虚拟机中间的 hypervisors 可以通过什么方法知道自己其实是在虚拟机的 ?

## 等待处理的文档
- https://www.usenix.org/conference/osdi10/turtles-project-design-and-implementation-nested-virtualization
- https://archive.fosdem.org/2018/schedule/event/vai_kvm_on_hyperv/attachments/slides/2200/export/events/attachments/vai_kvm_on_hyperv/slides/2200/slides_fosdem2018_vkuznets.pdf
- https://archive.fosdem.org/2019/schedule/event/vai_enlightening_kvm/attachments/slides/2860/export/events/attachments/vai_enlightening_kvm/slides/2860/vkuznets_fosdem2019_enlightening_kvm.pdf

## 是如何处理 vmcs 的

- prepare_vmcs12 : L1 给 L2 准备的
- sync_vmcs02_to_vmcs12 :
- prepare_vmcs02 : L0 给 L2 准备

当 vmexit 的时候，需要将 vmcs02 同步到 vmcs12 中:

```txt
@[
    sync_vmcs02_to_vmcs12+1
    nested_vmx_vmexit+245
    nested_vmx_reflect_vmexit+580
    vmx_handle_exit+193
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 1255147
```

- [ ] vmcs02 如何同步到 vmcs01 ? 找到具体的那个写动作

## 关键文档 : 原始的 kvm forum

- https://www.linux-kvm.org/images/e/e9/Kvm-forum-2013-nested-virtualization-shadow-turtles.pdf
- https://www.linux-kvm.org/images/8/8c/Kvm-forum-2013-nested-ept.pdf

## ARM 上 nested kvm 的尝试

- https://lwn.net/Articles/728193/
- https://lore.kernel.org/linux-arm-kernel/20230515173103.1017669-1-maz@kernel.org/

真的太强了，这个 feature 连续搞了好多年了，居然还在坚持。

## 嵌套的影响

### svm

1. svm_set_cpu_caps : 影响 guest 看到的 cpu feature
2. kvm_enable_efer_bits(EFER_SVME | EFER_LMSLE);

## 看看 QEMU 中这个内容

- kvm_init_nested_state

KVM_CAP_NESTED_STATE

## 关于 FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX 的设置，真的非常头疼

其实嵌套的属性可以向每一个层次暴露?

在 fw_cfg_build_feature_control 中 /etc/msr_feature_control 是控制的虚拟机是否可以开启
嵌套虚拟化的功能。

虚拟机的 msr_feature_control 实际上提供的是，该虚拟机是否可以开启虚拟化。

- 关于 nested 之后，guest 中应该看不到 vmx 了

1. 如果在 L1 虚拟机中执行 modprobe kvm_intel nested=1 ，如果 modprobe kvm_intel nested=0 ，没有影响
2. 如果在 L1 虚拟机中设置 /etc/modprobe.d/kvm.conf 将 nested 参数设置为 0 之后，虚拟机的 msr_feature_control 将会变为 0

注意，kvm 存储了两个数值来控制
```c
	/*
	 * Only bits masked by msr_ia32_feature_control_valid_bits can be set in
	 * msr_ia32_feature_control. FEATURE_CONTROL_LOCKED is always included
	 * in msr_ia32_feature_control_valid_bits.
	 */
	u64 msr_ia32_feature_control;
	u64 msr_ia32_feature_control_valid_bits;
```
- msr_ia32_feature_control 从 guest os 中获取的，guest 初始化的时候来设置。
相当于执行
```sh
python3 -c 'import struct;f=open("/dev/cpu/0/msr", "rb");f.seek(0x3a);print(hex(struct.unpack("Q",f.read(8))[0]))'
```
- msr_ia32_feature_control_valid_bits 是 host kvm 的过程初始化，描述了 host kvm 可以提供的能力。


4.19 内核中，L1 虚拟机的 kvm 是否开启 nested 会导致会影响
/dev/cpu/0/msr 的输出结果。但是这个 nested=0 需要在 /etc/modprobe.d/kvm.conf 调整，
使用 modprobe kvm_intel nested=1 是无效的。

而且该效果会穿透进入到整个 qemu 中。

并不是，这个数值完全是具有随机性的。

```txt
[root@localhost 17:17:15 ~]$  cat /sys/module/kvm_intel/parameters/nested
1
[root@localhost 17:17:17 ~]$ python3 -c 'import struct;f=open("/dev/cpu/0/msr", "rb");f.seek(0x3a);print(hex(struct.unpack("Q",f.read(8))[0]))'
0x0
```
然后也是可以的。



将主线的内核的测试结果是:

1. guest nested 关闭 x
2. qemu 注释掉 x
3. host 中 nested 关闭 ok

2021 年中增加了 init_ia32_feat_ctl 的实现，所以只要检测到 vmx 都回去设置，而 host 关闭 nest

为什么这个没有显示出来作用? 因为 FEATURE_CONTROL_LOCKED 直接被注释掉了
```c
static __init int vmx_disabled_by_bios(void)
{
	u64 msr;

	rdmsrl(MSR_IA32_FEATURE_CONTROL, msr);
	if (msr & FEATURE_CONTROL_LOCKED) {
		/* launched w/ TXT and VMX disabled */
		if (!(msr & FEATURE_CONTROL_VMXON_ENABLED_INSIDE_SMX)
			&& tboot_enabled())
			return 1;
		/* launched w/o TXT and VMX only enabled w/ TXT */
		if (!(msr & FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX)
			&& (msr & FEATURE_CONTROL_VMXON_ENABLED_INSIDE_SMX)
			&& !tboot_enabled()) {
			printk(KERN_WARNING "kvm: disable TXT in the BIOS or "
				"activate TXT before enabling KVM\n");
			return 1;
		}
		/* launched w/o TXT and VMX disabled */
		if (!(msr & FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX)
			&& !tboot_enabled())
			return 1;
	}

	return 0;
}
```

## freebsd nested
https://just.graphica.com.au/tips/freebsd-virtualisation-with-bhyve/
- 并不是 freebsd 支持 nested ，而是因为 host kvm 支持 nested ，
只是 freebsd 支持虚拟化而已。

## 从 MSR_IA32_FEAT_CTL 来分析嵌套虚拟化

1. 如何理解 vmx_leave_nested ?
```c
	case MSR_IA32_FEAT_CTL:
		if (!is_vmx_feature_control_msr_valid(vmx, msr_info))
			return 1;

		vmx->msr_ia32_feature_control = data;
		if (msr_info->host_initiated && data == 0)
			vmx_leave_nested(vcpu);

		/* SGX may be enabled/disabled by guest's firmware */
		vmx_write_encls_bitmap(vcpu, NULL);
		break;
```

2. msr->host_initiated : 有趣的，我们
```c
static inline bool is_vmx_feature_control_msr_valid(struct vcpu_vmx *vmx,
						    struct msr_data *msr)
{
	uint64_t valid_bits;

	/*
	 * Ensure KVM_SUPPORTED_FEATURE_CONTROL is updated when new bits are
	 * exposed to the guest.
	 */
	WARN_ON_ONCE(vmx->msr_ia32_feature_control_valid_bits &
		     ~KVM_SUPPORTED_FEATURE_CONTROL);

	if (!msr->host_initiated &&
	    (vmx->msr_ia32_feature_control & FEAT_CTL_LOCKED))
		return false;

	if (msr->host_initiated)
		valid_bits = KVM_SUPPORTED_FEATURE_CONTROL;
	else
		valid_bits = vmx->msr_ia32_feature_control_valid_bits;

	return !(msr->data & ~valid_bits);
}
```

3. handle_vmxon 和 kvm_cpu_vmxon 是一个对应的来处理
```c
static int handle_vmxon(struct kvm_vcpu *vcpu)
```

## kvm_is_using_evmcs() 是为了处理 vmx 在 hyperv 中使用吗?

drivers/hv/Kconfig


## 关于嵌套虚拟化 xen 的处理
https://wiki.xenproject.org/wiki/Nested_Virtualization_in_Xen

## 观测到底是否开启了嵌套虚拟化
```sh
sudo perf stat -I 1000 -e kvm:kvm_nested_vmrun
sudo perf stat -I 1000 -e kvm:kvm_nested_vmenter
```

## 嵌套虚拟化的实现
- https://31main.cn-bj.ufileos.com//Uploads/Files/2012/10/19/10106/634862700236047573.pdf

## 到底如何实现无穷嵌套 ?

现在的代码实现为 prepare_vmcs02 ，但是显然是不存在 prepare_vmcs03 ，是如何保证的。

## 时钟也需要考虑嵌套虚拟化，就在时钟的地方分析了

## 如何理解 is_guest_mode

现场的环境:

L1 32 个 vcpu 空负载，L2 4 个 vCPU 编译内核

1. 观察 L1 的 debugfs 内容如下
```txt
[root@nixos:/sys/kernel/debug/kvm/786263-25]# cat vcpu*/guest_mode
0
0
0
1
0
0
1
0
0
0
0
0
0
0
0
0
0
0
0
0
0
1
0
0
0
0
1
0
0
0
0
0
```
2. L2 的 debugfs 全部都是 0

```c
static inline void enter_guest_mode(struct kvm_vcpu *vcpu)
{
	vcpu->arch.hflags |= HF_GUEST_MASK;
	vcpu->stat.guest_mode = 1;
}

static inline void leave_guest_mode(struct kvm_vcpu *vcpu)
{
	vcpu->arch.hflags &= ~HF_GUEST_MASK;

	if (vcpu->arch.load_eoi_exitmap_pending) {
		vcpu->arch.load_eoi_exitmap_pending = false;
		kvm_make_request(KVM_REQ_LOAD_EOI_EXITMAP, vcpu);
	}

	vcpu->stat.guest_mode = 0;
}
```

### [x] L1 也会执行 enter_guest_mode 吗?

```txt
sudo bpftrace -e "kprobe:nested_vmx_enter_non_root_mode { @[kstack] = count(); }"
Attaching 1 probe...
^C

@[
    nested_vmx_enter_non_root_mode+5
    nested_vmx_run+291
    vmx_handle_exit+301
    kvm_arch_vcpu_ioctl_run+1718
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+194
    entry_SYSCALL_64_after_hwframe+121
]: 453
```

一般来说: nested_vmx_enter_non_root_mode -> enter_guest_mode 开始进入到 L2 中。


### [ ] 退出的地方一共有两个，但是进入只有一个

## 在嵌套的环境中， ept misconfig 有办法知道是 L 几的问题?

考虑到 docs/kvm/mmu/paging_tmpl.md 中专门存在 ept 的 page table walk 的，所以

```txt
 +-------------+   +-------------+     +---------------+   +--------------+
 |             |   |             |     |        x      |   |              |
 +-------------+   +-------------+     +---------------+   +--------------+

+-----------------------------------------------------------------------------+
|                                                                             |
+-----------------------------------------------------------------------------+
```
如果 L4 的运行过程中，ept 的 page table 缺失发生在 L3 中，那么整个流程是什么样子的 ?

## vmcs 之间的同步

### vmcs12
通过 VMPTRLD 记录下 L1 的内容。

```c
/*
 * The nested_vmx structure is part of vcpu_vmx, and holds information we need
 * for correct emulation of VMX (i.e., nested VMX) on this vcpu.
 */
struct nested_vmx {

	/*
	 * Cache of the guest's VMCS, existing outside of guest memory.
	 * Loaded from guest memory during VMPTRLD. Flushed to guest
	 * memory during VMCLEAR and VMPTRLD.
	 */
	struct vmcs12 *cached_vmcs12;
	/*
	 * Cache of the guest's shadow VMCS, existing outside of guest
	 * memory. Loaded from guest memory during VM entry. Flushed
	 * to guest memory during VM exit.
	 */
	struct vmcs12 *cached_shadow_vmcs12;
```

## 如何模拟 smm ？


## 想不到 nested vCPU 的 load 也是这个流程

```txt
@[
    kvm_arch_vcpu_load+361
    kvm_arch_vcpu_load+361
    kvm_sched_in+44
    finish_task_switch.isra.0+271
    __schedule+1116
    preempt_schedule_common+44
    __cond_resched+34
    __cond_resched_rwlock_write+59
    mmu_sync_children+690
    kvm_mmu_sync_roots+248
    kvm_mmu_load+849
    vcpu_enter_guest.constprop.0+3404
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+555
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 1
```
居然如此自然的东西。

本来的以为，如果 nested vCPU unload 的时候，会在 l1 中进行，但是
可以直接在 l0 中进行。

也就是，nested vCPU 可以直接被 sched in 和 sched out ，而不是:

nested vCPU sched out -> vmenter 到 l1 ， 执行 vCPU 的代码

直接 sched out / sched in ，不用通过 l1 ，只能说明当时 CPU 被直接 preempt 了，
如果 l2 idle ，就会到 l1 中， l1 idle 就会到物理机中。

## nested : 在那个函数中决定是否进入 L1
<!-- c8d84b52-2df3-4bed-b2cc-e6e507b4b5fa -->

nested_vmx_reflect_vmexit
```c
/*
 * Conditionally reflect a VM-Exit into L1.  Returns %true if the VM-Exit was
 * reflected into L1.
 */
```

在 `__vmx_handle_exit` 中，
如果当前 vcpu 是 is_guest_mode() ，那么调用 nested_vmx_reflect_vmexit 分析到底是 L0 中处理，
还是让 L1 中处理。
这个行为具有无穷嵌套的性质，例如当 L0 进一步启动了 L1 L2 L3 的时候，
在 L1 中可以观察到 nested_vmx_reflect_vmexit

## 这个字段的含义是什么?

```txt
static bool __read_mostly enable_shadow_vmcs = 1;
module_param_named(enable_shadow_vmcs, enable_shadow_vmcs, bool, S_IRUGO);
```

deepseek 说:

> 安全地模拟 L1 对 VMCS 的操作（如 VMREAD/VMWRITE）。
> 隔离 L1 和 L0 的 VMCS 访问，防止 L1 破坏 L0 的状态。


只有打开之后:
```c
static void set_current_vmptr(struct vcpu_vmx *vmx, gpa_t vmptr)
{
	vmx->nested.current_vmptr = vmptr;
	if (enable_shadow_vmcs) {
		secondary_exec_controls_setbit(vmx, SECONDARY_EXEC_SHADOW_VMCS);
		vmcs_write64(VMCS_LINK_POINTER,
			     __pa(vmx->vmcs01.shadow_vmcs));
		vmx->nested.need_vmcs12_to_shadow_sync = true;
	}
	vmx->nested.dirty_vmcs12 = true;
	vmx->nested.force_msr_bitmap_recalc = true;
}
```

也就是加载特定的 vmcs 的时候可以:

- vmx_handle_exit+300
  - handle_vmptrld+448
    - set_current_vmptr+1


看看手册中的: 26.10 VMCS TYPES: ORDINARY AND SHADOW ，很清晰了

如果没有 vmcs 的 shadow ，那么 l1 中执行 vmread 和 vmwrite 总是导致 exit ，
有了，那么不一定。

## vmcs01 和 vmcs12 vmcs02 是什么关系?
<!-- 26a99fd1-71fe-4878-9c90-139d940a91be -->

如果在运行 l1 ，那么 loaded_vmcs 指向 vmcs01 ，如果运行 l2 ，那么指向
vmcs02

```c
struct vcpu_vmx {

	/*
	 * loaded_vmcs points to the VMCS currently used in this vcpu. For a
	 * non-nested (L1) guest, it always points to vmcs01. For a nested
	 * guest (L2), it points to a different VMCS.
	 */
	struct loaded_vmcs    vmcs01;
  struct loaded_vmcs   *loaded_vmcs;

```

vmcs12 是 l1 看 l2 的视角缓存的 vmcs 。
```c
/*
 * The nested_vmx structure is part of vcpu_vmx, and holds information we need
 * for correct emulation of VMX (i.e., nested VMX) on this vcpu.
 */
struct nested_vmx {

	/*
	 * Cache of the guest's VMCS, existing outside of guest memory.
	 * Loaded from guest memory during VMPTRLD. Flushed to guest
	 * memory during VMCLEAR and VMPTRLD.
	 */
	struct vmcs12 *cached_vmcs12;
	/*
	 * Cache of the guest's shadow VMCS, existing outside of guest
	 * memory. Loaded from guest memory during VM entry. Flushed
	 * to guest memory during VM exit.
	 */
	struct vmcs12 *cached_shadow_vmcs12;
```

他们的关键 : prepare_vmcs02

## vmx nested 相关的硬件支持
<!-- fa34fe2c-2255-40f9-8324-c1e203aa806f -->

VMCS 中关联的字段:
```c
/* VMCS Encodings */
enum vmcs_field {
  // 按道理指向 l1 的 epte
  EPTP_LIST_ADDRESS               = 0x00002024,
  EPTP_LIST_ADDRESS_HIGH          = 0x00002025,

  // nested 和 普通的 ept 都是通过这个字段获取问题 GPA 在哪里
  GUEST_PHYSICAL_ADDRESS          = 0x00002400,
  GUEST_PHYSICAL_ADDRESS_HIGH     = 0x00002401,

  // 用于实现
  VMCS_LINK_POINTER               = 0x00002800,
  VMCS_LINK_POINTER_HIGH          = 0x00002801,
```

### VMCS_LINK_POINTER

在嵌套虚拟化中，场景通常是：**L0（物理机）** 上运行着 **L1（虚拟机监控器，如 KVM/VMware）**，而 L1 又试图运行一个 **L2（嵌套虚拟机）**。

* **没有 VMCS Shadowing 时**：L1 每次对 L2 的 VMCS 进行修改（执行 `VMREAD` 或 `VMWRITE` 指令）都会导致 L2 产生一个 **VM Exit**，陷入到 L0 中处理。这种频繁的切换会导致巨大的性能损耗。
* **有了 VMCS Shadowing 后**：
	1. L0 会在物理内存中为 L1 准备一个 **Shadow VMCS**。
	2. L0 将这个 Shadow VMCS 的物理地址填入 L1 VMCS 的 `VMCS_LINK_POINTER` 字段中。
	3. **硬件加速**：当 L1 执行 `VMREAD` 或 `VMWRITE` 来操作 L2 的控制结构时，CPU 硬件会直接重定向到 `VMCS_LINK_POINTER` 指向的那个 Shadow VMCS。
	4. **结果**：L1 的这些操作变成了硬件直接完成，**不再产生 VM Exit**，极大地提升了嵌套虚拟化的效率。

要让 `VMCS_LINK_POINTER` 真正发挥加速作用，必须满足以下条件：
1. **启用 VMCS Shadowing**：在“次级处理器执行控制”（Secondary Processor-Based VM-Execution Controls）位图中，将“VMCS shadowing”位（第 7 位）设为 1。
2. **配置位图**：L0 需要配置 **VMREAD Bitmap** 和 **VMWRITE Bitmap**。这些位图决定了哪些 VMCS 字段允许 L1 直接访问，哪些仍然需要拦截（VM Exit）。

### EPTP_LIST_ADDRESS
参考 SDM 26.6.14 VM-Function Controls
文档中的内容和 arch/x86/kvm/vmx/nested.c:handle_vmfunc 完全对的上。

原来这个是切换 vmfunc 的，他的作用是当 L1 存在多个虚拟机，然后来切换的他们的 ept 的

这个功能暂时没有启用。


## 4.19 内核基本不能运行嵌套，启动 Linux 也会有问题的

无论 l1 和 l2 的 kernel 是什么版本:
```txt
[  293.582505] device vnet8 entered promiscuous mode
[  617.916939] general protection fault: 0000 [#1] SMP NOPTI
[  618.003286] RIP: 0010:memchr_inv+0xa8/0xf0
[  619.147776] Code: 00 41 29 c1 48 89 f8 4e 8d 04 0f 3a 08 75 52 48 83 c0 01 4c 39 c0 75 f3 4c 29 ca 4c 89 c7 e9 72 ff ff ff 89 d0 48 85 d2 74 ce <40> 3a 37 75 19 83 e8 01 48 8d 44 07 01 eb 04 3a 0f 75 0b 48 83 c7
[  619.302184] RSP: 0018:ffffb02518d1fe50 EFLAGS: 00010002
[  619.370062] RAX: 000000000000000d RBX: ffff22fcbfb96000 RCX: 0000000000000000
[  619.437488] RDX: 000000000000000d RSI: 0000000000000000 RDI: ffff22fcbfb9604f
[  619.516323] RBP: ffff95853ffd6000 R08: 0000000000000000 R09: 000000010004d15e
[  619.637501] R10: 0000000000100000 R11: 0000000000000001 R12: 000000000000007b
[  619.708291] R13: ffff9644bfbd6780 R14: 0000000000000000 R15: 0000000000000000
[  619.773560] FS:  00007fcd245d6700(0000) GS:ffff9644bfbc0000(0000) knlGS:0000000000000000
[  619.841426] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[  619.908507] CR2: 00000000ffffffff CR3: 000000604bc32000 CR4: 00000000003406e0
[  619.978550] Call Trace:
[  620.092816]  need_update+0x3f/0x80
[  620.194711]  quiet_vmstat+0x31/0x3c
[  620.265897]  tick_nohz_stop_tick+0x59/0x1b0
[  620.365336]  tick_nohz_idle_stop_tick+0x5e/0xc0
[  620.434981]  do_idle+0x1c9/0x250
[  620.548172]  cpu_startup_entry+0x6f/0x80
[  620.675124]  start_secondary+0x18d/0x1e0
[  620.821424]  secondary_startup_64+0xb6/0xc0
```

## 为了 nested ，都做了什么工作
1. iommu

```c
	.domain_alloc_user	= intel_iommu_domain_alloc_user,
```
intel 的支持在:
https://lore.kernel.org/all/20230928071528.26258-1-yi.l.liu@intel.com/

2. kvm-clock ?
3. iommu
4. posted interrupt

## 有趣的工作
https://news.ycombinator.com/item?id=46997133

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
