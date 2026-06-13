# ttwu

## 经典路线

```txt
@[
    resched_curr+5
    enqueue_task_fair+563
    enqueue_task+55
    ttwu_do_activate+111
    try_to_wake_up+713
    hrtimer_wakeup+34
    __hrtimer_run_queues+332
    hrtimer_interrupt+255
    __sysvec_apic_timer_interrupt+82
    sysvec_apic_timer_interrupt+110
    asm_sysvec_apic_timer_interrupt+26
    cpuidle_enter_state+220
    cpuidle_enter+45
    do_idle+459
    cpu_startup_entry+41
    start_secondary+291
    common_startup_64+318
]: 884
```

在一个 waitq 的唤醒:
- __wake_up_sync_key
  - __wake_up_common_lock
    - __wake_up_common
      - pollwake
        - __pollwake
          - default_wake_function
            - try_to_wake_up
              - select_task_rq
                - select_task_rq_fair

## 实现细节
- try_to_wake_up : 这个函数存在**特别多**的注释，但是基本内容如下，其实很简单:
  - select_task_rq
  - ttwu_queue
    - ttwu_queue_wakelist
    - ttwu_do_activate
      - activate_task
      - ttwu_do_wakeup
      - p->sched_class->task_woken(rq, p);
      - update_avg(&rq->avg_idle, delta);



## ipi

请求者，添加任务到队列中
```txt
@[
    smp_call_function_many_cond+1
    on_each_cpu_cond_mask+64
    flush_tlb_mm_range+392
    ptep_clear_flush+101
    do_wp_page+942
    __handle_mm_fault+2859
    handle_mm_fault+383
    do_user_addr_fault+380
    exc_page_fault+130
    asm_exc_page_fault+38
]: 3

```

```c
static DEFINE_PER_CPU_SHARED_ALIGNED(struct llist_head, call_single_queue);
```

然后接受者收到中断，来进行切换:
```txt
@[
    resched_curr+5
    enqueue_task_fair+559
    enqueue_task+55
    ttwu_do_activate+111
    sched_ttwu_pending+245
    __flush_smp_call_function_queue+320
    __sysvec_call_function_single+28
    sysvec_call_function_single+110
    asm_sysvec_call_function_single+26
    finish_task_switch.isra.0+161
    __schedule+940
    schedule_idle+35
    cpu_startup_entry+41
    start_secondary+286
    common_startup_64+318
]: 1366
```

## TODO
1. 那么 kernel/trace/trace_sched_wakeup.c 是做什么的？

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
