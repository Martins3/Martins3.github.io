#ifndef HACKING_H_PA2UMYTB
#define HACKING_H_PA2UMYTB

#include <linux/module.h>
#include "config.h"

ulong get_parameter(uint n);

struct work {
	struct work_struct work;
	int id;
};
int batch_queue_works(work_func_t func, unsigned workers, size_t size);

extern struct kobj_attribute sysfs_attribute;

extern struct blocking_notifier_head free_resorces;

#define DECLARE_TESTER(_prefix)          \
	int test_##_prefix(long action); \
	int test_##_prefix##_init(void); \
	int test_##_prefix##_exit(void);

#define DEFINE_TESTER_RESOURCE(_prefix)                                       \
	static int _prefix##_status_exit(struct notifier_block *unused,       \
					 unsigned long event, void *ptr)      \
	{                                                                     \
		return test_##_prefix##_exit();                               \
	}                                                                     \
                                                                              \
	struct notifier_block _prefix##_status_block = {                      \
		.notifier_call = _prefix##_status_exit,                       \
	};                                                                    \
                                                                              \
	static ssize_t _prefix##_store(struct kobject *kobj,                  \
				       struct kobj_attribute *attr,           \
				       const char *buf, size_t count)         \
	{                                                                     \
		int ret;                                                      \
		long action;                                                  \
		static bool _prefix_##status_ready;                           \
		ret = kstrtol(buf, 10, &action);                              \
		if (ret < 0)                                                  \
			return ret;                                           \
		if (!_prefix_##status_ready) {                                \
			ret = blocking_notifier_chain_register(               \
				&free_resorces, &_prefix##_status_block);     \
			if (ret < 0)                                          \
				return ret;                                   \
			ret = test_##_prefix##_init();                        \
			if (ret < 0) {                                        \
				pr_err(#_prefix " init failed\n");            \
				return ret;                                   \
			}                                                     \
                                                                              \
			_prefix_##status_ready = true;                        \
		}                                                             \
                                                                              \
		pr_info("action = %ld current=%s \n", action, current->comm); \
		pr_debug("%s : action = %lx current=%s now=%lld\n",           \
			 __FUNCTION__, action, current->comm, ktime_get());   \
                                                                              \
		ret = test_##_prefix(action);                                 \
		pr_debug("%s : now=%lld\n", __FUNCTION__, ktime_get());       \
		if (ret) {                                                    \
			pr_err("%s : ret=%d\n", __FUNCTION__, ret);           \
			return ret;                                           \
		}                                                             \
		return count;                                                 \
	}                                                                     \
                                                                              \
	static struct kobj_attribute _prefix##_attribute =                    \
		__ATTR(_prefix, 0660, NULL, _prefix##_store);

#define DEFINE_TESTER(_prefix)                                                \
	static ssize_t _prefix##_store(struct kobject *kobj,                  \
				       struct kobj_attribute *attr,           \
				       const char *buf, size_t count)         \
	{                                                                     \
		int ret;                                                      \
		long action;                                                  \
		ret = kstrtol(buf, 10, &action);                              \
		if (ret < 0)                                                  \
			return ret;                                           \
		pr_info("action = %ld current=%s \n", action, current->comm); \
                                                                              \
		pr_debug("%s : now=%lld\n", __FUNCTION__, ktime_get());       \
		ret = test_##_prefix(action);                                 \
		pr_debug("%s : now=%lld\n", __FUNCTION__, ktime_get());       \
		if (ret)                                                      \
			return ret;                                           \
		return count;                                                 \
	}                                                                     \
                                                                              \
	static struct kobj_attribute _prefix##_attribute =                    \
		__ATTR(_prefix, 0660, NULL, _prefix##_store);

DECLARE_TESTER(sched_debug)
#ifdef CONFIG_TEST_RADIX_TREE
	DECLARE_TESTER(radix_tree)
#endif
#ifdef CONFIG_TEST_ANON_INODE
	DECLARE_TESTER(anon_inode)
#endif
#ifdef CONFIG_TEST_PMMIR
	DECLARE_TESTER(pmmir)
#endif
#ifdef CONFIG_TEST_FTRACER
#endif
#ifdef CONFIG_TEST_EXCEPTION
	DECLARE_TESTER(exception)
#endif
#ifdef CONFIG_TEST_SOFTIRQ
	DECLARE_TESTER(softirq)
#endif
#ifdef CONFIG_TEST_SIGNAL
	DECLARE_TESTER(signal)
#endif
#ifdef CONFIG_TEST_HRTIMER
	DECLARE_TESTER(hrtimer)
#endif
#ifdef CONFIG_TEST_FOLIO_QUEUE
	DECLARE_TESTER(folio_queue)
#endif
#ifdef CONFIG_TEST_SAN
	DECLARE_TESTER(san)
#endif
#ifdef CONFIG_TEST_MODIFIER
	DECLARE_TESTER(modifier)
#endif
#ifdef CONFIG_TEST_REFCOUNT
	DECLARE_TESTER(refcount)
#endif
#ifdef CONFIG_TEST_RCUREF
	DECLARE_TESTER(rcuref)
#endif
#ifdef CONFIG_TEST_SPINLOCK
	DECLARE_TESTER(spinlock)
#endif
#ifdef CONFIG_TEST_RT_MUTEX
	DECLARE_TESTER(rt_mutex)
#endif
#ifdef CONFIG_TEST_MEMORY_MODEL
DECLARE_TESTER(memory_model)
#endif
#ifdef CONFIG_TEST_LOCKDEP
DECLARE_TESTER(lockdep)
#endif
#ifdef CONFIG_TEST_GENETLINK
DECLARE_TESTER(genetlink)
#endif
#ifdef CONFIG_TEST_NETLINK
DECLARE_TESTER(netlink)
#endif
#ifdef CONFIG_TEST_STATIC_KEY
DECLARE_TESTER(static_key)
#endif
DECLARE_TESTER(aarch64)
DECLARE_TESTER(access_once)
DECLARE_TESTER(aio)
DECLARE_TESTER(api_dis)
DECLARE_TESTER(apic)
DECLARE_TESTER(asm)
DECLARE_TESTER(atomic)
DECLARE_TESTER(bio)
DECLARE_TESTER(bitops)
DECLARE_TESTER(btree)
DECLARE_TESTER(cgroup)
DECLARE_TESTER(clock)
DECLARE_TESTER(complete)
DECLARE_TESTER(debugfs)
DECLARE_TESTER(dev)
DECLARE_TESTER(epoll)
DECLARE_TESTER(event_delivery)
DECLARE_TESTER(eventfd)
DECLARE_TESTER(folio)
DECLARE_TESTER(folio_lock)
DECLARE_TESTER(guard)
DECLARE_TESTER(gup)
DECLARE_TESTER(hlist)
DECLARE_TESTER(idr)
DECLARE_TESTER(io_wait)
DECLARE_TESTER(iouring)
DECLARE_TESTER(ipi)
DECLARE_TESTER(irqwork)
DECLARE_TESTER(jiffies)
DECLARE_TESTER(kobject)
DECLARE_TESTER(kthread)
DECLARE_TESTER(kthread)
DECLARE_TESTER(lru)
DECLARE_TESTER(lru_lock)
DECLARE_TESTER(maple_tree)
DECLARE_TESTER(mapping)
DECLARE_TESTER(misc)
DECLARE_TESTER(mm_misc)
DECLARE_TESTER(mm_walk)
DECLARE_TESTER(mmap_lock)
DECLARE_TESTER(mmu_notifier)
DECLARE_TESTER(module)
DECLARE_TESTER(msr_mmio)
DECLARE_TESTER(mutex)
DECLARE_TESTER(nic)
DECLARE_TESTER(notifier)
DECLARE_TESTER(pageflag)
DECLARE_TESTER(pci)
DECLARE_TESTER(percpu)
DECLARE_TESTER(percpu_counter)
DECLARE_TESTER(percpu_ref)
DECLARE_TESTER(percpu_rwsem)
DECLARE_TESTER(pid)
DECLARE_TESTER(printk)
DECLARE_TESTER(proc)
DECLARE_TESTER(process_state)
DECLARE_TESTER(radix_tree)
DECLARE_TESTER(rbtree)
DECLARE_TESTER(rculist)
DECLARE_TESTER(rcupdate)
DECLARE_TESTER(rcuref)
DECLARE_TESTER(rcustall)
DECLARE_TESTER(rcuwait)
DECLARE_TESTER(rng)
DECLARE_TESTER(rwsem)
DECLARE_TESTER(sched)
DECLARE_TESTER(seq)
DECLARE_TESTER(seqlock)
DECLARE_TESTER(sg)
DECLARE_TESTER(share)
DECLARE_TESTER(slub)
DECLARE_TESTER(smm)
DECLARE_TESTER(socket_client)
DECLARE_TESTER(srcu)
DECLARE_TESTER(stack)
DECLARE_TESTER(suberror)
DECLARE_TESTER(swait)
DECLARE_TESTER(sysctl)
DECLARE_TESTER(sysreg)
DECLARE_TESTER(task_work)
DECLARE_TESTER(tasklet)
DECLARE_TESTER(tif)
DECLARE_TESTER(timer)
DECLARE_TESTER(tracepoint)
DECLARE_TESTER(tsc)
DECLARE_TESTER(tsx)
DECLARE_TESTER(uaccess)
DECLARE_TESTER(vmalloc)
DECLARE_TESTER(private)
DECLARE_TESTER(wait)
DECLARE_TESTER(waitbit)
DECLARE_TESTER(workqueue)
DECLARE_TESTER(x2apic)
DECLARE_TESTER(x86_misc)
DECLARE_TESTER(xarray)
DECLARE_TESTER(zstd)
#ifdef CONFIG_TEST_BITMAP_TEST
	DECLARE_TESTER(bitmap_test)
#endif
#ifdef CONFIG_TEST_SBITMAP_TEST
	DECLARE_TESTER(sbitmap_test)
#endif

#endif /* end of include guard: HACKING_H_PA2UMYTB */
