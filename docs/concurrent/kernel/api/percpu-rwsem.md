# 为什么 percpu 还需要 rwsem 啊?

Documentation/locking/percpu-rw-semaphore.rst

和普通的 rwsem 差别是: 传统的 rwsem 即使是读也是需要上锁的，使用了原子指令，
但是 percpu-rw-semaphore 的读只是需要使用 rcu 就可以了，但是 write lock 的开销很大。

又看到老朋友哇:
```c
static inline void percpu_rwsem_release(struct percpu_rw_semaphore *sem,
					bool read, unsigned long ip)
{
	lock_release(&sem->dep_map, ip);
}

static inline void percpu_rwsem_acquire(struct percpu_rw_semaphore *sem,
					bool read, unsigned long ip)
{
	lock_acquire(&sem->dep_map, 0, 1, read, 1, NULL, ip);
}
```

## 这个 lock 不复杂，但是使用了很多黑科技

1. rcu
```txt
#0  synchronize_rcu () at kernel/rcu/tree.c:3489
#1  0xffffffff811df698 in rcu_sync_enter (rsp=rsp@entry=0xffffffff831694c0 <cgroup_threadgroup_rwsem>) at kernel/rcu/sync.c:149
#2  0xffffffff822ab414 in percpu_down_write (sem=sem@entry=0xffffffff831694c0 <cgroup_threadgroup_rwsem>) at kernel/locking/percpu-rwsem.c:231
#3  0xffffffff8122f78c in cgroup_attach_lock (lock_threadgroup=true) at kernel/cgroup/cgroup.c:2437
#4  cgroup_procs_write_start (buf=buf@entry=0xffff888106689140 "1416", threadgroup=threadgroup@entry=true, threadgroup_locked=threadgroup_locked@entry=0xffffc90000017def) at kernel/cgroup/cgroup.c:2939
#5  0xffffffff8123298a in __cgroup_procs_write (of=0xffff888106716780, buf=0xffff888106689140 "1416", threadgroup=threadgroup@entry=true) at kernel/cgroup/cgroup.c:5139
#6  0xffffffff81232af7 in cgroup_procs_write (of=<optimized out>, buf=<optimized out>, nbytes=5, off=<optimized out>) at kernel/cgroup/cgroup.c:5175
#7  0xffffffff814d3de9 in kernfs_fop_write_iter (iocb=0xffffc90000017ea0, iter=<optimized out>) at fs/kernfs/file.c:334
#8  0xffffffff81423ceb in call_write_iter (iter=0x0 <fixed_percpu_data>, kio=0xffffffff822aec19 <_raw_spin_unlock_irq+41>, file=0xffff8881064c4b00) at ./include/linux/fs.h:1851
#9  new_sync_write (ppos=0xffffc90000017f08, len=5, buf=0x7ffe667e979a "1416\n", filp=0xffff8881064c4b00) at fs/read_write.c:491
#10 vfs_write (file=file@entry=0xffff8881064c4b00, buf=buf@entry=0x7ffe667e979a "1416\n", count=count@entry=5, pos=pos@entry=0xffffc90000017f08) at fs/read_write.c:584
#11 0xffffffff81424153 in ksys_write (fd=<optimized out>, buf=0x7ffe667e979a "1416\n", count=5) at fs/read_write.c:637
#12 0xffffffff82294f2c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000017f58) at arch/x86/entry/common.c:50
#13 do_syscall_64 (regs=0xffffc90000017f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#14 0xffffffff824000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

2. 利用了 wait_event

```c
struct percpu_rw_semaphore {
	struct rcu_sync		rss;
	unsigned int __percpu	*read_count;
	struct rcuwait		writer;
	wait_queue_head_t	waiters;
	atomic_t		block;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
#endif
};
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
