# rcu 杂记
## rcu stall 检测的位置
<!-- 08e40a31-e217-4097-8e0d-aceae18402cc -->

rcu_sched_clock_irq 在 timer interrupt 中被周期的调用

- update_process_times
  - rcu_sched_clock_irq

实际上，这个问题比想象的还有复杂，我不相信 rcu stall 只是 timer interrupt 的简单的 kick 就可以了。

## RCU 基本 API
<!-- 4d83f9bb-eb63-4c3b-89b2-5f28843edbd5 -->

具体看 Documentation/RCU/whatisRCU.rst 的 "8.  FULL LIST OF RCU APIs"

- RCU list traversal
- RCU pointer/list update
- RCU
- bh
- sched
- RCU: Initialization/cleanup/ordering
- RCU: Quiescents states and control
- RCU-sync primitive
- RCU-Tasks
- RCU-Tasks-Rude
- RCU-Tasks-Trace
- SRCU list traversal
- SRCU
- SRCU: Initialization/cleanup/ordering
- All: lockdep-checked RCU utility APIs
- All: Unchecked RCU-protected pointer access
- All: Unchecked RCU-protected pointer access with dereferencing prohibited

后面还有基本使用指南

汗流浃背了没有!

## 内核中在使用的 rcu 经典案例
<!-- be38b6e5-0810-4ef6-91e1-3744ab553ee8 -->

### task struct 的 rcu 保护

访问都是需要的:
- get_task_cred
- get_mem_cgroup_from_mm
- find_task_by_pid_ns


```txt
@[
        put_task_struct_rcu_user+0
        __schedule+720
        schedule_idle+48
        do_idle+196
        cpu_startup_entry+64
        secondary_start_kernel+224
        __secondary_switched+192
]: 81
```

然后释放的位置这里，很经典了:
```txt
__put_task_struct+5
rcu_do_batch+486
rcu_core+666
__do_softirq+199
__irq_exit_rcu+171
sysvec_apic_timer_interrupt+118
asm_sysvec_apic_timer_interrupt+26
cpuidle_enter_state+204
cpuidle_enter+45
do_idle+452
cpu_startup_entry+29
start_secondary+277
secondary_startup_64_no_verify+224
```

不过，可以看看
cgroup_procs_write_start 通过 refcount 来
也就是调用 get_task_struct

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
