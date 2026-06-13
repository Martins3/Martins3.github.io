# [KVM Lock Overview](https://docs.kernel.org/virt/kvm/locking.html)

### cpus_read_lock
```txt
@[
    cpus_read_lock+5
    static_key_enable+18
    mem_cgroup_css_online+182
    cgroup_apply_control_enable+863
    cgroup_mkdir+1083
    kernfs_iop_mkdir+101
    vfs_mkdir+379
    do_mkdirat+152
    __x64_sys_mkdir+43
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    cpus_read_lock+5
    static_key_slow_inc+18
    mem_cgroup_css_alloc+1310
    cgroup_apply_control_enable+281
    cgroup_mkdir+1083
    kernfs_iop_mkdir+101
    vfs_mkdir+379
    do_mkdirat+152
    __x64_sys_mkdir+43
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    cpus_read_lock+5
    static_key_slow_inc+18
    mem_cgroup_css_alloc+1281
    cgroup_apply_control_enable+281
    cgroup_mkdir+1083
    kernfs_iop_mkdir+101
    vfs_mkdir+379
    do_mkdirat+152
    __x64_sys_mkdir+43
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    cpus_read_lock+5
    jump_label_update_timeout+22
    process_scheduled_works+444
    worker_thread+712
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 2
@[
    cpus_read_lock+5
    kvm_dev_ioctl+1214
    __se_sys_ioctl+115
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 2
@[
    cpus_read_lock+5
    static_key_slow_inc+18
    kvm_dev_ioctl+2315
    __se_sys_ioctl+115
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 2
@[
    cpus_read_lock+5
    cgroup_procs_write_start+109
    __cgroup_procs_write+87
    cgroup_procs_write+23
    kernfs_fop_write_iter+240
    vfs_write+892
    ksys_write+114
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 2
@[
    cpus_read_lock+5
    static_key_slow_inc+18
    cgroup_bpf_attach+1210
    cgroup_bpf_prog_attach+146
    bpf_prog_attach+446
    __sys_bpf+417
    __x64_sys_bpf+28
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 2
@[
    cpus_read_lock+5
    cgroup_attach_lock+17
    cgroup_attach_task_all+47
    kvm_vm_worker_thread+70
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 2
@[
    cpus_read_lock+5
    static_key_disable+18
    once_deferred+30
    process_scheduled_works+444
    worker_thread+712
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 2
@[
    cpus_read_lock+5
    static_key_slow_inc+18
    kvm_lapic_reg_write+752
    vmx_set_msr+2684
    __kvm_set_msr+183
    kvm_emulate_wrmsr+84
    vmx_handle_exit+1205
    kvm_arch_vcpu_ioctl_run+6873
    kvm_vcpu_ioctl+1537
    __se_sys_ioctl+115
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 2
@[
    cpus_read_lock+5
    static_key_slow_inc+18
    kvm_create_lapic+239
    kvm_arch_vcpu_create+116
    kvm_vm_ioctl_create_vcpu+348
    kvm_vm_ioctl+1770
    __se_sys_ioctl+115
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 6
@[
    cpus_read_lock+5
    vmstat_shepherd+19
    process_scheduled_works+444
    worker_thread+712
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 9
```

## kvm_lock
```c
/*
 * Ordering of locks:
 *
 *	kvm->lock --> kvm->slots_lock --> kvm->irq_lock
 */

DEFINE_MUTEX(kvm_lock);
```

## 好多文档都没有记录

```txt
	struct srcu_struct srcu;
	struct srcu_struct irq_srcu;
```

## 如何保护 memslot 的

```rst
``kvm->srcu``
^^^^^^^^^^^^^
:Type:		srcu lock
:Arch:		any
:Protects:	- kvm->memslots
		- kvm->buses
:Comment:	The srcu read lock must be held while accessing memslots (e.g.
		when using gfn_to_* functions) and while accessing in-kernel
		MMIO/PIO address->device structure mapping (kvm->buses).
		The srcu index can be stored in kvm_vcpu->srcu_idx per vcpu
		if it is needed by multiple functions.
```

例如
```c
/*
 * Called within kvm->srcu read side.
 * Returns 1 to let vcpu_run() continue the guest execution loop without
 * exiting to the userspace.  Otherwise, the value will be returned to the
 * userspace.
 */
static int vcpu_enter_guest(struct kvm_vcpu *vcpu)
```

vcpu_enter_guest 还同时被 vcpu rscu 保护的，具体在 kvm_arch_vcpu_ioctl_run 中

```txt
kvm_vcpu_srcu_read_lock(vcpu);

kvm_vcpu_srcu_read_unlock(vcpu);
```

这个东西的实现，看似是
```c
static inline void kvm_vcpu_srcu_read_lock(struct kvm_vcpu *vcpu)
{
#ifdef CONFIG_PROVE_RCU
	WARN_ONCE(vcpu->srcu_depth++,
		  "KVM: Illegal vCPU srcu_idx LOCK, depth=%d", vcpu->srcu_depth - 1);
#endif
	vcpu->____srcu_idx = srcu_read_lock(&vcpu->kvm->srcu);
}
```

```txt
	synchronize_srcu(&kvm->srcu);
```

更多时候:
```txt
		idx = srcu_read_lock(&vcpu->kvm->srcu);
```

如果一直在运行，会有导致始终无法释放吗? 就是这个 CPU 总是运行的状态

具体的问题在: kvm_set_memslot

## page fault
mm/shmem.c:shmem_fault

启动 qemu 测试，memory backend 是 memfd ，结果为:
```txt

[52966.163244] 3 locks held by qemu-system-x86/5090:
[52966.163796]  #0: ffff888127c000b0 (&vcpu->mutex){+.+.}-{4:4}, at: kvm_vcpu_ioctl+0x96/0xa70 [kvm]
[52966.164979]  #1: ffffc900023e6ef0 (&kvm->srcu){.+.+}-{0:0}, at: kvm_arch_vcpu_ioctl_run+0x1379/0x2450 [kvm]
[52966.166577]  #2: ffff88810864ea50 (&mm->mmap_lock){++++}-{4:4}, at: get_user_pages_unlocked+0x88/0x360
```

1. kvm_vcpu_ioctl
```txt
	if (mutex_lock_killable(&vcpu->mutex))
```

2. kvm_arch_vcpu_ioctl_run
```txt
	kvm_vcpu_srcu_read_lock(vcpu);
```

3. get_user_pages_unlocked -> __get_user_pages_locked

```txt
	if (!*locked) {
		if (mmap_read_lock_killable(mm))
			return -EAGAIN;
		must_unlock = true;
		*locked = 1;
	}
```

## kvm_tdp 到底解决了什么锁

既然是一个 option ，显然可以测试出来这个东西

## kvm 为什么需要 srcu 来着?
```txt
		kvm_vcpu_srcu_read_unlock(vcpu);
		if (vcpu->arch.mp_state == KVM_MP_STATE_HALTED)
			kvm_vcpu_haltevcpu);
		else
			kvm_vcpu_block(vcpu);
		kvm_vcpu_srcu_read_lock(vcpu);
```

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
