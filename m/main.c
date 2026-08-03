#include "internal.h"
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/version.h>

#define MODULE_NAME "lab"
MODULE_AUTHOR("Martins3");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simple kernel module test kernel api");
MODULE_VERSION("0.1");

static char *name = "martins3";
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name, "The name to display in /var/log/messages");

/*
 * 演示一个模块参数
 */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 15, 0))
#define NVME_PCI_MIN_QUEUE_SIZE 2
#define NVME_PCI_MAX_QUEUE_SIZE 4095
static int io_queue_depth_set(const char *val, const struct kernel_param *kp)
{
	dump_stack();
	return param_set_uint_minmax(val, kp, NVME_PCI_MIN_QUEUE_SIZE,
				     NVME_PCI_MAX_QUEUE_SIZE);
}

static const struct kernel_param_ops io_queue_depth_ops = {
	.set = io_queue_depth_set,
	.get = param_get_uint,
};
static unsigned int io_queue_depth = 1024;
module_param_cb(io_queue_depth, &io_queue_depth_ops, &io_queue_depth, 0644);
MODULE_PARM_DESC(io_queue_depth, "set io queue depth, should >= 2 and < 4096");
#endif

static int test = 0;
module_param_named(init, test, int, 0644);

// TODO 考虑下有没有更好的方法来添加到测试
// 使用 sysctl 也许很容易，但是 sysctl 的语义不是正确的
static ulong para[8];
module_param_named(para0, para[0], ulong, 0644);
module_param_named(para1, para[1], ulong, 0644);
ulong get_parameter(uint n)
{
	BUG_ON(n >= 8);
	return para[n];
}

int batch_queue_works(work_func_t func, unsigned workers, size_t size)
{
	void *works;
	size_t i;
	struct work *work;
	if (size < sizeof(struct work))
		return -EINVAL;
	works = kvmalloc_array(workers, size, GFP_KERNEL);
	if (!works)
		return -ENOMEM;
	for (i = 0; i < workers; i++) {
		work = works + size * i;
		INIT_WORK(&work->work, func);
		work->id = i;
		if (!queue_work(system_unbound_wq, &work->work))
			return -ENAVAIL;
	}
	for (i = 0; i < workers; i++) {
		work = works + size * i;
		flush_work(&work->work);
	}
	kvfree(works);
	return 0;
}

// 记录一个有趣的问题，如果添加一个 asmlinkage 的测试项目
// 那么模块可以正常加载，但是  ls -la /sys/kernel/hacking 会有 io error
#ifdef CONFIG_TEST_MEMORY_MODEL
DEFINE_TESTER(memory_model)
#endif
#ifdef CONFIG_TEST_RT_MUTEX
DEFINE_TESTER(rt_mutex)
#endif
#ifdef CONFIG_TEST_SPINLOCK
DEFINE_TESTER(spinlock)
#endif
#ifdef CONFIG_TEST_RCUREF
DEFINE_TESTER(rcuref)
#endif
#ifdef CONFIG_TEST_REFCOUNT
DEFINE_TESTER(refcount)
#endif
#ifdef CONFIG_TEST_MODIFIER
DEFINE_TESTER(modifier)
#endif
#ifdef CONFIG_TEST_SAN
DEFINE_TESTER(san)
#endif
#ifdef CONFIG_TEST_FOLIO_QUEUE
DEFINE_TESTER(folio_queue)
#endif
#ifdef CONFIG_TEST_HRTIMER
DEFINE_TESTER(hrtimer)
#endif
#ifdef CONFIG_TEST_SIGNAL
DEFINE_TESTER(signal)
#endif
#ifdef CONFIG_TEST_SOFTIRQ
DEFINE_TESTER(softirq)
#endif
#ifdef CONFIG_TEST_EXCEPTION
DEFINE_TESTER(exception)
#endif
#ifdef CONFIG_TEST_PMMIR
	DEFINE_TESTER(pmmir)
#endif
#ifdef CONFIG_TEST_ANON_INODE
	DEFINE_TESTER(anon_inode)
#endif
#ifdef CONFIG_TEST_RADIX_TREE
	DEFINE_TESTER(radix_tree)
#endif
// start of definition
#ifdef CONFIG_TEST_LOCKDEP
DEFINE_TESTER_RESOURCE(lockdep)
#endif
#ifdef CONFIG_TEST_TRACEPOINT
DEFINE_TESTER_RESOURCE(tracepoint)
#endif
#ifdef CONFIG_TEST_GENETLINK
DEFINE_TESTER_RESOURCE(genetlink)
#endif
#ifdef CONFIG_TEST_SCHED_DEBUG
DEFINE_TESTER(sched_debug)
#endif
#ifdef CONFIG_TEST_NETLINK
DEFINE_TESTER_RESOURCE(netlink)
#endif
#ifdef CONFIG_TEST_STATIC_KEY
DEFINE_TESTER(static_key)
#endif
#ifdef CONFIG_TEST_SLUB
DEFINE_TESTER(slub)
#endif
#ifdef CONFIG_TEST_PCI
DEFINE_TESTER(pci)
#endif
#ifdef CONFIG_TEST_PID
DEFINE_TESTER(pid)
#endif
#ifdef CONFIG_TEST_TIMER
DEFINE_TESTER_RESOURCE(timer)
#endif
#ifdef CONFIG_TEST_CLOCK
DEFINE_TESTER_RESOURCE(clock)
#endif
#ifdef CONFIG_TEST_DEV
DEFINE_TESTER_RESOURCE(dev)
#endif
#ifdef CONFIG_TEST_SYSCTL
DEFINE_TESTER_RESOURCE(sysctl)
#endif
#ifdef CONFIG_TEST_KOBJECT
DEFINE_TESTER_RESOURCE(kobject)
#endif
#ifdef CONFIG_TEST_TIF
DEFINE_TESTER(tif)
#endif
#ifdef CONFIG_TEST_PERCPU_REF
DEFINE_TESTER(percpu_ref)
#endif
#ifdef CONFIG_TEST_RCUSTALL
DEFINE_TESTER(rcustall)
#endif
#ifdef CONFIG_TEST_MODULE
DEFINE_TESTER(module)
#endif
#ifdef CONFIG_TEST_PERCPU
DEFINE_TESTER_RESOURCE(percpu)
#endif
#ifdef CONFIG_TEST_CGROUP
DEFINE_TESTER(cgroup)
#endif
#ifdef CONFIG_TEST_BIO
DEFINE_TESTER(bio)
#endif
#ifdef CONFIG_TEST_GUP
DEFINE_TESTER(gup)
#endif
#ifdef CONFIG_TEST_SG
DEFINE_TESTER(sg)
#endif
#ifdef CONFIG_TEST_BITOPS
DEFINE_TESTER(bitops)
#endif
#ifdef CONFIG_TEST_RNG
DEFINE_TESTER(rng)
#endif
#ifdef CONFIG_TEST_MAPLE_TREE
DEFINE_TESTER(maple_tree)
#endif
#ifdef CONFIG_TEST_RBTREE
DEFINE_TESTER(rbtree)
#endif
#ifdef CONFIG_TEST_XARRAY
DEFINE_TESTER(xarray)
#endif
#ifdef CONFIG_TEST_ZSTD
DEFINE_TESTER(zstd)
#endif
#ifdef CONFIG_TEST_BITMAP_TEST
DEFINE_TESTER(bitmap_test)
#endif
#ifdef CONFIG_TEST_SBITMAP_TEST
DEFINE_TESTER(sbitmap_test)
#endif
#ifdef CONFIG_TEST_LRU
DEFINE_TESTER_RESOURCE(lru)
#endif
#ifdef CONFIG_TEST_MM_WALK
DEFINE_TESTER(mm_walk)
#endif
#ifdef CONFIG_TEST_MAPPING
DEFINE_TESTER_RESOURCE(mapping)
#endif
#ifdef CONFIG_TEST_MMAP_LOCK
DEFINE_TESTER_RESOURCE(mmap_lock)
#endif
#ifdef CONFIG_TEST_LRU_LOCK
DEFINE_TESTER_RESOURCE(lru_lock)
#endif
#ifdef CONFIG_TEST_FOLIO_LOCK
DEFINE_TESTER_RESOURCE(folio_lock)
#endif
#ifdef CONFIG_TEST_MM_MISC
DEFINE_TESTER(mm_misc)
#endif
#ifdef CONFIG_TEST_STACK
DEFINE_TESTER(stack)
#endif
#ifdef CONFIG_TEST_VMALLOC
DEFINE_TESTER(vmalloc)
#endif
#ifdef CONFIG_TEST_PRIVATE
DEFINE_TESTER(private)
#endif
#ifdef CONFIG_TEST_PAGEFLAG
DEFINE_TESTER(pageflag)
#endif
#ifdef CONFIG_TEST_MMU_NOTIFIER
DEFINE_TESTER_RESOURCE(mmu_notifier)
#endif
#ifdef CONFIG_TEST_SHARE
DEFINE_TESTER_RESOURCE(share)
#endif
#ifdef CONFIG_TEST_FOLIO
DEFINE_TESTER_RESOURCE(folio)
#endif
#ifdef CONFIG_TEST_API_DIS
DEFINE_TESTER(api_dis)
#endif
#ifdef CONFIG_TEST_SOCKET_CLIENT
DEFINE_TESTER_RESOURCE(socket_client)
#endif
#ifdef CONFIG_TEST_SEQ
DEFINE_TESTER_RESOURCE(seq)
#endif
#ifdef CONFIG_TEST_TASKLET
DEFINE_TESTER_RESOURCE(tasklet)
#endif
#ifdef CONFIG_TEST_BTREE
DEFINE_TESTER(btree)
#endif
#ifdef CONFIG_TEST_PRINTK
DEFINE_TESTER(printk)
#endif
#ifdef CONFIG_X86_64
#ifdef CONFIG_TEST_ASM
DEFINE_TESTER_RESOURCE(asm)
#endif
#ifdef CONFIG_TEST_X86_MISC
DEFINE_TESTER_RESOURCE(x86_misc)
#endif
#ifdef CONFIG_TEST_SMM
DEFINE_TESTER(smm)
#endif
#ifdef CONFIG_TEST_TSC
DEFINE_TESTER(tsc)
#endif
#ifdef CONFIG_TEST_EVENT_DELIVERY
DEFINE_TESTER(event_delivery)
#endif
#ifdef CONFIG_TEST_MSR_MMIO
DEFINE_TESTER(msr_mmio)
#endif
#ifdef CONFIG_TEST_TSX
DEFINE_TESTER(tsx)
#endif
#ifdef CONFIG_TEST_X2APIC
DEFINE_TESTER(x2apic)
#endif
#ifdef CONFIG_TEST_APIC
DEFINE_TESTER(apic)
#endif
#ifdef CONFIG_TEST_SUBERROR
DEFINE_TESTER(suberror)
#endif
#endif
#ifdef CONFIG_ARM64
#ifdef CONFIG_TEST_SYSREG
DEFINE_TESTER(sysreg)
#endif
#ifdef CONFIG_TEST_AARCH64
DEFINE_TESTER_RESOURCE(aarch64)
#endif
#endif

#ifdef CONFIG_TEST_HLIST
DEFINE_TESTER(hlist)
#endif
#ifdef CONFIG_TEST_IDR
DEFINE_TESTER(idr)
#endif
#ifdef CONFIG_TEST_MUTEX
DEFINE_TESTER(mutex)
#endif
#ifdef CONFIG_TEST_SRCU
DEFINE_TESTER(srcu)
#endif
#ifdef CONFIG_TEST_WAIT
DEFINE_TESTER(wait)
#endif
#ifdef CONFIG_TEST_ATOMIC
DEFINE_TESTER(atomic)
#endif
#ifdef CONFIG_TEST_IO_WAIT
DEFINE_TESTER(io_wait)
#endif
#ifdef CONFIG_TEST_RWSEM
DEFINE_TESTER(rwsem)
#endif
#ifdef CONFIG_TEST_COMPLETE
DEFINE_TESTER(complete)
#endif
#ifdef CONFIG_TEST_PERCPU_RWSEM
DEFINE_TESTER(percpu_rwsem)
#endif
#ifdef CONFIG_TEST_WORKQUEUE
DEFINE_TESTER_RESOURCE(workqueue)
#endif
#ifdef CONFIG_TEST_WAITBIT
DEFINE_TESTER(waitbit)
#endif
#ifdef CONFIG_TEST_RCUWAIT
DEFINE_TESTER(rcuwait)
#endif
#ifdef CONFIG_TEST_ACCESS_ONCE
DEFINE_TESTER(access_once)
#endif
#ifdef CONFIG_TEST_SEQLOCK
DEFINE_TESTER(seqlock)
#endif
#ifdef CONFIG_TEST_IRQWORK
DEFINE_TESTER(irqwork)
#endif
#ifdef CONFIG_TEST_IPI
DEFINE_TESTER(ipi)
#endif
#ifdef CONFIG_TEST_EVENTFD
DEFINE_TESTER(eventfd)
#endif
#ifdef CONFIG_TEST_EPOLL
DEFINE_TESTER_RESOURCE(epoll)
#endif
#ifdef CONFIG_TEST_PROCESS_STATE
DEFINE_TESTER(process_state)
#endif
#ifdef CONFIG_TEST_SCHED
DEFINE_TESTER(sched)
#endif
#ifdef CONFIG_TEST_GUARD
DEFINE_TESTER(guard)
#endif
#ifdef CONFIG_TEST_PERCPU_COUNTER
DEFINE_TESTER_RESOURCE(percpu_counter)
#endif
#ifdef CONFIG_TEST_DEBUGFS
DEFINE_TESTER(debugfs)
#endif
#ifdef CONFIG_TEST_MISC
DEFINE_TESTER_RESOURCE(misc)
#endif
#ifdef CONFIG_TEST_PROC
DEFINE_TESTER(proc)
#endif
#ifdef CONFIG_TEST_NIC
DEFINE_TESTER(nic)
#endif
#ifdef CONFIG_TEST_AIO
DEFINE_TESTER(aio)
#endif
#ifdef CONFIG_TEST_IOURING
DEFINE_TESTER(iouring)
#endif
#ifdef CONFIG_TEST_TASK_WORK
DEFINE_TESTER(task_work)
#endif
#ifdef CONFIG_TEST_UACCESS
DEFINE_TESTER(uaccess)
#endif
#ifdef CONFIG_TEST_KTHREAD
DEFINE_TESTER_RESOURCE(kthread)
#endif
#ifdef CONFIG_TEST_NOTIFIER
DEFINE_TESTER(notifier)
#endif
#ifdef CONFIG_TEST_RCUPDATE
DEFINE_TESTER_RESOURCE(rcupdate)
#endif
#ifdef CONFIG_TEST_RCULIST
DEFINE_TESTER(rculist)
#endif
#ifdef CONFIG_TEST_JIFFIES
DEFINE_TESTER(jiffies)
#endif
#ifdef CONFIG_TEST_SWAIT
DEFINE_TESTER_RESOURCE(swait)
#endif
#ifdef CONFIG_TEST_MM_SL
DEFINE_TESTER_RESOURCE(mm_sl)
#endif
#ifdef CONFIG_TEST_MM_LL
DEFINE_TESTER_RESOURCE(mm_ll)
#endif
#ifdef CONFIG_TEST_MM_SS
DEFINE_TESTER_RESOURCE(mm_ss)
#endif
#ifdef CONFIG_TEST_MM_LS
DEFINE_TESTER_RESOURCE(mm_ls)
#endif

/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */
static struct attribute *attrs[] = {
#ifdef CONFIG_TEST_RADIX_TREE
	&radix_tree_attribute.attr,
#endif
#ifdef CONFIG_TEST_ANON_INODE
	&anon_inode_attribute.attr,
#endif
#ifdef CONFIG_TEST_PMMIR
	&pmmir_attribute.attr,
#endif
#ifdef CONFIG_TEST_EXCEPTION
	&exception_attribute.attr,
#endif
#ifdef CONFIG_TEST_SOFTIRQ
	&softirq_attribute.attr,
#endif
#ifdef CONFIG_TEST_SIGNAL
	&signal_attribute.attr,
#endif
#ifdef CONFIG_TEST_HRTIMER
	&hrtimer_attribute.attr,
#endif
#ifdef CONFIG_TEST_FOLIO_QUEUE
	&folio_queue_attribute.attr,
#endif
#ifdef CONFIG_TEST_SAN
	&san_attribute.attr,
#endif
#ifdef CONFIG_TEST_MODIFIER
	&modifier_attribute.attr,
#endif
#ifdef CONFIG_TEST_REFCOUNT
	&refcount_attribute.attr,
#endif
#ifdef CONFIG_TEST_RCUREF
	&rcuref_attribute.attr,
#endif
#ifdef CONFIG_TEST_SPINLOCK
	&spinlock_attribute.attr,
#endif
#ifdef CONFIG_TEST_RT_MUTEX
	&rt_mutex_attribute.attr,
#endif
#ifdef CONFIG_TEST_MEMORY_MODEL
	&memory_model_attribute.attr,
#endif
#ifdef CONFIG_TEST_TRACEPOINT
	&tracepoint_attribute.attr,
#endif
#ifdef CONFIG_TEST_LOCKDEP
	&lockdep_attribute.attr,
#endif
#ifdef CONFIG_TEST_GENETLINK
	&genetlink_attribute.attr,
#endif
#ifdef CONFIG_TEST_NETLINK
	&netlink_attribute.attr,
#endif
#ifdef CONFIG_TEST_STATIC_KEY
	&static_key_attribute.attr,
#endif
#ifdef CONFIG_TEST_SLUB
	&slub_attribute.attr,
#endif
#ifdef CONFIG_TEST_PCI
	&pci_attribute.attr,
#endif
#ifdef CONFIG_TEST_PID
	&pid_attribute.attr,
#endif
#ifdef CONFIG_TEST_TIMER
	&timer_attribute.attr,
#endif
#ifdef CONFIG_TEST_CLOCK
	&clock_attribute.attr,
#endif
#ifdef CONFIG_TEST_DEV
	&dev_attribute.attr,
#endif
#ifdef CONFIG_TEST_HLIST
	&hlist_attribute.attr,
#endif
#ifdef CONFIG_TEST_SYSCTL
	&sysctl_attribute.attr,
#endif
#ifdef CONFIG_TEST_KOBJECT
	&kobject_attribute.attr,
#endif
#ifdef CONFIG_TEST_TIF
	&tif_attribute.attr,
#endif
#ifdef CONFIG_TEST_PERCPU_REF
	&percpu_ref_attribute.attr,
#endif
#ifdef CONFIG_TEST_RCUSTALL
	&rcustall_attribute.attr,
#endif
#ifdef CONFIG_TEST_MODULE
	&module_attribute.attr,
#endif
#ifdef CONFIG_TEST_CGROUP
	&cgroup_attribute.attr,
#endif
#ifdef CONFIG_TEST_BIO
	&bio_attribute.attr,
#endif
#ifdef CONFIG_TEST_GUP
	&gup_attribute.attr,
#endif
#ifdef CONFIG_TEST_SG
	&sg_attribute.attr,
#endif
#ifdef CONFIG_TEST_BITOPS
	&bitops_attribute.attr,
#endif
#ifdef CONFIG_TEST_RNG
	&rng_attribute.attr,
#endif
#ifdef CONFIG_TEST_MAPLE_TREE
	&maple_tree_attribute.attr,
#endif
#ifdef CONFIG_TEST_RBTREE
	&rbtree_attribute.attr,
#endif
#ifdef CONFIG_TEST_XARRAY
	&xarray_attribute.attr,
#endif
#ifdef CONFIG_TEST_ZSTD
	&zstd_attribute.attr,
#endif
#ifdef CONFIG_TEST_BITMAP_TEST
	&bitmap_test_attribute.attr,
#endif
#ifdef CONFIG_TEST_SBITMAP_TEST
	&sbitmap_test_attribute.attr,
#endif
#ifdef CONFIG_TEST_LRU
	&lru_attribute.attr,
#endif
#ifdef CONFIG_TEST_MM_WALK
	&mm_walk_attribute.attr,
#endif
#ifdef CONFIG_TEST_MAPPING
	&mapping_attribute.attr,
#endif
#ifdef CONFIG_TEST_MMAP_LOCK
	&mmap_lock_attribute.attr,
#endif
#ifdef CONFIG_TEST_LRU_LOCK
	&lru_lock_attribute.attr,
#endif
#ifdef CONFIG_TEST_FOLIO_LOCK
	&folio_lock_attribute.attr,
#endif
#ifdef CONFIG_TEST_MM_MISC
	&mm_misc_attribute.attr,
#endif
#ifdef CONFIG_TEST_STACK
	&stack_attribute.attr,
#endif
#ifdef CONFIG_TEST_VMALLOC
	&vmalloc_attribute.attr,
#endif
#ifdef CONFIG_TEST_PRIVATE
	&private_attribute.attr,
#endif
#ifdef CONFIG_TEST_PAGEFLAG
	&pageflag_attribute.attr,
#endif
#ifdef CONFIG_TEST_MMU_NOTIFIER
	&mmu_notifier_attribute.attr,
#endif
#ifdef CONFIG_TEST_SHARE
	&share_attribute.attr,
#endif
#ifdef CONFIG_TEST_API_DIS
	&api_dis_attribute.attr,
#endif
#ifdef CONFIG_TEST_PERCPU
	&percpu_attribute.attr,
#endif
#ifdef CONFIG_TEST_SOCKET_CLIENT
	&socket_client_attribute.attr,
#endif
#ifdef CONFIG_TEST_SEQ
	&seq_attribute.attr,
#endif
#ifdef CONFIG_TEST_TASKLET
	&tasklet_attribute.attr,
#endif
#ifdef CONFIG_TEST_BTREE
	&btree_attribute.attr,
#endif
#ifdef CONFIG_TEST_PRINTK
	&printk_attribute.attr,
#endif
#ifdef CONFIG_X86_64
#ifdef CONFIG_TEST_X2APIC
	&x2apic_attribute.attr,
#endif
#ifdef CONFIG_TEST_APIC
	&apic_attribute.attr,
#endif
#ifdef CONFIG_TEST_SUBERROR
	&suberror_attribute.attr,
#endif
#ifdef CONFIG_TEST_ASM
	&asm_attribute.attr,
#endif
#ifdef CONFIG_TEST_SMM
	&smm_attribute.attr,
#endif
#ifdef CONFIG_TEST_TSC
	&tsc_attribute.attr,
#endif
#ifdef CONFIG_TEST_MSR_MMIO
	&msr_mmio_attribute.attr,
#endif
#ifdef CONFIG_TEST_TSX
	&tsx_attribute.attr,
#endif
#ifdef CONFIG_TEST_X86_MISC
	&x86_misc_attribute.attr,
#endif
#ifdef CONFIG_TEST_EVENT_DELIVERY
	&event_delivery_attribute.attr,
#endif
#endif

#ifdef CONFIG_ARM64
#ifdef CONFIG_TEST_SYSREG
	&sysreg_attribute.attr,
#endif
#ifdef CONFIG_TEST_AARCH64
	&aarch64_attribute.attr,
#endif
#endif
#ifdef CONFIG_TEST_IDR
	&idr_attribute.attr,
#endif
#ifdef CONFIG_TEST_SCHED_DEBUG
	&sched_debug_attribute.attr,
#endif
#ifdef CONFIG_TEST_MUTEX
	&mutex_attribute.attr,
#endif
#ifdef CONFIG_TEST_SRCU
	&srcu_attribute.attr,
#endif
#ifdef CONFIG_TEST_WAIT
	&wait_attribute.attr,
#endif
#ifdef CONFIG_TEST_ATOMIC
	&atomic_attribute.attr,
#endif
#ifdef CONFIG_TEST_IO_WAIT
	&io_wait_attribute.attr,
#endif
#ifdef CONFIG_TEST_RWSEM
	&rwsem_attribute.attr,
#endif
#ifdef CONFIG_TEST_COMPLETE
	&complete_attribute.attr,
#endif
#ifdef CONFIG_TEST_PERCPU_RWSEM
	&percpu_rwsem_attribute.attr,
#endif
#ifdef CONFIG_TEST_WORKQUEUE
	&workqueue_attribute.attr,
#endif
#ifdef CONFIG_TEST_WAITBIT
	&waitbit_attribute.attr,
#endif
#ifdef CONFIG_TEST_RCUWAIT
	&rcuwait_attribute.attr,
#endif
#ifdef CONFIG_TEST_ACCESS_ONCE
	&access_once_attribute.attr,
#endif
#ifdef CONFIG_TEST_SEQLOCK
	&seqlock_attribute.attr,
#endif
#ifdef CONFIG_TEST_IRQWORK
	&irqwork_attribute.attr,
#endif
#ifdef CONFIG_TEST_IPI
	&ipi_attribute.attr,
#endif
#ifdef CONFIG_TEST_EVENTFD
	&eventfd_attribute.attr,
#endif
#ifdef CONFIG_TEST_EPOLL
	&epoll_attribute.attr,
#endif
#ifdef CONFIG_TEST_PROCESS_STATE
	&process_state_attribute.attr,
#endif
#ifdef CONFIG_TEST_SCHED
	&sched_attribute.attr,
#endif
#ifdef CONFIG_TEST_GUARD
	&guard_attribute.attr,
#endif
#ifdef CONFIG_TEST_PERCPU_COUNTER
	&percpu_counter_attribute.attr,
#endif
#ifdef CONFIG_TEST_DEBUGFS
	&debugfs_attribute.attr,
#endif
#ifdef CONFIG_TEST_MISC
	&misc_attribute.attr,
#endif
#ifdef CONFIG_TEST_FOLIO
	&folio_attribute.attr,
#endif
#ifdef CONFIG_TEST_PROC
	&proc_attribute.attr,
#endif
#ifdef CONFIG_TEST_NIC
	&nic_attribute.attr,
#endif
#ifdef CONFIG_TEST_AIO
	&aio_attribute.attr,
#endif
#ifdef CONFIG_TEST_IOURING
	&iouring_attribute.attr,
#endif
#ifdef CONFIG_TEST_TASK_WORK
	&task_work_attribute.attr,
#endif
#ifdef CONFIG_TEST_UACCESS
	&uaccess_attribute.attr,
#endif
#ifdef CONFIG_TEST_KTHREAD
	&kthread_attribute.attr,
#endif
#ifdef CONFIG_TEST_NOTIFIER
	&notifier_attribute.attr,
#endif
#ifdef CONFIG_TEST_RCUPDATE
	&rcupdate_attribute.attr,
#endif
#ifdef CONFIG_TEST_RCULIST
	&rculist_attribute.attr,
#endif
#ifdef CONFIG_TEST_JIFFIES
	&jiffies_attribute.attr,
#endif
#ifdef CONFIG_TEST_SWAIT
	&swait_attribute.attr,
#endif
#ifdef CONFIG_TEST_MM_SL
	&mm_sl_attribute.attr,
#endif
#ifdef CONFIG_TEST_MM_LL
	&mm_ll_attribute.attr,
#endif
#ifdef CONFIG_TEST_MM_SS
	&mm_ss_attribute.attr,
#endif
#ifdef CONFIG_TEST_MM_LS
	&mm_ls_attribute.attr,
#endif
	NULL, /* need to NULL terminate the list of attributes */
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *mymodule;
struct blocking_notifier_head free_resorces;

static int __init greeter_init(void)
{
	int error = 0;
	BLOCKING_INIT_NOTIFIER_HEAD(&free_resorces);
	mymodule = kobject_create_and_add("hacking", kernel_kobj);
	if (!mymodule)
		return -ENOMEM;

	error = sysfs_create_group(mymodule, &attr_group);
	if (error)
		kobject_put(mymodule);

	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return error;
}

static void __exit greeter_exit(void)
{
	pr_info("👋 [%s:%d] \n", __FUNCTION__, __LINE__);
	kobject_put(mymodule);
	blocking_notifier_call_chain(&free_resorces, 0, NULL);
}

module_init(greeter_init);
module_exit(greeter_exit);
