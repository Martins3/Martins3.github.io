# vz 中间的 tlb 管理

关于 tlb 的问题合集可以体现在下面这一个函数中间
```c
static int kvm_vz_vcpu_run(struct kvm_run *run, struct kvm_vcpu *vcpu)
{
	int cpu = smp_processor_id();
	int r;

	kvm_timer_callbacks->acquire_htimer(vcpu);
	/* Check if we have any exceptions/interrupts pending */
	kvm_mips_deliver_interrupts(vcpu, read_gc0_cause());

	kvm_vz_check_requests(vcpu, cpu);
	kvm_vz_vcpu_load_tlb(vcpu, cpu);
	kvm_vz_vcpu_load_wired(vcpu);

	r = vcpu->arch.vcpu_run(run, vcpu);

	kvm_vz_vcpu_save_wired(vcpu);

	return r;
}
```

1. kvm_mips_deliver_interrupts : 关于 remote tlb shot
2. kvm_vz_vcpu_load_tlb : 当 vcpu 在 physical cpu 上迁移之后，需要考虑 guestid 的 invalid
3. kvm_vz_vcpu_load_wired : 当 load 出现

## remote tlb shot : kvm_vz_check_requests

## guestid : kvm_vz_vcpu_load_tlb
- [ ] 两个 load 的关系是什么，部署的位置
  - schedu_in / schedu_out

	.vcpu_load = kvm_vz_vcpu_load,
	.vcpu_put = kvm_vz_vcpu_put,

- [ ] guestid 如果是 guest 的 cp0 寄存器 ?
  - 怎么可能
  - 但是 host 如何写入这个数值 ?

## wired TLB :  kvm_vz_vcpu_save_wired

- [ ] 分别保存的是什么东西 ?
  - [ ] guest_tlb 包含 wired_tlb 的内容吗 ?

	/* wired guest TLB entries */
	struct kvm_mips_tlb *wired_tlb;

	/* S/W Based TLB for guest */
	struct kvm_mips_tlb guest_tlb[KVM_MIPS_GUEST_TLB_SIZE];

- [ ] 被 kvm_vz_vcpu_put 调用
  - kvm_vz_vcpu_put 就是保存一下所有的 cp0 吗 ?


- kvm_arch_vcpu_ioctl_run
  - lose_fpu
  - local_irq_disable
  - guest_enter_irqoff

PF_VCPU 表示当前的进程执行 `r = kvm_mips_callbacks->vcpu_run(run, vcpu);`

- [x] 我无法理解是，如果 PF_VCPU 没有了，为什么还可能执行 kvm_vz_vcpu_put ?
  - kvm_vz_vcpu_put 其实是用于 vcpu 切换 物理 cpu 的时候，在物理 cpu 中间保存的各种数值就需要切换掉
  - [ ] 那么，在 vcpu_run 的中间切换 CPU 会特殊处理啊 ?

- [ ] guest 存在额外的一套 TLB 吗 ?
  - tlbgr 寄存器 ?

- kvm_vz_vcpu_save_wired
  - [ ] kvm_vz_save_guesttlb : 被唯一调用
      - [ ] set_root_gid_to_guest_gid
      - [ ] clear_root_gid


从源代码分析，以前取决于 guest 的 wired 寄存器的数值, 只要初始化的时候，将 wired 处理掉，那么就可以了
```
	unsigned int wired = read_gc0_wired();
```
