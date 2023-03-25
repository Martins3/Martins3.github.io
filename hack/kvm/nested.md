# nest
- [ ] https://www.linux-kvm.org/images/3/33/02x03-NestedVirtualization.pdf : ppt
   - https://www.usenix.org/legacy/events/osdi10/tech/full_papers/Ben-Yehuda.pdf : 文章
   - https://www.usenix.org/conference/osdi10/turtles-project-design-and-implementation-nested-virtualization : 视频

Operating system hypervisors (Linux/KVM, WinXP mode in newer versions of Windows)
- Cloud Computing – Give users the ability to run their own hypervisors!
- Security – Mcafee DeepSafe
- Testing/debugging hypervisors
- Interoperability


- [ ] https://www.kernel.org/doc/html/latest/virt/kvm/nested-vmx.html
- [ ] https://www.kernel.org/doc/html/latest/virt/kvm/running-nested-guests.html

[Improving KVM nVMX](https://www.linux-kvm.org/images/8/8e/Improving_KVM_nVMX.pdf)

- [ ] eVMCS 是做什么的 ?
- [ ] 那些 vmexit 让 L0 处理，那些让 L1 处理 ?
    - [ ] 是靠 inject 一个 vmexit 信号过去吗 ?
    - [ ] 如果 Emulate L2→L1

Basic KVM event-injection
1. L1 RDMSR bad_msr
2. RDMSR exits to L0
3. L0 emulates RDMSR and queues #GP:
    - Save pending exception in struct kvm_vcpu_arch
    - Set KVM_REQ_EVENT
4. Before host entry to guest, KVM_REQ_EVENT evaluates queued events:
    1. Inject pending #GP to guest by VMCS
    2. *If (vCPU in guest-mode && Has event which should L2→L1) ⇒ Emulate L2→L1*

- [ ] event injection 的模拟 : 在没有 nested 的时候，需要靠 event injection 告知
  - [ ] kvm_inject_page_fault : 居然存在 async page fault
      - [ ] kvm_queue_exception_e
        - [ ] kvm_multiple_exception
          - [ ] kvm_make_request : **各种地方调用 : step 1**
  - [ ] vcpu_enter_guest **在此处检查 : step 2**
    - [ ] kvm_check_request && inject_pending_event
      - [x] kvm_x86_ops.set_nmi(vcpu); 和 kvm_x86_ops.set_irq(vcpu); 来注入，vmx 注册函数最后会写入 vmcs 来实现
        - `kvm_x86_ops.nested_ops->check_events(vcpu);` : *Call check_nested_events() even if we reinjected a previous event in order for caller to determine if it should require immediate-exit from L2 to L1 due to pending L1 events which require exit from L2 to L1.*
        - [ ] vmx_check_nested_events : 当在 nested mode 的时候才会调用
            - [ ] nested_vmx_check_exception : Has event which should L2→L1
		        - [ ] nested_vmx_inject_exception_vmexit(vcpu, exit_qual);
                - [ ] nested_vmx_vmexit : 其实大多数事情利用 L0 处理 L1 的 vm exit handler 都是可以处理的，但是部分事情需要 L1 的处理，所以会退到 L1, 让 L1 辅助处理
                    - [ ] nested_vmx_vmexit 完成的事情 : 将 vmcs 切换为管理 L1 的 vmcs，然后一些事情（是什么事情 ？）的处理，进入到 L1 中间
                    - sync_vmcs02_to_vmcs12 : 准备基本内容
                    - 比如利用 prepare_vmcs12 将 event (比如 page fault ) 写入到 vmcs12 中间
                      - [ ] 但是注入的消息，L1 如何读去 vmcs12 中间的 `vmcs12->vm_exit_reason` `vmcs12->exit_qualification` `vmcs12->vm_exit_intr_info`
                        - 应该是利用指令 VMREAD/VMWRITE/VMCLEAR/VMPTRLD 访问，然后从guest L1 中间弹出来，最后进入到 L1 中间，L1 检查到的原因就是这些
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
- [ ] 是什么是 SMM ?
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
  - [ ] nested_vmx_handle_enlightened_vmptrld :  This is an equivalent of the nested hypervisor executing the vmptrld instruction.
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
  - [ ] vmx_switch_vmcs : *迷惑*


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

## event injection
- [ ] vcpu_vmx::idt_vectoring_info; 在普通模式下的
- [ ] `vmcs12->idt_vectoring_info_field` : 在 nested 模式下

- [ ] 在 vmcs 中间，存在 IDT-vectoring information
  - [ ] 文档:
    - [ ] 24.9.3 Information for VM Exits That Occur During Event Delivery
    - [ ] 26.5 EVENT INJECTION
    - [ ] 27.2.3 Information for VM Exits During Event Delivery


[^1] 中间指出，在处理一个 event delivery 的时候，结果造成了另一个 vmexit, 那么这个 vmexit 出来的时候，在 vmcs 中间的 idt_vectoring_info 就会存储下来当时正在 deliver 的 event，具体描述如下:
> - A VMExit can occur during event-delivery
> - Example: Write of exception frame to stack triggers `EPT_VIOLATION`
> - CPU saves the event which attempted to deliver in `vmcs->idt_vectoring_info`
> - On `guest->host`:
>   1. KVM checks if `vmcs->idt_vectoring_info valid`
>   2. If valid, queue injected event in `struct kvm_vcpu_arch` and set `KVM_REQ_EVENT`
> - `KVM_REQ_EVENT` will evaluate injected event on next entry to guest

huxueshi : 其实我不是很懂，机制是 event deliver 的时候出现 vmexit，到时候从 vmexit 的位置重新进入到 deliver 的 event 对应的 handler 不就完成了，这个操作看来是重新进行一次 handler 执行

- [ ] 忽然想到 : 当 guest 中间出现 page fault, 在使用 ept 的情况下，如果这个 page fault 是 guest page table missing 造成的，会导致 page fault 吗 ?

[^1]:
> What if VMExit occurs during event-delivery to L2?

**TO BE CONTINUE** 先让我理解一下到底什么指的是 event delivery ?
  - [ ] 并且理解一下 Intel 是如何处理 NMI 的 ？

## nest.c 的代码的阅读记录(should be clear)
- [ ] nested_vmx_hardware_setup : 最开始的时候初始化
    - [ ] 在 level one 的时候的注册 exit handler 比此处多很多
    - 在虚拟机中间的 hypervisors 可以通过什么方法知道自己其实是在虚拟机的 ?

## 等待处理的文档
https://www.usenix.org/conference/osdi10/turtles-project-design-and-implementation-nested-virtualization
https://archive.fosdem.org/2018/schedule/event/vai_kvm_on_hyperv/attachments/slides/2200/export/events/attachments/vai_kvm_on_hyperv/slides/2200/slides_fosdem2018_vkuznets.pdf
https://archive.fosdem.org/2019/schedule/event/vai_enlightening_kvm/attachments/slides/2860/export/events/attachments/vai_enlightening_kvm/slides/2860/vkuznets_fosdem2019_enlightening_kvm.pdf

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
