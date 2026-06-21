## arm 环境的确容易出现 backtrace 没有的情况

为什么会出现这个情况，需要调查一下:

vcpu_put 的 backtrace 只能观察到:
```txt
@[
]: 125
```

x86 中使用 bpftrace 中可以观察到的结果:
```txt
@[
        vcpu_put+5
        kvm_arch_vcpu_ioctl+1380
        kvm_vcpu_ioctl+1742
        __x64_sys_ioctl+150
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 128
```

## tracepoint 完全是正确的
在 arm 中是这个样的，显然，这里仅仅展示了 CPU ，后面的 d.h.. 之类就没有了
这是不知道咋测试的，不用关心了

```txt
 sudo cat /sys/kernel/debug/tracing/trace
# tracer: function
#
# entries-in-buffer/entries-written: 733/733   #P:96
#
#           TASK-PID     CPU#     TIMESTAMP  FUNCTION
#              | |         |         |         |
            sshd-703271  [000]  228905.602395: do_softirq <-__local_bh_enable_ip
             ssh-709049  [086]  228911.290913: do_softirq <-__local_bh_enable_ip
 qemu-system-aar-685149  [087]  228911.295584: do_softirq <-__local_bh_enable_ip
 qemu-system-aar-685149  [087]  228911.316045: do_softirq <-__local_bh_enable_ip
             ssh-709049  [086]  228911.401818: do_softirq <-__local_bh_enable_ip
```

fedora 实际测试 6.17.1-300.fc43.aarch64 ，并不存在任何问题:
```txt
sudo cat /sys/kernel/debug/tracing/trace_pipe

           <...>-1162199 [081] ...1. 686702.860792: do_softirq <-__local_bh_enable_ip
              sh-1162528 [005] .N.1. 686702.925861: do_softirq <-__local_bh_enable_ip
           xargs-1162750 [001] ...1. 686702.959796: do_softirq <-__local_bh_enable_ip
              sh-1164023 [046] .N.1. 686703.195832: do_softirq <-__local_bh_enable_ip
            file-1164155 [001] .N.1. 686703.219857: do_softirq <-__local_bh_enable_ip
            nvim-379058  [046] .N.1. 686703.242890: do_softirq <-__local_bh_enable_ip
           <...>-1164450 [052] ...1. 686703.271789: do_softirq <-__local_bh_enable_ip
    sshd-session-931524  [011] ...1. 686703.677682: do_softirq <-__local_bh_enable_ip
           xargs-1153943 [069] .N.1. 686703.944824: do_softirq <-__local_bh_enable_ip
           <...>-1167670 [079] ...1. 686704.036785: do_softirq <-__local_bh_enable_ip
           xargs-1153943 [057] ...1. 686704.237781: do_softirq <-__local_bh_enable_ip
              sh-1168588 [048] .N.1. 686704.256835: do_softirq <-__local_bh_enable_ip
             cat-1100345 [048] ...1. 686704.256973: do_softirq <-__local_bh_enable_ip
  kworker/u387:1-931114  [048] .N.1. 686704.349832: do_softirq <-__local_bh_enable_ip
    sshd-session-931524  [011] ...1. 686704.353483: do_softirq <-__local_bh_enable_ip

```

观察 idle_cpu 也是如此:
```txt
    kworker/78:1-619     [078] d....   866.987658: cpu_util.constprop.0 <-update_sg_lb_stats.isra.0
    kworker/78:1-619     [078] d....   866.987658: idle_cpu <-update_sg_lb_stats.isra.0
            sudo-13915   [077] .....   866.987658: copy_page_range <-dup_mmap
    kworker/78:1-619     [078] d....   866.987658: cpu_util.constprop.0 <-update_sg_lb_stats.isra.0
    kworker/78:1-619     [078] d....   866.987659: idle_cpu <-update_sg_lb_stats.isra.0
            sudo-13915   [077] .....   866.987659: copy_pte_range <-copy_pud_range
    kworker/78:1-619     [078] d....   866.987659: cpu_util.constprop.0 <-update_sg_lb_stats.isra.0
            sudo-13915   [077] .....   866.987659: __pte_alloc <-copy_pte_range
    kworker/78:1-619     [078] d....   866.987659: idle_cpu <-update_sg_lb_stats.isra.0
            sudo-13915   [077] .....   866.987659: alloc_pages_noprof <-__pte_alloc
    kworker/78:1-619     [078] d....   866.987659: cpu_util.constprop.0 <-update_sg_lb_stats.isra.0
            sudo-13915   [077] .....   866.987659: alloc_frozen_pages_noprof <-alloc_pages_noprof
    kworker/78:1-619     [078] d....   866.987659: idle_cpu <-update_sg_lb_stats.isra.0
            sudo-13915   [077] .....   866.987659: alloc_pages_mpol <-alloc_frozen_pages_noprof
            sudo-13915   [077] .....   866.987659: policy_nodemask <-alloc_pages_mpol
```

但是 guest os 中观察到也是有
```txt
 systemd-journal-520     [000] .....  1353.155004: do_el0_svc <-el0_svc
 systemd-journal-520     [000] .....  1353.155004: syscall_trace_enter <-do_el0_svc
 systemd-journal-520     [000] .....  1353.155004: __secure_computing <-syscall_trace_enter
 systemd-journal-520     [000] .....  1353.155004: __seccomp_filter <-__secure_computing
 systemd-journal-520     [000] .....  1353.155004: populate_seccomp_data <-__seccomp_filter
 systemd-journal-520     [000] .....  1353.155004: invoke_syscall.constprop.0 <-do_el0_svc
 systemd-journal-520     [000] .....  1353.155005: __arm64_sys_gettid <-invoke_syscall.constprop.0
 systemd-journal-520     [000] .....  1353.155005: __task_pid_nr_ns <-__arm64_sys_gettid
 systemd-journal-520     [000] .....  1353.155005: __rcu_read_lock <-__task_pid_nr_ns
 systemd-journal-520     [000] .....  1353.155006: rcu_read_lock_held <-__task_pid_nr_ns
 systemd-journal-520     [000] .....  1353.155006: rcu_lockdep_current_cpu_online <-rcu_read_lock_held
```

6.6 内核中测试结果:
```txt
zcat /proc/config.gz | grep -i ftrace

CONFIG_GCC_SUPPORTS_DYNAMIC_FTRACE_WITH_ARGS=y
CONFIG_HAVE_FTRACE_GRAPH_FUNC=y
CONFIG_HAVE_DYNAMIC_FTRACE=y
CONFIG_HAVE_DYNAMIC_FTRACE_WITH_DIRECT_CALLS=y
CONFIG_HAVE_DYNAMIC_FTRACE_WITH_CALL_OPS=y
CONFIG_HAVE_DYNAMIC_FTRACE_WITH_ARGS=y
CONFIG_HAVE_FTRACE_MCOUNT_RECORD=y
CONFIG_FTRACE=y
CONFIG_DYNAMIC_FTRACE=y
CONFIG_DYNAMIC_FTRACE_WITH_DIRECT_CALLS=y
CONFIG_DYNAMIC_FTRACE_WITH_CALL_OPS=y
CONFIG_DYNAMIC_FTRACE_WITH_ARGS=y
CONFIG_FTRACE_SYSCALLS=y
CONFIG_FTRACE_MCOUNT_RECORD=y
CONFIG_FTRACE_MCOUNT_USE_PATCHABLE_FUNCTION_ENTRY=y
# CONFIG_FTRACE_RECORD_RECURSION is not set
# CONFIG_FTRACE_VALIDATE_RCU_IS_WATCHING is not set
# CONFIG_FTRACE_STARTUP_TEST is not set
# CONFIG_FTRACE_SORT_STARTUP_TEST is not set
# CONFIG_SAMPLE_FTRACE_DIRECT is not set
# CONFIG_SAMPLE_FTRACE_DIRECT_MULTI is not set
# CONFIG_SAMPLE_FTRACE_OPS is not set
CONFIG_HAVE_SAMPLE_FTRACE_DIRECT=y
CONFIG_HAVE_SAMPLE_FTRACE_DIRECT_MULTI=y
```

6.16 的配置:
```txt
CONFIG_GCC_SUPPORTS_DYNAMIC_FTRACE_WITH_ARGS=y
# CONFIG_PSTORE_FTRACE is not set
CONFIG_HAVE_FTRACE_GRAPH_FUNC=y
CONFIG_HAVE_DYNAMIC_FTRACE=y
CONFIG_HAVE_DYNAMIC_FTRACE_WITH_DIRECT_CALLS=y
CONFIG_HAVE_DYNAMIC_FTRACE_WITH_CALL_OPS=y
CONFIG_HAVE_DYNAMIC_FTRACE_WITH_ARGS=y
CONFIG_HAVE_FTRACE_MCOUNT_RECORD=y
CONFIG_FTRACE=y
CONFIG_DYNAMIC_FTRACE=y
CONFIG_DYNAMIC_FTRACE_WITH_DIRECT_CALLS=y
CONFIG_DYNAMIC_FTRACE_WITH_CALL_OPS=y
CONFIG_DYNAMIC_FTRACE_WITH_ARGS=y
CONFIG_FTRACE_SYSCALLS=y
CONFIG_FTRACE_MCOUNT_RECORD=y
CONFIG_FTRACE_MCOUNT_USE_PATCHABLE_FUNCTION_ENTRY=y
# CONFIG_FTRACE_RECORD_RECURSION is not set
# CONFIG_FTRACE_VALIDATE_RCU_IS_WATCHING is not set
# CONFIG_FTRACE_STARTUP_TEST is not set
# CONFIG_FTRACE_SORT_STARTUP_TEST is not set
CONFIG_HAVE_SAMPLE_FTRACE_DIRECT=y
CONFIG_HAVE_SAMPLE_FTRACE_DIRECT_MULTI=y
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
