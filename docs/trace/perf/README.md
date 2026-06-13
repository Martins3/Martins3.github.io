# 关于 perf 我知道的一切

## 总的入口
- https://perf.wiki.kernel.org/index.php/Main_Page
- https://perf.wiki.kernel.org/index.php/Tutorial

- https://www.brendangregg.com/perf.html

## 简单分类

### 核心
- perf-report
- perf-record
- perf-top
- perf-stat
- perf-probe
- [ ] perf-diff
- [ ] perf-script-perl
- [ ] perf-script-python
- [ ] perf-script : 展示原始数据，方便使用脚本分析


### perf 领域模块相关
<!-- 5265b2e5-c202-488b-8275-1a4eb6cde4c8 -->

这都是怎么实现的? 应该是同时收集各种 tracepoint 吧

- perf-kmem
- perf-kwork
- perf-sched : 难道说，用的是 tracepoint 来实现的
- perf-lock
- perf-iostat

**问题**: perf-kvm、perf-kmem、perf-kwork、perf-sched、perf-lock、perf-iostat 这些都是怎么实现的?

**回答**: 这些工具确实是基于 **tracepoint** 实现的。它们本质上是对多个相关 tracepoint 的封装和聚合分析：

| 工具 | 底层实现 | 使用的 Tracepoint |
|------|---------|------------------|
| `perf-sched` | `sched:sched_switch`, `sched:sched_wakeup`, `sched:sched_migrate_task` 等 | 调度相关 tracepoint |
| `perf-kmem` | `kmem:kmalloc`, `kmem:kfree`, `kmem:mm_page_alloc` 等 | 内存分配相关 |
| `perf-lock` | `lock:lock_acquire`, `lock:lock_release` | 锁事件 |
| `perf-kwork` | `workqueue:workqueue_queue_work`, `workqueue:workqueue_execute_start` | 工作队列 |
| `perf-iostat` | block 层的 tracepoint | block 相关 |
| `perf-kvm` | |

可以通过 `perf sched record` 后查看 `perf evlist` 来确认它使用了哪些事件。


### 性能计数器
1. perf-mem

```txt
perf mem record ./stream
```

2. perf-c2c

### [ ] 不知道如何处理的
- perf-intel-pt

### 其他
- perf-data : 处理 perf 收集的 data 格式的
- perf-evlist
- [x] perf-kallsyms : 搜索 kallsyms 的，但是难道使用 cat /proc/kallsyms 不是更好?
```txt
🧀  sudo perf kallsyms follow_pte
follow_pte: [kernel] [kernel.kallsyms] 0xffffffffb3122530-0xffffffffb3122700 (0xffffffffb3122530-0xffffffffb3122700)
```
- perf-bench : 可以用来替代 sysbench 使用
- [ ] perf-buildid-cache
- [ ] perf-buildid-list
- [ ] perf-archive

- [ ] perf-daemon
- [ ] perf-dlfilter
- [ ] perf-config
- perf-timechart : 获取整个系统的状态概览，然后输出为 svg 。评价: 效果不好，svg 很难渲染。
  - 看上去没什么神奇的，只是将收集了一些 tracepoint


## 逐个说明

### perf-record
- https://man7.org/linux/man-pages/man1/perf-record.1.html

#### 1
同时观察系统中所有的 events :

perf record --all-kernel -- sleep  10
```txt
66.95%  sleep    -  [k] kmem_cache_free_bulk.part.0
14.73%  sleep    -  [k] kmem_cache_alloc
 8.95%  sleep    -  [k] mas_next_slot
 8.63%  sleep    -  [k] next_uptodate_page
 0.33%  sleep    -  [k] __perf_event_task_sched_in
 0.20%  perf-ex  -  [k] nmi_restore
 0.19%  sleep    -  [k] nmi_restore
 0.01%  sleep    -  [k] native_write_msr
 0.00%  perf-ex  -  [k] native_write_msr
 0.00%  sleep    -  [k] x86_pmu_enable
 0.00%  perf-ex  -  [k] intel_pmu_handle_irq
 0.00%  perf-ex  -  [k] perf_ctx_enable
 0.00%  sleep    -  [k] intel_pmu_handle_irq
 0.00%  sleep    -  [k] native_apic_msr_write
```

#### [ ] 2 --stat
```txt
       -s, --stat
           Record per-thread event counts. Use it with perf report -T to see the values.
```

```sh
sudo perf record -s ./a.out
sudo perf report -T
```

得到结果:
```txt
    33.33%  a.out    [kernel.kallsyms]  [k] filemap_map_pages
    33.33%  a.out    [kernel.kallsyms]  [k] handle_softirqs
    33.33%  a.out    libc.so.6          [.] printf
```

#### 3 使用 -e 指定 events

都可以使用 perf record -e cgroup-switches 来实现
```txt
List of pre-defined events (to be used in -e or -M):

  branch-instructions OR branches                    [Hardware event]
  branch-misses                                      [Hardware event]
  cache-misses                                       [Hardware event]
  cache-references                                   [Hardware event]
  cpu-cycles OR cycles                               [Hardware event]
  instructions                                       [Hardware event]
  stalled-cycles-backend OR idle-cycles-backend      [Hardware event]
  stalled-cycles-frontend OR idle-cycles-frontend    [Hardware event]
  alignment-faults                                   [Software event]
  bpf-output                                         [Software event]
  cgroup-switches                                    [Software event]
  context-switches OR cs                             [Software event]
  cpu-clock                                          [Software event]
  cpu-migrations OR migrations                       [Software event]
  dummy                                              [Software event]
  emulation-faults                                   [Software event]
  major-faults                                       [Software event]
  minor-faults                                       [Software event]
  page-faults OR faults                              [Software event]
  task-clock                                         [Software event]
  duration_time                                      [Tool event]
  user_time                                          [Tool event]
  system_time                                        [Tool event]
```


### perf-stat
-  使用 perf stat -e cycles 是不是可以测试 CPU 频率
  - 我都不知道输出这个有什么意义


> perf-stat - Run a command and gather performance counter statistics

观测的性能计数器的

```txt
 Performance counter stats for 'system wide':

        166,821.89 msec cpu-clock                        #   32.001 CPUs utilized
            26,946      context-switches                 #  161.526 /sec
               217      cpu-migrations                   #    1.301 /sec
             1,536      page-faults                      #    9.207 /sec
    17,464,397,614      cycles                           #    0.105 GHz                         (83.33%)
       141,145,121      stalled-cycles-frontend          #    0.81% frontend cycles idle        (83.33%)
       395,044,169      stalled-cycles-backend           #    2.26% backend cycles idle         (83.33%)
     3,560,584,630      instructions                     #    0.20  insn per cycle
                                                  #    0.11  stalled cycles per insn     (83.33%)
       783,154,336      branches                         #    4.695 M/sec                       (83.34%)
         7,230,596      branch-misses                    #    0.92% of all branches             (83.34%)

       5.213018552 seconds time elapsed
```

### perf-annotate

用于分析指令级别的代码，但是执行级别的 perf 没太看懂

感觉 snapshot_refaults ，结果都是在
```txt
Percent│       cmp    0x80(%r15),%r12
       │     ↓ jne    f1
  0.47 │ 5a:   nop
       │       mov    $0xffffffff,%edi
       │       xor    %r14d,%r14d
       │     ↓ jmp    83
       │ 69:   mov    0x340(%r15),%rax
  0.47 │       lea    0x50(%rax),%rdx
  5.92 │       movslq %edi,%rax
       │       mov    -0x48ebc640(,%rax,8),%rax
  0.03 │       add    (%rax,%rdx,1),%r14
 92.86 │ 83:   mov    %rbx,%rsi
  0.16 │     → callq  cpumask_next
       │       cmp    0x123a873(%rip),%eax
       │       mov    %eax,%edi
  0.03 │     ↑ jb     69
       │       test   %r14,%r14
       │       mov    $0x0,%eax
       │       cmovns %r14,%rax
  0.03 │ a1:   xor    %edx,%edx
       │       mov    %rbp,%rsi
       │       mov    %rax,0x78(%r15)
       │       mov    %r13,%rdi
       │     → callq  mem_cgroup_iter
       │       test   %rax,%rax
       │       mov    %rax,%rbp
       │     ↑ jne    38
       │       add    $0x8,%rsp
       │       pop    %rbx
       │       pop    %rbp
       │       pop    %r12
       │       pop    %r13
       │       pop    %r14
```

都是在这个 for 循环这里:
```sh
	for_each_possible_cpu(cpu)
		x += per_cpu(pn_ext->lruvec_stat_local->count[idx], cpu);
```
所以，这是完全符合预期的，而不是说这一条指令执行特别慢

### perf-script

#### perf 生成 flamegraph
<!-- cda52c0b-f3fa-4391-a034-fea682f33214 -->

参考内核 tools/perf/scripts/python/flamegraph.py 中的文件，发现是支持直接生成 flamegraph 的

> plain perf record -a -g -F 99 sleep 60
>     perf script report flamegraph
>
> Combined:
>
>    plain perf script flamegraph -a -F 99 sleep 60

而 https://github.com/brendangregg/FlameGraph 可以针对于任何格式生成项目

更好的工具:
https://github.com/jonhoo/inferno

### perf-list
- [ ] man perf-list(1) 分析了很多内容，没太看懂

man perf-list(1) 没有接受，其实可以这样来展示 tracepoint subsys

- perf list kvm

### perf-c2c

cache 的观测，但是测试不成功

### perf-top
sudo perf top 实时展示系统中那个符号的消耗最高

sudo perf top -e syscalls:sys_enter_openat

perf-top 的功能选项很多

### perf-trace

#### syscall
```sh
sudo perf trace ls
```

简单看了下，应该是靠的 syscall tracepoint 来实现的
```txt
🧀   perf trace ls
Error:  No permissions to read /sys/kernel/tracing//events/raw_syscalls/sys_(enter|exit)
Hint:   Try 'sudo mount -o remount,mode=755 /sys/kernel/tracing/'
```

好玩的部分:
```txt
sudo perf trace --call-graph dwarf  ls

#  这个 `--kernel-syscall-graph` 似乎是没有效果的，配合上 --max-stack 使用也是没有什么神奇之处
sudo perf trace --max-stack=8 --kernel-syscall-graph ls
```

- [ ] sudo perf trace -F min --max-stack=8 --max-events 10 ls # 确认下，minor fault 是通过 pmc 实现的吗?

- 比 strace 更加好用的工具吧
- https://martincarstenbach.wordpress.com/2021/07/29/do-you-know-perf-trace-its-an-almost-perfect-replacement-for-strace/
- https://unix.stackexchange.com/questions/696280/how-do-i-use-perf-trace-record

正如 Manual 所说，perf-trace 不限于来做 syscall 的跟踪
**This is a live mode tool in addition to working with perf.data files like the other perf tools.**

```sh
sudo perf trace -e sched:*switch
```


### perf-ftrace

简单的封装了 function 和 function-graph

function 和 function-graph 之外的 tracer 没有办法正常使用，而且那些 tracer 本来只是用于特殊场景
```sh
sudo perf ftrace ls # 默认全体 graph
sudo perf ftrace -t wakeup ls # 无法正常使用
sudo perf ftrace -G vfs_open ls
sudo perf ftrace -T vfs_open ls
```

```sh
sudo perf ftrace -C0 -G '__netif_receive_skb_list_core' -g 'smp_*'
```

```txt
    -v, --verbose         Be more verbose
        --func-opts <options>
                          Function tracer options, available options: call-graph,irq-info
        --graph-opts <options>
                          Graph tracer options, available options: args,retval,retval-hex,retaddr,noslee>
        --inherit         Trace children processes
        --tid <tid>       Trace on existing thread id (exclusive to --pid)
```



### perf-kwork
https://man7.org/linux/man-pages/man1/perf-kwork.1.html



### perf-probe

`perf probe --add tcp_sendmsg` 之后，会产生这个目录:

/sys/kernel/debug/tracing/events/probe/do_fault

其实 perf probe 可以基于 function name，或者是 source file ，暂时对于该语法没有兴趣

```txt
sudo perf probe --funcs
```
perf probe --add qi_flush_piotlb

### perf-diff
- perf diff perf-1.data perf-2.data ： 按照 diff 差距来分析

### perf-kvm
在 host 中对于 guest 进行 profile ，应该是无法实现 -g 的效果的

### perf-inject

**perf-inject**: 数据注入工具
- 用于向 perf.data 文件中添加额外信息
- 常见用法：将分支栈信息注入，用于 better call graph
```bash
perf record --branch-stack ./a.out
perf inject -i perf.data -b -o perf.data.new
```


### perf-sched

```sh
sudo perf sched record sleep 10
sudo perf sched latency
sudo perf sched map
sudo perf sched replay
sudo perf sched timehist
sudo perf sched script # 展示原始数据，看上去 perf sched script 没有太大的区别
sudo perf report # 用常规方法查看
```

实现的原理就是同时在这些位置插入 tracepoint 的，类似
```txt
124 sched:sched_switch
83 sched:sched_stat_runtime
0 sched:sched_process_fork
0 sched:sched_wakeup_new
33 sched:sched_migrate_task
46 sched:sched_waking
0 dummy:HG
```

https://zhuanlan.zhihu.com/p/14709946806

原来 perf sched latency 是可以知道一个程序是什么唤醒的
```txt
复现后利用perf sched latency看看各个线程的调度延迟以及时间点：

$ perf sched latency
...
  :211677:211677        |    160.391 ms |     2276 | avg:    2.231 ms | max:  630.267 ms | max at: 1802765.259076 s
  :211670:211670        |    137.200 ms |     2018 | avg:    2.356 ms | max:  591.592 ms | max at: 1802765.270541 s
...
这俩的max at时间点是接近的，max抖动的值和数据库的慢请求日志也能对上。说明数据库的抖动就是来自于内核调度延迟！但为什么这么慢呢？
```

### perf-test
<!-- 55bd96a4-458c-473a-9667-d86cd5defa9f -->

perf 环境检测，之后遇到 perf 的时候，符号找不到，可以用这个方法。
检查

#### 自己构建的内核，有时候 perf 没有符号信息了
```txt
+   80.46%     0.00%  vhost-88680      [unknown]                [k] 0000000000000000
+   80.46%     0.00%  vhost-88680      [kernel.kallsyms]        [k] ret_from_fork_asm
+   80.46%     0.00%  vhost-88680      [kernel.kallsyms]        [k] ret_from_fork
+   80.46%     0.03%  vhost-88680      [kernel.kallsyms]        [k] vhost_task_fn
+   80.21%     0.09%  vhost-88680      [vhost]                  [k] vhost_run_work_list
+   58.46%     0.00%  vhost-88680      [vhost]                  [k] 0xffffffffc24dc673
+   58.46%     0.11%  vhost-88680      [tun]                    [k] tun_recvmsg
+   58.31%     0.05%  vhost-88680      [tun]                    [k] tun_do_read
+   57.19%     0.47%  vhost-88680      [tun]                    [k] tun_put_user.isra.0
+   55.51%     0.07%  vhost-88680      [kernel.kallsyms]        [k] skb_copy_datagram_iter
+   55.43%     0.60%  vhost-88680      [kernel.kallsyms]        [k] __skb_datagram_iter
+   52.95%    52.78%  vhost-88680      [kernel.kallsyms]        [k] _copy_to_iter
+   17.67%     0.00%  CPU 0/KVM        libc.so.6                [.] start_thread
+   17.67%     0.00%  CPU 0/KVM        qemu-system-x86_64       [.] qemu_thread_start
+   17.67%     0.00%  CPU 0/KVM        qemu-system-x86_64       [.] kvm_vcpu_thread_fn
+   17.67%     0.00%  CPU 0/KVM        qemu-system-x86_64       [.] kvm_cpu_exec
+   17.67%     0.00%  CPU 0/KVM        libc.so.6                [.] __GI___ioctl
+   17.30%     0.00%  CPU 0/KVM        [kernel.kallsyms]        [k] entry_SYSCALL_64_after_hwframe
+   17.30%     0.00%  CPU 0/KVM        [kernel.kallsyms]        [k] do_syscall_64
+   17.30%     0.00%  CPU 0/KVM        [kernel.kallsyms]        [k] __x64_sys_ioctl
+   17.29%     0.00%  CPU 0/KVM        [snd_hda_core]           [k] 0xffffffffc1ae1a44
+   17.29%     0.00%  CPU 0/KVM        [kvm]                    [k] kvm_arch_vcpu_ioctl_run
+   17.29%     0.07%  CPU 0/KVM        [kvm]                    [k] vcpu_run
+   12.60%     1.74%  vhost-88680      [vhost]                  [k] vhost_get_vq_desc
+   12.48%     0.00%  vhost-88680      [vhost]                  [k] 0xffffffffc24dc4a0
+   11.67%     0.00%  vhost-88680      [vhost]                  [k] 0xffffffffc24d9ea9
+    7.74%     0.00%  CPU 0/KVM        [snd_hda_core]           [k] 0xffffffffc1ae6f14
+    5.72%     0.87%  CPU 0/KVM        [kvm]                    [k] kvm_vcpu_has_events
+    5.71%     0.00%  CPU 0/KVM        [snd_hda_core]           [k] 0xffffffffc1adf966
```

自己构建的内核有这个问题(尝试不要 localmod config 试试 )

这个和可以解决部分问题，但是不能解决所有的问题
sudo perf report -k /home/martins3/data/kernel/linux-full/vmlinux
这是自然，有的内核模块的符号没有到位

(不知道为什么，后来又好了，为什么都没干)

#### 这个报错什么意思?

```txt
➜  ~ perf probe -V qi_flush_piotlb

Failed to find the path for the kernel: No such file or directory
  Error: Failed to show vars.
```

## 常见的使用
<!-- 94454b83-232c-488c-9b27-520552c3b24c -->

应该全部整理到脚本，并且记录一下:

1. 记录所有的到达 `tcp_sendmsg` 的 backtrace 和比例
```sh
sudo perf probe 'tcp_sendmsg'
sudo perf record -e probe:tcp_sendmsg -a -g sleep 10

# 也可以对于 tracepoint 使用
sudo perf record -e io_uring:io_uring_complete -g -C 1 -- sleep 10

```

而使用 bpftrace 可以获取到更加精细的数值统计。

2. 统计所有的 trace point
```sh
sudo perf stat -e 'kvm:*' -a sleep 1s
```

3. 实时统计
perf top -e kvm:kvm_nested_vmrun

4. kprobe 获取返回值
  - perf probe vfs_open%return

5. per-thread : 好吧，并没有感受到效果
```txt
perf record -s ls
perf report -T
```
```txt
       -s, --stat
           Record per-thread event counts. Use it with perf report -T to see the values.
```

6. 使用 ftrace 观测到 function-graph ，并且忽视掉 smp_ 开头的函数

```txt
sudo perf ftrace -C0 -G '__netif_receive_skb_list_core' -g 'smp_*'
```

7. 记录下所有的 kvm 的 tracepoint
```txt
sudo perf record -e "kvm:*"
```

8. 使用 tracepoint 的 filter 功能:
```txt
sudo perf stat -e block:block_rq_issue --filter "nr_sector < 1" -e block:block_rq_issue --filter "nr_sector >= 1"
```

9. 观测 kvm exit 的原因
```txt
sudo perf top -e kvm:kvm_exit
```
10. 获取到 kvm_exit 的时候 ip 是什么
```txt
sudo perf record -e kvm:kvm_exit
sudo perf script
```


## perf 权限问题
<!-- 69d73367-f865-404c-b7dc-3da35c2fb34c -->

| 事件类型 | 是否需要 root | 说明 |
|---------|--------------|------|
| Software events (上下文切换等) | 否 | 普通用户可用 |
| Hardware events (cycles等) | 看配置 | `/proc/sys/kernel/perf_event_paranoid` |
| Tracepoints | 通常需要 | 需要读取 /sys/kernel/tracing |
| kprobes/uprobes | 需要 | 内核调试功能 |

```bash
# 查看当前权限级别
cat /proc/sys/kernel/perf_event_paranoid
# -1: 无限制  0: 允许用户态事件  1: 仅允许内核代码段  2: 仅限内核

echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid
```


## 解决一下 mac 中无法 perf 吗
参考这个:
https://gist.github.com/Manouchehri/6b949141c4096ec75fd4767a3803139d

在 13900k 中，发现已经无法使用这个命令了，实在是有趣的:
```txt
🧀  sudo perf record -C 1
[sudo] password for martins3:
WARNING: A requested CPU in '1' is not supported by PMU 'cpu_atom' (CPUs 16-31) for event 'cycles:P'
Error:
The sys_perf_event_open() syscall returned with 22 (Invalid argument) for event (cpu_atom/cycles/P).
/bin/dmesg | grep -i perf may provide additional information.
```
man perf-record 中专门有一个段说明这个问题

但是似乎也是没有解决的:
https://lore.kernel.org/all/9B585813-A300-4793-AC65-8C69086508CE@gmail.com/T/#mee3b233be1dffd690d8ae43f20dbac56bc83539d


## perf 的 cgroup 支持
<!-- 17daf222-5205-4192-8d72-37ea1bc4a6c7 -->

perf 支持通过 cgroup 过滤事件：

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```bash
# 1. 创建 cgroup
sudo mkdir -p /sys/fs/cgroup/perf_event/mygroup
echo $$ | sudo tee /sys/fs/cgroup/perf_event/mygroup/cgroup.procs

# 2. 使用 -G 或 --cgroup 选项
sudo perf record -G mygroup -e cycles -a -- sleep 10
sudo perf stat -G mygroup -e cycles,instructions -a -- sleep 10

# 3. 监控特定容器的 cgroup
sudo perf record -G docker/container_id -e cycles -a
```

我猜测这个功能是需要内核支持的

## 获取 msr
```txt
🧀  sudo perf list msr
  msr/aperf/                                         [Kernel PMU event]
  msr/cpu_thermal_margin/                            [Kernel PMU event]
  msr/mperf/                                         [Kernel PMU event]
  msr/pperf/                                         [Kernel PMU event]
  msr/smi/                                           [Kernel PMU event]
  msr/tsc/                                           [Kernel PMU event]
  cfg80211:cfg80211_pmsr_complete                    [Tracepoint event]
  cfg80211:cfg80211_pmsr_report                      [Tracepoint event]
  cfg80211:rdev_abort_pmsr                           [Tracepoint event]
  cfg80211:rdev_start_pmsr                           [Tracepoint event]
  kvm:kvm_hv_syndbg_get_msr                          [Tracepoint event]
  kvm:kvm_hv_syndbg_set_msr                          [Tracepoint event]
  kvm:kvm_hv_synic_set_msr                           [Tracepoint event]
  kvm:kvm_msr                                        [Tracepoint event]
  kvm:kvm_vmgexit_msr_protocol_enter                 [Tracepoint event]
  kvm:kvm_vmgexit_msr_protocol_exit                  [Tracepoint event]
  mac80211:drv_abort_pmsr                            [Tracepoint event]
  mac80211:drv_start_pmsr                            [Tracepoint event]
  msr:rdpmc                                          [Tracepoint event]
  msr:read_msr                                       [Tracepoint event]
  msr:write_msr                                      [Tracepoint event]
```

hyperv 虚拟机中观察到的结果:
```txt
sudo perf list msr
  msr/aperf/                                         [Kernel PMU event]
  msr/mperf/                                         [Kernel PMU event]
  msr/tsc/                                           [Kernel PMU event]
  msr:rdpmc                                          [Tracepoint event]
  msr:read_msr                                       [Tracepoint event]
  msr:write_msr                                      [Tracepoint event]
```

## [ ] 在 guest 中 perf

https://stackoverflow.com/questions/21155354/perf-data-file-has-no-samples

```txt
[    0.152649] unchecked MSR access error: WRMSR to 0x38f (tried to write 0x0001000f0000003f) at rIP: 0xffffffffb901006a (__intel_pmu_enable_all.constprop.0+0x5a/0x100)
[    0.153291] Call Trace:
[    0.153291]  <TASK>
[    0.153291]  perf_ctx_enable+0x3c/0x60
[    0.153291]  __perf_install_in_context+0x169/0x210
[    0.153291]  ? __pfx_remote_function+0x10/0x10
[    0.153291]  remote_function+0x49/0x60
[    0.153291]  generic_exec_single+0x79/0xb0
[    0.153291]  smp_call_function_single+0xb8/0x180
[    0.153291]  ? __pfx_remote_function+0x10/0x10
[    0.153291]  perf_install_in_context+0x16f/0x200
[    0.153291]  ? __pfx___perf_install_in_context+0x10/0x10
[    0.153291]  perf_event_create_kernel_counter+0x168/0x1b0
[    0.153291]  hardlockup_detector_event_create+0x34/0x50
[    0.153291]  hardlockup_detector_perf_init+0xb/0x43
[    0.153291]  lockup_detector_init+0x31/0x82
[    0.153291]  kernel_init_freeable+0xf1/0x20c
[    0.153291]  ? __pfx_kernel_init+0x10/0x10
[    0.153291]  kernel_init+0x15/0x120
[    0.153291]  ret_from_fork+0x29/0x50
[    0.153291]  </TASK>
```

默认 -cpu host 就可以实现在 guset 中 perf ，但是存在上面的报错，最后导致无法正确使用。

perf record -e cpu-clock


## 看懂 perf report -p $pid -g 的结果

以 tsc 被替换为 hpet 作为 clocksource 为例:
```txt
-   56.69%     0.22%  fio      [kernel.kallsyms]  [k] entry_SYSCALL_64_after_hwframe
   - 56.47% entry_SYSCALL_64_after_hwframe
      - do_syscall_64
         + 47.92% __x64_sys_io_submit
         + 5.99% __x64_sys_clock_gettime
           1.25% syscall_exit_to_user_mode
         + 0.78% __x64_sys_io_getevents
+   56.41%     0.13%  fio      [kernel.kallsyms]  [k] do_syscall_64
+   50.08%     0.26%  fio      libc.so.6          [.] syscall
-   48.08%     0.58%  fio      [vdso]             [.] __vdso_clock_gettime
   - 47.51% __vdso_clock_gettime
      - 7.09% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 5.82% __x64_sys_clock_gettime
               - 5.32% posix_get_monotonic_timespec
                  - 5.28% ktime_get_ts64
                       5.18% read_hpet
              0.85% syscall_exit_to_user_mode
        1.15% entry_SYSCALL_64
+   48.04%     0.13%  fio      [kernel.kallsyms]  [k] __x64_sys_io_submit
+   47.71%     0.16%  fio      [kernel.kallsyms]  [k] io_submit_one
-   46.06%     0.04%  fio      libc.so.6          [.] clock_gettime@@GLIBC_2.17
   - 46.03% clock_gettime@@GLIBC_2.17
      - 46.00% __vdso_clock_gettime
         - 7.32% entry_SYSCALL_64_after_hwframe
            - do_syscall_64
               - 6.06% __x64_sys_clock_gettime
                  - 5.56% posix_get_monotonic_timespec
                     - 5.52% ktime_get_ts64
```

从上向下，还是列出了所有的符号时间占比，但是注意这里的迷惑性的东西，
有的符号只是前面符号的子集，但是有的并不是，所以:

符号不是从 100% 开始的，因为 `__vdso_clock_gettime` 用 48% ，而 `__x64_sys_io_submit` ，
他们从自己的入口开始。

```txt
-   95.98%     0.51%  fio      libc.so.6          [.] syscall
   - 95.47% syscall
      - 92.99% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 90.75% __x64_sys_io_submit
               - 90.40% io_submit_one
                  - 87.08% aio_read
                     - 86.47% blkdev_read_iter
                        - 86.34% blkdev_direct_IO.part.0
                           - 82.93% submit_bio_noacct_nocheck
                              - 81.61% __submit_bio
                                 - blk_mq_submit_bio
                                    - 59.62% blk_mq_try_issue_directly
                                       - 59.58% __blk_mq_issue_directly
                                          + 40.21% null_handle_cmd
                                          + 19.36% null_queue_rq
                                    - 21.92% __blk_mq_alloc_requests
                                       - 15.00% ktime_get
                                            read_hpet
                                         4.47% blk_mq_rq_ctx_init.isra.0
                                       + 1.45% blk_mq_get_tag
                              + 1.31% ktime_get
                           + 1.87% bio_iov_iter_get_pages
                           + 0.72% bio_alloc_bioset
                  + 1.05% aio_complete
            + 1.47% __x64_sys_io_getevents
              0.53% syscall_exit_to_user_mode
        1.46% entry_SYSCALL_64
```

```txt
-   80.69%     3.13%  fio      libc.so.6          [.] syscall
   - 77.56% syscall
      - 68.55% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 60.08% __x64_sys_io_submit
               - 56.96% io_submit_one.constprop.0
                  - 41.34% aio_read
                     + 41.18% blkdev_read_iter
                  - 4.92% aio_complete
                       1.91% _raw_spin_lock_irqsave
                       0.65% _raw_spin_unlock_irqrestore
                    4.20% __put_user_4
                    1.78% _copy_from_user
                    1.07% fget
                    0.62% kmem_cache_free
                 2.19% __get_user_8
               + 0.75% lookup_ioctx
            - 7.36% __x64_sys_io_getevents
               - 7.08% do_io_getevents
                  - 5.53% read_events
                     - 4.89% aio_read_events
                          2.23% _copy_to_user
                          1.08% mutex_lock
                    1.07% lookup_ioctx
              0.62% syscall_exit_to_user_mode
        7.35% entry_SYSCALL_64
        0.62% entry_SYSCALL_64_safe_stack
        0.61% asm_sysvec_apic_timer_interrupt
```
注意，从

还是在虚拟机中的，其结果为，而物理机中，如果 gtod_reduce=1 ，那么为 240k ，如果为 0 ，
大致只有 130k 。
```txt
sudo fio /home/martins3/data/vn/docs/kernel/code/aio/4k-read.fio
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=64
fio-3.38
Starting 1 process
Jobs: 1 (f=1): [r(1)][0.7%][r=5080MiB/s][r=1300k IOPS][eta 16m:33s]
```

这里把 iodepth=1 的时候还是这样的结果，说明没有什么队列的问题。

暂时无法理解，因为 read_hpet 中仅仅使用了 15% 左右的，但是导致了 80% 的性能下降。


## 观察

分析都是如何发生上下文切换的:
```txt
sudo perf record -g --switch-events -a sleep 10
```

分析 122697 这个进程主要在什么切换走:
```txt
 sudo /usr/share/bcc/tools/offcputime -p  122697  10
```

要是有类似这种仅仅分析软中断就好了:
```txt
perf record -g -a -e 'irq:*,softirq:*' sleep 10
```


## perf 是可以观察到硬中断
<!-- a735682e-47e7-4152-a4af-b069a06820a7 -->

就是在 13900k 上观察，然后就可以看到:
如果没有观察硬中断，考虑一下是不是 perf 配置了参数 -p
导致只是观察这个 process 了，看看 hi 在那个 core ，然后专门 perf 这个 core 就可以了。

而且从原理上来说，perf 使用的是 nmi ，所以就是可以观测到中断的。

```txt
   - common_startup_64
      - 42.77% start_secondary
         - cpu_startup_entry
            - 42.76% do_idle
               - 41.29% cpuidle_idle_call
                  - 38.42% cpuidle_enter
                     - 38.37% cpuidle_enter_state
                        - 17.08% asm_common_interrupt
                           - 16.77% common_interrupt
                              - 14.35% __common_interrupt
                                 - 14.26% handle_edge_irq
                                    - 12.96% handle_irq_event
                                       - 12.25% __handle_irq_event_percpu
                                          - 12.14% nvme_irq
                                             - 8.01% blk_mq_end_request_batch
                                                - 5.91% blk_complete_request.constprop.0
                                                   - 2.85% blkdev_bio_end_io_async
                                                      - 1.78% aio_complete_rw
                                                           1.22% aio_complete
                                                      - 0.97% kmem_cache_free
                                                           0.67% __slab_free
                                                   - 1.11% kmem_cache_free
                                                        0.70% __slab_free
                                                   - 0.79% bio_check_pages_dirty
                                                        __bio_release_pages
                                                     0.69% bio_endio
                                               3.38% nvme_poll_cq
                                      0.75% native_apic_msr_eoi
                              - 0.69% irq_enter_rcu
                                   0.55% tick_irq_enter
                                0.54% __irq_exit_rcu
                          14.12% intel_idle
                          3.36% __irqentry_text_start
                  - 1.65% menu_select
                       0.68% tick_nohz_get_sleep_length
               - 0.54% flush_smp_call_function_queue
                  - 0.52% __flush_smp_call_function_queue
                       0.51% sched_ttwu_pending
               - 0.54% schedule_idle
                    0.52% __schedule
```


## python3-perf
<!-- 25d643a0-c1f8-48bf-9bd4-ce401e5d57d7 -->

tuned 是依赖这个库的:
```txt
tuned/plugins/plugin_scheduler.py | grep perf
# perf code was borrowed from kernel/tools/perf/python/twatch.py
import perf
                                instance._cpus = perf.cpu_map()
                                instance._threads = perf.thread_map()
                                evsel = perf.evsel(type = perf.TYPE_SOFTWARE,
                                        config = perf.COUNT_SW_DUMMY,
                                        sample_type = perf.SAMPLE_TID | perf.SAMPLE_CPU)
                                instance._evlist = perf.evlist(instance._cpus, instance._threads)
                        # no perf
                                                        if event.type == perf.RECORD_COMM:
                                                        elif event.type == perf.RECORD_EXIT:
                cpus = list(perf.cpu_map())
```
可以使用这个库感受一下 docs/trace/perf/perf-python3-demo.py

另外 perf 必然提供了一个 c 的接口的。

perf 还可以支持分析 python 程序的性能:
https://docs.python.org/zh-cn/3.15/howto/perf_profiling.html

## 确认一个问题，perf 是无法处理递归场景的
在这里， nvme_ns_head_submit_bio 被 submit_bio_noacct_nocheck
调用了，之后，其再次到 submit_bio_noacct 中来提交代码:
```txt
   - do_syscall_64
      - 70.65% __do_sys_io_uring_enter
         - 67.45% io_submit_sqes
            - 66.64% io_issue_sqe
               - 66.10% io_write
                  - 63.85% blkdev_write_iter
                     - 62.19% blkdev_direct_IO.part.0
                        - 57.74% submit_bio_noacct_nocheck
                           - 54.70% blk_mq_submit_bio
                              - 51.26% blk_mq_try_issue_directly
                                 - 50.80% __blk_mq_issue_directly
                                    - 50.25% nvme_queue_rq
                                         0.99% nvme_prep_rq.part.0
                                         0.52% _raw_spin_unlock
                              - 1.62% __blk_mq_alloc_requests
                                   0.74% blk_mq_get_tag
                           - 2.01% __submit_bio
                                1.68% nvme_ns_head_submit_bio
                           - 0.56% ktime_get
                                kvm_clock_get_cycles
                        - 2.00% bio_iov_iter_get_pages
                           - 1.86% iov_iter_extract_pages
                              - 1.74% pin_user_pages_fast
                                   1.67% internal_get_user_pages_fast
                        - 1.44% bio_alloc_bioset
                           - 0.93% bio_associate_blkg
                                0.77% bio_associate_blkg_from_css
                  - 0.99% security_file_permission
                       0.57% selinux_file_permission
         - 1.67% __io_run_local_work
            - 1.04% io_req_rw_complete
                 0.62% __fsnotify_parent
           0.54% __fget_light
        0.67% syscall_enter_from_user_mode
```
- 原理上确认下，应该不是每次 stack backtrace 来解决的，不然性能问题太大了，但是这种层次结构是如何实现的?
- 测试上的确认，用 raid1 perf 一次

## 特殊

### perf 观测 page fault
<!-- 822ab805-fb5e-4457-b00a-cd1811c4c616 -->

利用 perf stat
```txt
perf stat -e page-faults,minor-faults,major-faults ls
```
- `minor-faults` 和 `major-faults` 是通过 **software events** (内核计数器) 实现的，不是 PMC
- 缺页是通过 `task_struct` 中的 `min_flt` 和 `maj_flt` 字段统计的


```txt
sudo perf trace --no-syscalls -F all ls
```
(这个应该是通过  tracepoint 获取的)

https://wiki.yoctoproject.org/wiki/Tracing_and_Profiling

## 问题

### --call-graph 和 -g 的区别

下面三种记录的内容都不同:
```txt
sudo perf record -p $(pgrep qemu) -- sleep 10                     # 不记录 stack
sudo perf record --call-graph dwarf -p $(pgrep qemu) -- sleep 10  # 完整详细
sudo perf record -p $(pgrep qemu) -g -- sleep 10                  # 用户态记录的很少
```

man perf-record(1) 对于 -g 和 --call-graph 分析比较清楚

### 当 6.5 内核使用 5.15 的 perf 的时候

似乎出现过
```txt
WARNING: 0-1 isn't a 'cpu_atom', please use a CPU list in the 'cpu_atom' range (16-31) 这个报错需要解决下
```

但是使用 6.5 perf 来进行 rk -p 的时候，会直接导致 guest crash
### software events 都是那些?

### 通过 perf 也可以获取函数的参数

测试发现需要 vmlinux 才可以

sudo perf probe -V policy_zonelist
sudo perf probe -a 'policy_zonelist mode=policy->mode flags=policy->flags'
sudo perf record -e probe:policy_zonelist -aR -- ls

https://netoptimizer.blogspot.com/2016/12/reading-live-runtime-kernel-variables.html



### 通过 debuginfo 展示 L 的信息
sudo perf probe  -m tun -L tun_net_xmit

如果提前拷贝到这里就可以了
```txt
/usr/lib/debug/lib/modules/$(uname -r)/kernel/drivers/net/tun.ko.debug
```

这个也可以:
sudo perf probe  -x /usr/lib/debug/lib/modules/$(uname -r)/kernel/drivers/net/tun.ko -L tun_net_xmit

### 阅读材料
https://news.ycombinator.com/item?id=42278003

https://github.com/perfwiki/main

https://www.brendangregg.com/Slides/SREcon2022_ComputingPerformance.pdf

https://clang.llvm.org/docs/UsersManual.html#profile-guided-optimization

https://google.github.io/tcmalloc/tuning.html

https://news.ycombinator.com/item?id=32214419

### 有趣的使用，也许还有更多的
```txt
🧀  sudo perf report --sort
 Error: option `sort' requires a value
 Usage: perf report [<options>]

    -s, --sort <key[,key2...]>
                          sort by key(s): overhead overhead_sys overhead_us overhead_guest_sys
                          overhead_guest_us overhead_children sample period
                          pid comm dso symbol parent cpu socket srcline srcfile
                          local_weight weight transaction trace symbol_size
                          dso_size cgroup cgroup_id ipc_null time code_page_size
                          local_ins_lat ins_lat local_p_stage_cyc p_stage_cyc
                          addr local_retire_lat retire_lat simd type typeoff
                          symoff dso_from dso_to symbol_from symbol_to mispredict
                          abort in_tx cycles srcline_from srcline_to ipc_lbr
                          addr_from addr_to symbol_daddr dso_daddr locked tlb
                          mem snoop dcacheline symbol_iaddr phys_daddr data_page_size
                          blocked
```

### 参考资料
- https://jvns.ca/perf-zine.pdf
- https://jvns.ca/perf-cheat-sheet.pdf

第三方解析工具
https://github.com/mstange/linux-perf-data

## kvm_stat 原来是直接使用的 perf_event_open
<!-- 4f825b78-5c1f-43d3-a550-be4887c49aa6 -->

tools/kvm/kvm_stat/kvm_stat

```txt
    def _perf_event_open(self, attr, pid, cpu, group_fd, flags):
        """Wrapper for the sys_perf_evt_open() syscall.

        Used to set up performance events, returns a file descriptor or -1
        on error.

        Attributes are:
        - syscall number
        - struct perf_event_attr *
        - pid or -1 to monitor all pids
        - cpu number or -1 to monitor all cpus
        - The file descriptor of the group leader or -1 to create a group.
        - flags

        """
        return self.syscall(ARCH.sc_perf_evt_open, ctypes.pointer(attr),
                            ctypes.c_int(pid), ctypes.c_int(cpu),
                            ctypes.c_int(group_fd), ctypes.c_long(flags))
```

## 分析这几个操作的本质都是什么?
<!-- 5921b02e-aee6-4582-b9d9-d430c9136185 -->

这几个问题现在 ai 生成都很清楚了，而且也是用了很多次，那么这些东西的功能
本质是什么?

1. perf probe 的方法需要操作 ftrace 吗? 如果不需要，ftrace 目录为什么会多东西出来?
如果 perf probe 就是操作 ftrace 的，那么 perf ftrace 的意义是什么
2.
sudo perf record -e probe:update_persistent_clock64 -a -g -- sleep 60
就可以获取 backtrace ，本质上这个是如何得到的?

3.

```txt
方法一：使用 perf probe（推荐）
# 1. 先检查函数是否可被跟踪
grep update_persistent_clock64 /sys/kernel/debug/tracing/available_filter_functions

# 2. 添加探针
sudo perf probe --add update_persistent_clock64

# 3. 跟踪该函数
sudo perf record -e probe:update_persistent_clock64 -a -- sleep 60

# 4. 查看结果
sudo perf report

# 5. 删除探针
sudo perf probe --del probe:update_persistent_clock64
方法二：跟踪时显示调用栈
# 添加探针并跟踪调用栈
sudo perf probe --add update_persistent_clock64
sudo perf record -e probe:update_persistent_clock64 -a -g -- sleep 60
sudo perf report -g
方法三：使用 perf ftrace（函数级跟踪）
# 直接使用 ftrace 跟踪
sudo perf ftrace -T update_persistent_clock64 -a -- sleep 60
方法四：使用 ftrace 直接跟踪（无需 perf）
# 通过 ftrace 接口直接跟踪
cd /sys/kernel/debug/tracing

# 设置跟踪函数
echo update_persistent_clock64 > set_ftrace_filter
echo sync_hw_clock > set_ftrace_filter

# 启用函数跟踪
echo function > current_tracer

echo 1 > tracing_on

# 查看跟踪输出
cat trace

# 关闭跟踪
echo nop > current_tracer
方法五：使用 kprobe 事件跟踪
cd /sys/kernel/debug/tracing

# 添加 kprobe
echo 'p:kprobes/my_probe update_persistent_clock64' >> kprobe_events

# 启用事件
echo 1 > events/kprobes/my_probe/enable

# 查看输出
cat trace

# 清理
echo 0 > events/kprobes/my_probe/enable
echo '-:kprobes/my_probe' >> kprobe_events
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
