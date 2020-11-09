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
- [ ] vmcs02 如何同步到 vmcs01 ?
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

```c
kvm_make_request(KVM_REQ_EVENT, vcpu); // 各种地方调用

vcpu_enter_guest // 在此处检查, 可以 kvm_emulate_wrmsr
```

- [ ] 一个 vcpu 进入 guest mode 意味着什么 ?
  - [x] handle_vmlaunch/nested_vmx_run ==> nested_vmx_run ==>  nested_vmx_enter_non_root_mode ==> enter_guest_mode ==> is_guest_mode


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

- [ ] enlightened 是什么意思?
  - [ ] nested_vmx_handle_enlightened_vmptrld :  This is an equivalent of the nested hypervisor executing the vmptrld instruction.
  - [ ] copy_enlightened_to_vmcs12

- [ ] 对比一下 struct vmcs 和 struct vmcs12 的内容

- [ ] check shadow vmcs 的工作方法 ?
    - copy_shadow_to_vmcs12 : 有些是 writable 的, 

## nest.c 的代码的阅读记录(should be clear)
- [ ] nested_vmx_hardware_setup : 最开始的时候初始化
    - [ ] 在 level one 的时候的注册 exit handler 比此处多很多
    - 在虚拟机中间的 hypervisors 可以通过什么方法知道自己其实是在虚拟机的 ?
