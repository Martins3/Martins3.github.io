# srcu
<!-- 831957e6-7799-4a47-a71e-1dcb79fb501f -->

普通的 rcu 中，read_rcu_lock / read_rcu_unlock 中间不可以 context switch 的，
因为 context switch 就是判断进入 GS 的关键点之一。

如果想要在 read_rcu_lock 中睡眠
```c
  idx = srcu_read_lock(&domain);

  /* 可以调度、睡眠，也可能从一个 CPU 迁移到另一个 CPU */

  srcu_read_unlock(&domain, idx);
```
介绍的简单易懂:
https://liujunming.top/2023/08/06/Linux-kernel-SRCU-usage/

## 基本原理
SRCU 的实现相当简单优雅，还是 rcu 机制的本质，如果引用计数全部都消失，那么就可以释放资源了。

和普通的 RCU 一个显著的不同就是每一个 srcu 都是有自己单独的 domain

SRCU 为每个 domain 维护两组读者计数，可以简单理解为：

```txt
active[0]
active[1]
current_index
```
读者进入时：

1. 读取当前使用的计数组。
2. 增加该组的进入计数。
3. 返回对应的 idx。
4. 退出时根据同一个 idx 增加退出计数。

概念上类似：

```c
int srcu_read_lock(struct srcu_struct *s)
{
    int idx = current_index(s);

    increment_lock_count(s, idx);
    return idx;
}

void srcu_read_unlock(struct srcu_struct *s, int idx)
{
    increment_unlock_count(s, idx);
}
```

因此 idx 必须原样传给 srcu_read_unlock() ，这个 idx 就是 current_index
它表示读者进入时属于哪一代。

更新者调用 synchronize_srcu() 时，大致执行：

1. 确保非当前计数组已经清空。
2. 切换当前计数组，让新读者进入另一组。
3. 等待切换前那组的已有读者全部退出。
4. 返回后，调用前已经存在的读者必然都结束了。

开始：

```txt
读者 A、B → active[0]
              current = 0
```

更新者切换：

```txt
旧读者 A、B → active[0]  ← 等待它们退出
新读者 C、D → active[1]  ← 不需要等待
              current = 1
```

A、B 全部退出后，grace period 完成

实际 Tree SRCU 使用每 CPU 的进入/退出计数和内存屏障，避免所有读者竞争同一个全局原子变量。
即使读者睡眠后迁移到另一 CPU，全局汇总的“进入次数减退出次数”仍能判断这一代是否还有活动读者。

## 经典案例

### kvm_mmu_notifier_invalidate_range_start

```c
static void kvm_mmu_notifier_release(struct mmu_notifier *mn,
				     struct mm_struct *mm)
{
	struct kvm *kvm = mmu_notifier_to_kvm(mn);
	int idx;

	idx = srcu_read_lock(&kvm->srcu);
	kvm_flush_shadow_all(kvm);
	srcu_read_unlock(&kvm->srcu, idx);
}
```

### blk_mq_run_dispatch_ops

总是在区分 srcu 和 rcu 的

## 实际实现情况

domain 的定义为:
```c
/*
 * Per-SRCU-domain structure, similar in function to rcu_state.
 */
struct srcu_struct {
	struct srcu_ctr __percpu *srcu_ctrp;
	struct srcu_data __percpu *sda;		/* Per-CPU srcu_data array. */
	u8 srcu_reader_flavor;
	struct lockdep_map dep_map;
	struct srcu_usage *srcu_sup;		/* Update-side data. */
};


/*
 * Per-CPU structure feeding into leaf srcu_node, similar in function
 * to rcu_node.
 */
struct srcu_data {
	/* Read-side state. */
	struct srcu_ctr srcu_ctrs[2];		/* Locks and unlocks per CPU. */
	int srcu_reader_flavor;			/* Reader flavor for srcu_struct structure? */
						/* Values: SRCU_READ_FLAVOR_.*  */

	/* Update-side state. */
	raw_spinlock_t __private lock ____cacheline_internodealigned_in_smp;
	struct rcu_segcblist srcu_cblist;	/* List of callbacks.*/
	unsigned long srcu_gp_seq_needed;	/* Furthest future GP needed. */
	unsigned long srcu_gp_seq_needed_exp;	/* Furthest future exp GP. */
	bool srcu_cblist_invoking;		/* Invoking these CBs? */
	struct timer_list delay_work;		/* Delay for CB invoking */
	struct work_struct work;		/* Context for CB invoking. */
	struct rcu_head srcu_barrier_head;	/* For srcu_barrier() use. */
	struct rcu_head srcu_ec_head;		/* For srcu_expedite_current() use. */
	int srcu_ec_state;			/*  State for srcu_expedite_current(). */
	struct srcu_node *mynode;		/* Leaf srcu_node. */
	unsigned long grpmask;			/* Mask for leaf srcu_node */
						/*  ->srcu_data_have_cbs[]. */
	int cpu;
	struct srcu_struct *ssp;
};
```
这里主要的考虑点为:
1. srcu 是支持多核
2. srcu 在 cirtical region 中可以从一个 CPU 中睡眠，然后在另外的一个 CPU 中醒过来

所以，结构为:
```txt
kvm->srcu
  |
  +-- sda  (per-CPU)
       |
       +-- CPU 0: struct srcu_data
       |    +-- srcu_ctrs[0]
       |    |    +-- srcu_locks
       |    |    `-- srcu_unlocks
       |    `-- srcu_ctrs[1]
       |         +-- srcu_locks
       |         `-- srcu_unlocks
       |
       +-- CPU 1: struct srcu_data
       |    +-- srcu_ctrs[0]
       |    `-- srcu_ctrs[1]
       |
       `-- CPU N: ...
```

```txt
  active[0] =
      sum_over_all_cpus(srcu_ctrs[0].srcu_locks) -
      sum_over_all_cpus(srcu_ctrs[0].srcu_unlocks);

  active[1] =
      sum_over_all_cpus(srcu_ctrs[1].srcu_locks) -
      sum_over_all_cpus(srcu_ctrs[1].srcu_unlocks);
```

例如源码扫描第 0 组进入计数：
```c
  for_each_possible_cpu(cpu) {
          struct srcu_data *sdp = per_cpu_ptr(ssp->sda, cpu);

          sum += atomic_long_read(
                  &sdp->srcu_ctrs[0].srcu_locks);
  }
```


### srcu_read_lock()

核心实现：

```c
int __srcu_read_lock(struct srcu_struct *ssp)
{
        struct srcu_ctr __percpu *scp =
                READ_ONCE(ssp->srcu_ctrp);

        this_cpu_inc(scp->srcu_locks.counter);
        smp_mb();

        return __srcu_ptr_to_ctr(ssp, scp);
}
```

ssp->srcu_ctrp = &ssp->sda->srcu_ctrs[0];
那么 CPU 3 上的读者实际增加：
per_cpu(ssp->sda, 3).srcu_ctrs[0].srcu_locks++;

并返回 idx = 0;

  退出时：
```txt
void __srcu_read_unlock(struct srcu_struct *ssp, int idx)
{
        smp_mb();
        this_cpu_inc(
            __srcu_ctr_to_ptr(ssp, idx)->srcu_unlocks.counter);
}
```

如果任务已经迁移到 CPU 5，则增加：

per_cpu(ssp->sda, 5).srcu_ctrs[0].srcu_unlocks++;

进入和退出可以发生在不同 CPU。因为更新者比较的是所有 CPU 的总和，所以仍然成立。

### synchronize_srcu()

current_index 的具体实现为:
```c
static inline bool __srcu_ptr_to_ctr(
        struct srcu_struct *ssp,
        struct srcu_ctr __percpu *scpp)
{
        return scpp - &ssp->sda->srcu_ctrs[0];
}
```

srcu_flip() 切换 srcu_ctrp

```c
WRITE_ONCE(ssp->srcu_ctrp,
        &ssp->sda->srcu_ctrs[
            !(ssp->srcu_ctrp -
              &ssp->sda->srcu_ctrs[0])
        ]);

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
