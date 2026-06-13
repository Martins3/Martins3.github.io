# rcu 基本使用
## 一些边缘的操作方法

```txt
➜  /sys find . -name "*rcu*"
# 所有的 slab 下面都有一个
./kernel/slab/ext2_inode_cache/destroy_by_rcu

/sys/kernel/debug/tracing/events/rcu
/sys/kernel/debug/tracing/events/rcu/rcu_utilization
/sys/kernel/debug/tracing/events/rcu/rcu_grace_period
/sys/kernel/debug/tracing/events/rcu/rcu_future_grace_period
/sys/kernel/debug/tracing/events/rcu/rcu_grace_period_init
/sys/kernel/debug/tracing/events/rcu/rcu_exp_grace_period
/sys/kernel/debug/tracing/events/rcu/rcu_exp_funnel_lock
/sys/kernel/debug/tracing/events/rcu/rcu_nocb_wake
/sys/kernel/debug/tracing/events/rcu/rcu_preempt_task
/sys/kernel/debug/tracing/events/rcu/rcu_unlock_preempted_task
/sys/kernel/debug/tracing/events/rcu/rcu_quiescent_state_report
/sys/kernel/debug/tracing/events/rcu/rcu_fqs
/sys/kernel/debug/tracing/events/rcu/rcu_stall_warning
/sys/kernel/debug/tracing/events/rcu/rcu_dyntick
/sys/kernel/debug/tracing/events/rcu/rcu_batch_start
/sys/kernel/debug/tracing/events/rcu/rcu_segcb_stats
/sys/kernel/debug/tracing/events/rcu/rcu_callback
/sys/kernel/debug/tracing/events/rcu/rcu_kvfree_callback
/sys/kernel/debug/tracing/events/rcu/rcu_invoke_callback
/sys/kernel/debug/tracing/events/rcu/rcu_invoke_kvfree_callback
/sys/kernel/debug/tracing/events/rcu/rcu_invoke_kfree_bulk_callback
/sys/kernel/debug/tracing/events/rcu/rcu_batch_end
/sys/kernel/debug/tracing/events/rcu/rcu_torture_read
/sys/kernel/debug/tracing/events/rcu/rcu_barrier


/sys/module/srcutree
/sys/module/srcutree/parameters/srcu_max_nodelay
/sys/module/srcutree/parameters/srcu_max_nodelay_phase
/sys/module/srcutree/parameters/srcu_retry_check_delay

/sys/module/rcupdate
/sys/module/rcupdate/parameters/rcu_cpu_stall_cputime
/sys/module/rcupdate/parameters/rcu_cpu_stall_ftrace_dump
/sys/module/rcupdate/parameters/rcu_cpu_stall_suppress
/sys/module/rcupdate/parameters/rcu_cpu_stall_suppress_at_boot
/sys/module/rcupdate/parameters/rcu_cpu_stall_timeout
/sys/module/rcupdate/parameters/rcu_exp_cpu_stall_timeout
/sys/module/rcupdate/parameters/rcu_exp_stall_task_details
/sys/module/rcupdate/parameters/rcu_expedited
/sys/module/rcupdate/parameters/rcu_normal
/sys/module/rcupdate/parameters/rcu_normal_after_boot
/sys/module/rcupdate/parameters/rcu_task_collapse_lim
/sys/module/rcupdate/parameters/rcu_task_contend_lim
/sys/module/rcupdate/parameters/rcu_task_enqueue_lim
/sys/module/rcupdate/parameters/rcu_task_ipi_delay
/sys/module/rcupdate/parameters/rcu_task_stall_info
/sys/module/rcupdate/parameters/rcu_task_stall_info_mult
/sys/module/rcupdate/parameters/rcu_task_stall_timeout

/sys/module/rcutree
/sys/module/rcutree/parameters/rcu_min_cached_objs
/sys/module/rcutree/parameters/rcu_divisor
/sys/module/rcutree/parameters/rcu_kick_kthreads
/sys/module/rcutree/parameters/rcu_resched_ns
/sys/module/rcutree/parameters/rcu_delay_page_cache_fill_msec
/sys/module/rcutree/parameters/rcu_fanout_leaf
/sys/module/rcutree/parameters/sysrq_rcu
/sys/module/rcutree/parameters/rcu_fanout_exact
/sys/module/rcutree/parameters/rcu_nocb_gp_stride


/sys/kernel/rcu_normal
/sys/kernel/rcu_expedited
/proc/sys/kernel/max_rcu_stall_to_panic
/proc/sys/kernel/panic_on_rcu_stall
```

## 一共是三个内核模块

1. srcutree
2. rcutree
3. rcupdata

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
