## perf 的实现原理


## TODO
- [x] 硬中断无法被 perf 吗?
  - 不太可能，应该只是 hardware 的时间很短
  - 考虑到 watchdog 的实现就是靠 perf event 维持的，这个是 NMI
  - [ ] 但是的确是没有找到 kernel 的 perf
- [ ] kernel/events/uprobes.c 为什么是放到这里的，难道不是放到 kernel/trace 中间吗?

perf 是基于时间进行采样的，称为：CPU 时钟事件。除了 CPU 时钟事件外，perf 还支持多种事件。
具体的支持的内容到 man perf-record 的 -e 参数上查看。

```txt
sudo perf record -e L1-dcache-load-misses -c 10000 -ag -- sleep 5
```

perf_event 结构体来描述一个事件（如 CPU 时钟事件）

因为一个进程可以同时分析多种事件，所以就使用 perf_event_context 结构来记录属于进程的所有事件

perf_event_overflow() 函数来进行数据采样

## 简单看看代码

实现的位置在: kernel/events/

```c
static struct pmu perf_breakpoint = {
	.task_ctx_nr	= perf_sw_context, /* could eventually get its own */

	.event_init	= hw_breakpoint_event_init,
	.add		= hw_breakpoint_add,
	.del		= hw_breakpoint_del,
	.start		= hw_breakpoint_start,
	.stop		= hw_breakpoint_stop,
	.read		= hw_breakpoint_pmu_read,
};
```

## event
默认的 event 方式:
```txt
#0  event_sched_in (event=event@entry=0xffff8881303b0000, ctx=ctx@entry=0xffff88881f9eac20) at kernel/events/core.c:2517
#1  0xffffffff81350187 in group_sched_in (ctx=0xffff88881f9eac20, group_event=0xffff8881303b0000) at kernel/events/core.c:2584
#2  merge_sched_in (event=0xffff8881303b0000, data=data@entry=0xffffc90001cb7c94) at kernel/events/core.c:3843
#3  0xffffffff813504d8 in visit_groups_merge (ctx=ctx@entry=0xffff88881f9eac20, groups=groups@entry=0xffff88881f9eac68, cpu=<optimized out>, pmu=0xffffffff8337d4e0 <perf_cpu_clock>, data=data@entry=0xffffc90001cb7c94, func=0xffffffff81350010 <merge_sched_in>) at kernel/events/core.c:3788
#4  0xffffffff813508b7 in pmu_groups_sched_in (pmu=<optimized out>, groups=0xffff88881f9eac68, ctx=0xffff88881f9eac20) at kernel/events/core.c:3870
#5  ctx_groups_sched_in (ctx=ctx@entry=0xffff88881f9eac20, groups=groups@entry=0xffff88881f9eac68, cgroup=false) at kernel/events/core.c:3883
#6  0xffffffff813509a0 in ctx_sched_in (ctx=ctx@entry=0xffff88881f9eac20, event_type=event_type@entry=EVENT_FLEXIBLE) at kernel/events/core.c:3937
#7  0xffffffff8135132e in perf_event_sched_in (ctx=0x0 <fixed_percpu_data>, cpuctx=0xffff88881f9eac20) at kernel/events/core.c:2678
#8  ctx_resched (cpuctx=cpuctx@entry=0xffff88881f9eac20, task_ctx=task_ctx@entry=0x0 <fixed_percpu_data>, event_type=EVENT_FLEXIBLE) at kernel/events/core.c:2736
#9  0xffffffff813516cd in __perf_install_in_context (info=<optimized out>) at kernel/events/core.c:2807
#10 0xffffffff81345498 in remote_function (data=0xffffc90001cb7de0) at kernel/events/core.c:92
#11 remote_function (data=0xffffc90001cb7de0) at kernel/events/core.c:72
#12 0xffffffff8123f231 in csd_do_func (csd=0x0 <fixed_percpu_data>, info=0xffffc90001cb7de0, func=0xffffffff81345450 <remote_function>) at kernel/smp.c:133
#13 generic_exec_single (cpu=cpu@entry=15, csd=csd@entry=0xffffc90001cb7d60) at kernel/smp.c:404
#14 0xffffffff8123f441 in smp_call_function_single (cpu=cpu@entry=15, func=func@entry=0xffffffff81345450 <remote_function>, info=info@entry=0xffffc90001cb7de0, wait=wait@entry=1) at kernel/smp.c:647
#15 0xffffffff81345e3c in cpu_function_call (info=0xffff8881303b0000, func=0xffffffff81351510 <__perf_install_in_context>, cpu=15) at kernel/events/core.c:153
#16 perf_install_in_context (ctx=ctx@entry=0xffff88881f9eac20, event=event@entry=0xffff8881303b0000, cpu=15) at kernel/events/core.c:2867
#17 0xffffffff8135458f in __do_sys_perf_event_open (attr_uptr=<optimized out>, pid=<optimized out>, cpu=<optimized out>, group_fd=<optimized out>, flags=<optimized out>) at kernel/events/core.c:12768
#18 0xffffffff824088aa in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001cb7f58) at arch/x86/entry/common.c:52
#19 do_syscall_64 (regs=0xffffc90001cb7f58, nr=<optimized out>) at arch/x86/entry/common.c:83
#20 0xffffffff826000eb in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

L1-dcache-load-misses 作为 trace event 的时候，发送 ipi 中断
```txt
#0  event_sched_in (event=event@entry=0xffff88812e2ff680, ctx=ctx@entry=0xffff88881f62ac20) at kernel/events/core.c:2517
#1  0xffffffff81350187 in group_sched_in (ctx=0xffff88881f62ac20, group_event=0xffff88812e2ff680) at kernel/events/core.c:2584
#2  merge_sched_in (event=0xffff88812e2ff680, data=data@entry=0xffffc90000003f0c) at kernel/events/core.c:3843
#3  0xffffffff813504d8 in visit_groups_merge (ctx=ctx@entry=0xffff88881f62ac20, groups=groups@entry=0xffff88881f62ac68, cpu=<optimized out>, pmu=0xffffffff8320fce0 <pmu>, data=data@entry=0xffffc90000003f0c, func=0xffffffff81350010 <merge_sched_in>) at kernel/events/core.c:3788
#4  0xffffffff813508b7 in pmu_groups_sched_in (pmu=<optimized out>, groups=0xffff88881f62ac68, ctx=0xffff88881f62ac20) at kernel/events/core.c:3870
#5  ctx_groups_sched_in (ctx=ctx@entry=0xffff88881f62ac20, groups=groups@entry=0xffff88881f62ac68, cgroup=false) at kernel/events/core.c:3883
#6  0xffffffff813509a0 in ctx_sched_in (ctx=ctx@entry=0xffff88881f62ac20, event_type=event_type@entry=EVENT_FLEXIBLE) at kernel/events/core.c:3937
#7  0xffffffff8135132e in perf_event_sched_in (ctx=0x0 <fixed_percpu_data>, cpuctx=0xffff88881f62ac20) at kernel/events/core.c:2678
#8  ctx_resched (cpuctx=cpuctx@entry=0xffff88881f62ac20, task_ctx=task_ctx@entry=0x0 <fixed_percpu_data>, event_type=EVENT_FLEXIBLE) at kernel/events/core.c:2736
#9  0xffffffff813516cd in __perf_install_in_context (info=<optimized out>) at kernel/events/core.c:2807
#10 0xffffffff81345498 in remote_function (data=0xffffc90001c37de0) at kernel/events/core.c:92
#11 remote_function (data=0xffffc90001c37de0) at kernel/events/core.c:72
#12 0xffffffff8123ec0b in csd_do_func (csd=0xffffc90001c37d60, info=0xffffc90001c37de0, func=0xffffffff81345450 <remote_function>) at kernel/smp.c:133
#13 __flush_smp_call_function_queue (warn_cpu_offline=warn_cpu_offline@entry=true) at kernel/smp.c:511
#14 0xffffffff8123efb3 in generic_smp_call_function_single_interrupt () at kernel/smp.c:428
#15 0xffffffff81106cfc in __sysvec_call_function_single (regs=<optimized out>) at arch/x86/kernel/smp.c:267
#16 0xffffffff8240d05f in sysvec_call_function_single (regs=0xffffffff83203dc8) at arch/x86/kernel/smp.c:262
```

## 输出
- perf_output_sample
  - perf_sample_save_callchain : 保存内核的调用链
  - perf_output_sample_regs
  - perf_output_sample_ustack

## software events

例如在 mm_account_fault 中可以看到:
```c
	perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS, 1, regs, address);
```

所有的定义
```c
/*
 * Special "software" events provided by the kernel, even if the hardware
 * does not support performance events. These events measure various
 * physical and sw events of the kernel (and allow the profiling of them as
 * well):
 */
enum perf_sw_ids {
	PERF_COUNT_SW_CPU_CLOCK			= 0,
	PERF_COUNT_SW_TASK_CLOCK		= 1,
	PERF_COUNT_SW_PAGE_FAULTS		= 2,
	PERF_COUNT_SW_CONTEXT_SWITCHES		= 3,
	PERF_COUNT_SW_CPU_MIGRATIONS		= 4,
	PERF_COUNT_SW_PAGE_FAULTS_MIN		= 5,
	PERF_COUNT_SW_PAGE_FAULTS_MAJ		= 6,
	PERF_COUNT_SW_ALIGNMENT_FAULTS		= 7,
	PERF_COUNT_SW_EMULATION_FAULTS		= 8,
	PERF_COUNT_SW_DUMMY			= 9,
	PERF_COUNT_SW_BPF_OUTPUT		= 10,
	PERF_COUNT_SW_CGROUP_SWITCHES		= 11,

	PERF_COUNT_SW_MAX,			/* non-ABI */
};
```
使用 perf list 可以所有的列举出来

## man perf_event_open(2) 的基本功能
<!-- c4232cbf-4770-432d-a157-0a40ca6d7f09 -->


```c
int perf_event_open(struct perf_event_attr *attr,
                    pid_t pid,      // 监控目标：0=自身，>0=指定进程，-1=全系统
                    int cpu,        // 监控 CPU：-1=任意，≥0=指定核心
                    int group_fd,   // 事件分组：-1=新建组，fd=加入现有组
                    unsigned long flags);
```

支持两种工作模式：
- 计数模式（Counting）：聚合统计事件总数，通过 read() 读取
- 采样模式（Sampling）：周期性记录样本到环形缓冲区，通过 mmap() 访问

| 类型                   | 说明            | 典型用途                     |
|------------------------|-----------------|------------------------------|
| `PERF_TYPE_HARDWARE`   | 通用硬件事件    | 指令数、CPU 周期、分支预测等 |
| `PERF_TYPE_SOFTWARE`   | 软件事件        | 页错误、上下文切换、CPU 迁移 |
| `PERF_TYPE_HW_CACHE`   | 缓存事件        | L1/L2/LLC 访问与缺失         |
| `PERF_TYPE_TRACEPOINT` | 内核 tracepoint | 精细内核行为追踪             |
| `PERF_TYPE_RAW`        | 原始 PMU 事件   | 架构特定高级计数器           |
| `PERF_TYPE_BREAKPOINT` | 硬件断点        | 内存访问/执行断点监控        |

所有的复杂性来自于三个方面
1. `struct perf_event_attr` 的定义
```c
struct perf_event_attr {
    __u32 type;                 /* Type of event */
    __u32 size;                 /* Size of attribute structure */
    __u64 config;               /* Type-specific configuration */

    union {
        __u64 sample_period;    /* Period of sampling */
        __u64 sample_freq;      /* Frequency of sampling */
    };

    __u64 sample_type;  /* Specifies values included in sample */
    __u64 read_format;  /* Specifies values returned in read */

    __u64 disabled       : 1,   /* off by default */
          inherit        : 1,   /* children inherit it */
          pinned         : 1,   /* must always be on PMU */
          exclusive      : 1,   /* only group on PMU */
          exclude_user   : 1,   /* don't count user */
          exclude_kernel : 1,   /* don't count kernel */
          exclude_hv     : 1,   /* don't count hypervisor */
          exclude_idle   : 1,   /* don't count when idle */
          mmap           : 1,   /* include mmap data */
          comm           : 1,   /* include comm data */
          freq           : 1,   /* use freq, not period */
          inherit_stat   : 1,   /* per task counts */
          enable_on_exec : 1,   /* next exec enables */
          task           : 1,   /* trace fork/exit */
          watermark      : 1,   /* wakeup_watermark */
          precise_ip     : 2,   /* skid constraint */
          mmap_data      : 1,   /* non-exec mmap data */
          sample_id_all  : 1,   /* sample_type all events */
          exclude_host   : 1,   /* don't count in host */
          exclude_guest  : 1,   /* don't count in guest */
          exclude_callchain_kernel : 1,
                                /* exclude kernel callchains */
          exclude_callchain_user   : 1,
                                /* exclude user callchains */
          mmap2          :  1,  /* include mmap with inode data */
          comm_exec      :  1,  /* flag comm events that are
                                   due to exec */
          use_clockid    :  1,  /* use clockid for time fields */
          context_switch :  1,  /* context switch data */
          write_backward :  1,  /* Write ring buffer from end
                                   to beginning */
          namespaces     :  1,  /* include namespaces data */
          ksymbol        :  1,  /* include ksymbol events */
          bpf_event      :  1,  /* include bpf events */
          aux_output     :  1,  /* generate AUX records
                                   instead of events */
          cgroup         :  1,  /* include cgroup events */
          text_poke      :  1,  /* include text poke events */
          build_id       :  1,  /* use build id in mmap2 events */
          inherit_thread :  1,  /* children only inherit */
                                /* if cloned with CLONE_THREAD */
          remove_on_exec :  1,  /* event is removed from task
                                   on exec */
          sigtrap        :  1,  /* send synchronous SIGTRAP
                                   on event */

          __reserved_1   : 26;

    union {
        __u32 wakeup_events;    /* wakeup every n events */
        __u32 wakeup_watermark; /* bytes before wakeup */
    };

    __u32     bp_type;          /* breakpoint type */

    union {
        __u64 bp_addr;          /* breakpoint address */
        __u64 kprobe_func;      /* for perf_kprobe */
        __u64 uprobe_path;      /* for perf_uprobe */
        __u64 config1;          /* extension of config */
    };

    union {
        __u64 bp_len;           /* breakpoint size */
        __u64 kprobe_addr;      /* with kprobe_func == NULL */
        __u64 probe_offset;     /* for perf_[k,u]probe */
        __u64 config2;          /* extension of config1 */
    };
    __u64 branch_sample_type;   /* enum perf_branch_sample_type */
    __u64 sample_regs_user;     /* user regs to dump on samples */
    __u32 sample_stack_user;    /* size of stack to dump on
                                   samples */
    __s32 clockid;              /* clock to use for time fields */
    __u64 sample_regs_intr;     /* regs to dump on samples */
    __u32 aux_watermark;        /* aux bytes before wakeup */
    __u16 sample_max_stack;     /* max frames in callchain */
    __u16 __reserved_2;         /* align to u64 */
    __u32 aux_sample_size;      /* max aux sample size */
    __u32 __reserved_3;         /* align to u64 */
    __u64 sig_data;             /* user data for sigtrap */
};
```
2. Reading results : read() 的结果
3. MMAP layout : mmap 结构体的定义

额外的说明了一些调整:
1. perf_event ioctl calls
2. perf_event related configuration files : 也就是 sysfs 下的文件


- [ ] 的确可以用来插入 kprobe / uprobe
- [ ] 那么可以插入 bpf 程序吗? 那个应该是另外的 syscall 吧?

## 基本
https://mp.weixin.qq.com/s/nfJDX-_aO765KPqseoygpA

1. 通过 pmu 来通知

对于 qemu 进行 perf 的时候，得到如下的内容:
```txt
@[
    perf_event_overflow+5
    intel_pmu_drain_pebs_icl+674
    handle_pmi_common+627
    intel_pmu_handle_irq+266
    perf_event_nmi_handler+42
    nmi_handle+94
    default_do_nmi+67
    exc_nmi+312
    asm_exc_nmi+184
]: 147
```

2. 将结果导出来:

perf_event_output
- perf_prepare_sample : 准备数据
- perf_output_sample() : 将数据保存到环形缓冲区中。

3. perf_swevent_hrtimer : 用于处理 software events

## [ ] 虚拟机的 perf
https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/commit/?id=c797b8872bb996f703ef0d68e763caa3770f8a5f

## 为什么 __perf_event_task_sched_in 在 guest 和 host 中结果完全不一样

__perf_event_task_sched_in 这个为什么在物理机中不断的被调用，而虚拟机中不会被调用

```c
static inline void perf_event_task_sched_in(struct task_struct *prev,
					    struct task_struct *task)
{
	if (static_branch_unlikely(&perf_sched_events))
		__perf_event_task_sched_in(prev, task);

	if (__perf_sw_enabled(PERF_COUNT_SW_CPU_MIGRATIONS) &&
	    task->sched_migrated) {
		__perf_sw_event_sched(PERF_COUNT_SW_CPU_MIGRATIONS, 1, 0);
		task->sched_migrated = 0;
	}
}
```
在虚拟机中，使用 perf record 还是 perf sched record 都无法触发这个


不知道这个会导致多少的性能问题:
```txt
@[
    perf_pmu_disable+5
    x86_pmu_start_txn+63
    merge_sched_in+353
    visit_groups_merge.constprop.0.isra.0+671
    ctx_sched_in+466
    __perf_event_task_sched_in+248
    finish_task_switch.isra.0+501
    __schedule+1002
    schedule+65
    schedule_hrtimeout_range_clock+226
    do_sys_poll+1261
    __x64_sys_poll+195
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

## [ ]  看看 kvm_stat 如何获取 tracepoint 的

## 初始化性能计数器的

```txt
# 内核启动的时候
- ret_from_fork_asm
  - ret_from_fork
    - kernel_init
      - kernel_init_freeable
        - lockup_detector_init
          - watchdog_hardlockup_probe
            - hardlockup_detector_event_create
              - perf_event_create_kernel_counter
                - perf_event_create_kernel_counter
                  - perf_event_alloc
                    - perf_init_event
                      - perf_try_init_event
                        - x86_pmu_event_init
                          - __x86_pmu_event_init
                            - amd_pmu_hw_config
                              - amd_core_hw_config


# 启动 perf 的时候
  - entry_SYSCALL_64
    - do_syscall_64
      - do_syscall_x64
        - __do_sys_perf_event_open
          - perf_event_alloc
            - perf_init_event
              - perf_try_init_event
                - x86_pmu_event_init
                  - __x86_pmu_event_init
                    - amd_pmu_hw_config
                      - amd_core_hw_config
```

## perf 是如何跟踪一个 pid 的
我们发现每隔 3s ， x86_pmu_disable_event 和 __x86_pmu_enable_event 会被调用一次:

ovsdb-server 导致的:
```txt
[   52.016101] x86_pmu_disable_event : ovsdb-server : 0
[   54.518657] __x86_pmu_enable_event ovsdb-server 0
```

跟踪获取到这个调用路径:
```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_poll
        - __se_sys_poll
          - __do_sys_poll
            - do_sys_poll
              - do_poll
                - poll_schedule_timeout
                  - schedule_hrtimeout_range
                    - schedule_hrtimeout_range_clock
                      - schedule
                        - __schedule_loop
                          - __schedule
                            - context_switch
                              - finish_task_switch
                                - perf_event_task_sched_in
                                  - __perf_event_task_sched_in
                                    - perf_event_context_sched_in
                                      - perf_ctx_enable
                                        - perf_pmu_enable
                                          - perf_pmu_enable
                                            - x86_pmu_enable
                                              - x86_pmu_start
                                                - amd_pmu_v2_enable_event
                                                  - __x86_pmu_enable_event
```

```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_poll
        - __se_sys_poll
          - __do_sys_poll
            - do_sys_poll
              - do_poll
                - poll_schedule_timeout
                  - schedule_hrtimeout_range
                    - schedule_hrtimeout_range_clock
                      - schedule
                        - __schedule_loop
                          - __schedule
                            - context_switch
                              - prepare_task_switch
                                - perf_event_task_sched_out
                                  - __perf_event_task_sched_out
                                    - perf_event_context_sched_out
                                      - task_ctx_sched_out
                                        - ctx_sched_out
                                          - __pmu_ctx_sched_out
                                            - group_sched_out
                                              - event_sched_out
                                                - x86_pmu_del
                                                  - x86_pmu_stop
                                                    - amd_pmu_disable_event
                                                      - x86_pmu_disable_event
```
上下文切换太经典了，如果这个程序当时在观测性能，那么切换过来之后就需要将 pmu 打开。


## vPMU ，不敢想象
https://lore.kernel.org/lkml/20240126085444.324918-1-xiong.y.zhang@linux.intel.com/T/#r635fefbc4f501ebe972bd2ad6d97ccec0390a875

## [ ]  intel 下 Guest os 启动的时候遇到过这个问题
```txt
[ 0.188252] unchecked MSR access error: WRMSR to 0x38f (tried to write 0x0001000f0000003f) at rIP: 0xffffffff8101007a (__intel_pmu_enable_all.constprop.0+0x5a/0x100)
[ 0.188759] Call Trace:
[ 0.188759] <TASK>
[ 0.188759] perf_ctx_enable+0x3c/0x60
[ 0.188759] __perf_install_in_context+0x169/0x210
[ 0.188759] ? __pfx_remote_function+0x10/0x10
[ 0.188759] remote_function+0x49/0x60
[ 0.188759] generic_exec_single+0x79/0xb0
[ 0.188759] smp_call_function_single+0xb8/0x180
[ 0.188759] ? __pfx_remote_function+0x10/0x10
[ 0.188759] perf_install_in_context+0x16f/0x200
[ 0.188759] ? __pfx___perf_install_in_context+0x10/0x10
[ 0.188759] perf_event_create_kernel_counter+0x177/0x1c0
[ 0.188759] hardlockup_detector_event_create+0x34/0x50
[ 0.188759] hardlockup_detector_perf_init+0xb/0x43
[ 0.188759] lockup_detector_init+0x31/0x82
[ 0.188759] kernel_init_freeable+0xf1/0x20c
[ 0.188759] ? __pfx_kernel_init+0x10/0x10
[ 0.188759] kernel_init+0x15/0x120
[ 0.188759] ret_from_fork+0x29/0x50
[ 0.188759] </TASK>
[ 0.189032] smp: Bringing up secondary CPUs ...
[ 0.189380] x86: Booting SMP configuration:
[ 0.189629] .... node #0, CPUs: #1 #2 #3 #4 #5 #6 #7 #8 #9 #10 #11 #12 #13 #14 #15 #16 #17 #18 #19 #20 #21
#22 #23 #24 #25 #26 #27 #28 #29 #30 [ 0.200010] smp: Brought up 1 node, 31 CPUs
```

## 测试代码
Documentation/userspace-api/perf_ring_buffer.rst

docs/trace/perf-demo.c

其中 sched_switch_entry 是参考这个的:

```txt
sudo cat /sys/kernel/debug/tracing/events/sched/sched_switch/format
[sudo] password for martins3:
name: sched_switch
ID: 307
format:
        field:unsigned short common_type;       offset:0;       size:2; signed:0;
        field:unsigned char common_flags;       offset:2;       size:1; signed:0;
        field:unsigned char common_preempt_count;       offset:3;       size:1; signed:0;
        field:int common_pid;   offset:4;       size:4; signed:1;

        field:char prev_comm[16];       offset:8;       size:16;        signed:0;
        field:pid_t prev_pid;   offset:24;      size:4; signed:1;
        field:int prev_prio;    offset:28;      size:4; signed:1;
        field:long prev_state;  offset:32;      size:8; signed:1;
        field:char next_comm[16];       offset:40;      size:16;        signed:0;
        field:pid_t next_pid;   offset:56;      size:4; signed:1;
        field:int next_prio;    offset:60;      size:4; signed:1;

print fmt: "prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s%s ==> next_comm=%s next_pid=%d next_prio=%d", REC->prev_comm, REC->prev_pid, REC->prev_prio, (REC->prev_state & ((((0x00000000 | 0x00000001 | 0x00000002 | 0x00000004 | 0x00000008 | 0x00000010 | 0x00000020 | 0x00000040) + 1) << 1) - 1)) ? __print_flags(REC->prev_state & ((((0x00000000 | 0x00000001 | 0x00000002 | 0x00000004 | 0x00000008 | 0x00000010 | 0x00000020 | 0x00000040) + 1) << 1) - 1), "|", { 0x00000001, "S" }, { 0x00000002, "D" }, { 0x00000004, "T" }, { 0x00000008, "t" }, { 0x00000010, "X" }, { 0x00000020, "Z" }, { 0x00000040, "P" }, { 0x00000080, "I" }) : "R", REC->prev_state & (((0x00000000 | 0x00000001 | 0x00000002 | 0x00000004 | 0x00000008 | 0x00000010 | 0x00000020 | 0x00000040) + 1) << 1) ? "+" : "", REC->next_comm, REC->next_pid, REC->next_prio
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
