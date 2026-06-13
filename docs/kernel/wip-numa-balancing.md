# numa balancing

## cat /proc/sys/kernel/numa_balancing 的行为是什么?

## mbind 行为是什么?

## numa balancing 需要什么样的硬件支持

## 如果我来设计 numa balalnce，需要考虑什么问题?
1. 到底是迁移的 cpu 还是迁移内存


> 似乎是为了处理当其中的 NUMA 远程访问，然后直接将 CPU 迁移过去的操作。


```c
struct numa_group{
```

```c
/* The regions in numa_faults array from task_struct */
enum numa_faults_stats {
	NUMA_MEM = 0,
	NUMA_CPU,
	NUMA_MEMBUF,
	NUMA_CPUBUF
};
extern void sched_setnuma(struct task_struct *p, int node);
extern int migrate_task_to(struct task_struct *p, int cpu);
extern int migrate_swap(struct task_struct *p, struct task_struct *t,
			int cpu, int scpu);
extern void init_numa_balancing(unsigned long clone_flags, struct task_struct *p);
```

- init_numa_balancing : 居然
