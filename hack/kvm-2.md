--> 调查一下 kvm_main 的功能

## vfio
似乎可以用于 GPU 虚拟化

https://www.kernel.org/doc/html/latest/driver-api/vfio.html
https://zhuanlan.zhihu.com/p/27026590

## eventfd
- [ ] eventfd 的原理重新分析一下
https://stackoverflow.com/questions/17937142/eventfd-role-in-linux

存在更好的通知方式吗 ?

## mmu notifier

kvm_mmu_notifier_invalidate_range_start


#### https://lwn.net/Articles/732952/
This mechanism allows any subsystem to hook into memory-management operations and receive a callback when changes are made to a process's page tables.


https://www.linux-kvm.org/images/3/33/KvmForum2008%24kdf2008_15.pdf
> 用于分析的 PPT


- young 是什么意思 ?

- mmu notifier 机制的想法 是 host 发生修改之后，然后发现其中的含义


kvm_mmu_notifier_clear_young
=> kvm_age_hva
=> kvm_handle_hva_range kvm_age_rmapp

```c
static int kvm_age_rmapp(struct kvm *kvm, struct kvm_rmap_head *rmap_head,
			 struct kvm_memory_slot *slot, gfn_t gfn, int level,
			 unsigned long data)
{
	u64 *sptep;
	struct rmap_iterator iter;
	int young = 0;

	for_each_rmap_spte(rmap_head, &iter, sptep)
		young |= mmu_spte_age(sptep);

	trace_kvm_age_page(gfn, level, slot, young);
	return young;
}
```

暂时的理解是 : host 告诉一个区域的 page 有问题了, 根据 HVA 可以知道 GPA 的物理地址出现问题，
那么需要知道找到对应的 shadow page table 进行处理.

由此可以想象 : 当 host 将 guest 的物理页面 swap 出去，由此可以
从而修改 shadow page table


kvm_arch_mmu_notifier_invalidate_range: 似乎是一个为 APIC 制作的

```c
void kvm_arch_mmu_notifier_invalidate_range(struct kvm *kvm,
					    unsigned long start, unsigned long end)
{
	unsigned long apic_address;

	/*
	 * The physical address of apic access page is stored in the VMCS.
	 * Update it when it becomes invalid.
	 */
	apic_address = gfn_to_hva(kvm, APIC_DEFAULT_PHYS_BASE >> PAGE_SHIFT);
	if (start <= apic_address && apic_address < end)
		kvm_make_all_cpus_request(kvm, KVM_REQ_APIC_PAGE_RELOAD);
}
```
- [ ] 无法理解 kvm_arch_mmu_notifier_invalidate_range 最后是给 KVM_REQ_APIC_PAGE_RELOAD

对应的通知机制是是给 guest 发出通知消息





## kvm_main
kvm_is_zone_device_pfn

- [ ] zone device
#### vcpu_load
- [ ] check 一下使用的位置
- [ ] preempt_notifier_register : 神奇的 notifier 机制
- [ ] 和 vcpu_put 的结合分析一下

```c
/*
 * Switches to specified vcpu, until a matching vcpu_put()
 */
void vcpu_load(struct kvm_vcpu *vcpu)
{
	int cpu = get_cpu();

	__this_cpu_write(kvm_running_vcpu, vcpu);
	preempt_notifier_register(&vcpu->preempt_notifier);
	kvm_arch_vcpu_load(vcpu, cpu);
	put_cpu();
}
```

#### kvm_vm_ioctl_set_memory_region

#### kvm_vcpu_unmap

#### kvm_read_guest
- [ ] 为什么要处理 guest page 机制

#### kvm_vcpu_fault
> 配合 vcpu ioctl 
```c
static int create_vcpu_fd(struct kvm_vcpu *vcpu)
{
	char name[8 + 1 + ITOA_MAX_LEN + 1];

	snprintf(name, sizeof(name), "kvm-vcpu:%d", vcpu->vcpu_id);
	return anon_inode_getfd(name, &kvm_vcpu_fops, vcpu, O_RDWR | O_CLOEXEC);
}
```

#### kvm device ioctl
> TODO

#### kvm io bus write

kvm_io_bus_write => `__kvm_io_bus_write`

```c
struct kvm_io_bus {
	int dev_count;
	int ioeventfd_count;
	struct kvm_io_range range[];
};
```
KVM: Adds support for in-kernel mmio handlers




## remote TLB
https://stackoverflow.com/questions/3748384/what-is-tlb-shootdown
https://stackoverflow.com/questions/50256740/who-performs-the-tlb-shootdown


