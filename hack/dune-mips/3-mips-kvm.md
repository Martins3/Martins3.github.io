# kvm

## memslot
how will dune register memslot for it ?

## load_cpu
- 为什么将同步信息放到 vcpu 中间没有用 ?


- 其实，vcpu_load 只是将关闭 preempt 之后，然后马上就打开了，
```c

/*
 * Switches to specified vcpu, until a matching vcpu_put()
 */
void vcpu_load(struct kvm_vcpu *vcpu)
{
	int cpu = get_cpu();
	preempt_notifier_register(&vcpu->preempt_notifier);
	kvm_arch_vcpu_load(vcpu, cpu);
	put_cpu();
}
```


## What happends to tlb.c

## How mmu notifier works

## Please analyze the complex arch_kvm_cpu , arch_kvm, kvm_run and ...

## some code route has been analyzed

## asid, machine id, tpid

## misc
