# rcu wq
<!-- b6568ed4-e6eb-48d7-8ae3-cc9122b94589 -->
配套代码 : ~/vn/m/workqueue.c 中的 testcase 18

这种需求来自两个机制的叠加：
1. 释放必须等 RCU grace period
2. 但真正的清理工作又不能在 RCU 回调上下文里做，需要放到 workqueue 中

```c
static void rcu_work_rcufn(struct rcu_head *rcu)
{
   struct rcu_work *rwork = container_of(rcu, struct rcu_work, rcu);
   local_irq_disable();
   __queue_work(WORK_CPU_UNBOUND, rwork->wq, &rwork->work);  // 只干这一件事
   local_irq_enable();
}
```

回调里什么都不做，只是把 work 入队。这就是全部设计：call_rcu 负责"什么时候安全"，workqueue 负责"在什么上下文干"。

## 经典案例

### aio
kioctx 的 table->table[] 查找是 RCU 保护的，free_ioctx 要做大量可睡眠的清理

```c
static void free_ioctx_reqs(struct percpu_ref *ref)
{
	struct kioctx *ctx = container_of(ref, struct kioctx, reqs);

	/* At this point we know that there are no any in-flight requests */
	if (ctx->rq_wait && atomic_dec_and_test(&ctx->rq_wait->count))
		complete(&ctx->rq_wait->comp);

	/* Synchronize against RCU protected table->table[] dereferences */
	INIT_RCU_WORK(&ctx->free_rwork, free_ioctx);
	queue_rcu_work(system_percpu_wq, &ctx->free_rwork);
}
```

首先，执行到 rcu 中:
- ??
  - envp_init
    - asm_sysvec_apic_timer_interrupt
      - sysvec_apic_timer_interrupt
        - instr_sysvec_apic_timer_interrupt
          - irq_exit_rcu
            - __irq_exit_rcu
              - invoke_softirq
                - __do_softirq
                  - handle_softirqs
                    - rcu_core
                      - rcu_do_batch
                        - rcu_work_rcufn

然后让工作放到 workqueue ，workqueue 执行其 callback ，结果如下:
```txt
@[
        free_ioctx+5
        process_one_work+414
        worker_thread+422
        kthread+228
        ret_from_fork+417
        ret_from_fork_asm+26
]: 1
```

### 其他案例

kmem_cache 销毁
- mm/slab_common.c —— kmem_cache 销毁：kmem_cache_release 用 rcu_work 在 grace period 之后才真正拆 cache（要拿 slab 锁、offload per-cpu sheaf 等
  ）；同文件的 kvfree_rcu 批量回收路径里，per-cpu 的 flush_rcu_work() 用来等
  之前 queue 的批量释放 work 跑完再挂新批次。

- kernel/cgroup/cgroup.c —— css（cgroup subsystem state）的释放 css_free_rwork_fn，cgroup 生命周期结束、reader 退出后才释放子系统状态。

- 网络子系统（net/sched/cls_api.c、drivers/net/macsec.c、 net/netfilter/nfnetlink_queue.c 等）—— RCU 保护的转发表/规则对象，删除后要
  等 grace period，再在进程上下文做解引用计数、释放内存等复杂收尾。

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
