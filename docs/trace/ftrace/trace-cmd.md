## https://lwn.net/Articles/410200/

1. ftrace-cmd 的 record 原理: 为每一个 cpu 创建出来一个 process，读取 /sys/kernel/debug/tracing/per_cpu/cpu0，最后主函数将所有的 concatenate 起来
2. -e 可以选择一个 trace event 或者一个 subsystem 的 trace point。
3. 四种功能 : Ftrace also has special plugin tracers that do not simply trace specific events.
   These tracers include the function, function graph, and latency tracers. 1. trace function 指的是 ? 2. `current_tracer` 可以放置的内容那么多，为什么只有这几个 plugin 3. 当 latency tracer 和 perf 的关系是什么 ?

```sh
sudo trace-cmd record -e "kvm:*"
```

```sh
trace-cmd record -p function -l 'sched_*' -n 'sched_slice'
```

```sh
trace-cmd record -p function_graph -l do_IRQ -e irq_handler_entry sleep 10
```

```sh
# 展示参数
sudo  trace-cmd list -e timer:hrtimer_expire_entry -F
# 根据参数过滤
sudo trace-cmd record -e 'timer:*' -f 'common_pid == 1' sleep 1
```

## trace-cmd record

-l -n : function
-g : function_graph
-e -f -R -T : events

对于 function 和 event 打开 stacktrace 是需要单独选项的

https://man7.org/linux/man-pages/man1/trace-cmd-record.1.html
```txt
       -T
           Enable a stacktrace on each event. For example:

                         <idle>-0     [003] 58549.289091: sched_switch:         kworker/0:1:0 [120] R ==> trace-cmd:2603 [120]
                         <idle>-0     [003] 58549.289092: kernel_stack:         <stack trace>
               => schedule (ffffffff814b260e)
               => cpu_idle (ffffffff8100a38c)
               => start_secondary (ffffffff814ab828)

       --func-stack
           Enable a stack trace on all functions. Note this is only
           applicable for the "function" plugin tracer, and will only
           take effect if the -l option is used and succeeds in limiting
           functions. If the function tracer is not filtered, and the
           stack trace is enabled, you can live lock the machine.
```
--func-stack 是通过 options/func_stack_trace 来实现的


```txt
 => secondary_startup_64_no_verify
          <idle>-0       [006] ..s1. 723387.256652: ip_rcv <-__netif_receive_skb_one_core
          <idle>-0       [006] ..s1. 723387.256655: <stack trace>
 => 0xffffffffc5fc309b
 => ip_rcv
 => __netif_receive_skb_one_core
 => process_backlog
 => __napi_poll
 => net_rx_action
 => __do_softirq
 => __irq_exit_rcu
 => sysvec_apic_timer_interrupt
 => asm_sysvec_apic_timer_interrupt
 => cpuidle_enter_state
 => cpuidle_enter
 => do_idle
 => cpu_startup_entry
 => start_secondary
 => secondary_startup_64_no_verify
```

## stacktrace

```sh
sudo trace-cmd start -e  nvme:nvme_complete_rq -T
```
但是这个是存在 bug 的，-T 打开之后就无法关闭了
需要跑到 /sys/kernel/debug/tracing/options 中将其关掉

## trace-cmd start

sudo trace-cmd start -p function -l ip_rcv

```txt
  metrics-server-11107   [016] ..s2. 721410.947718: ip_rcv <-__netif_receive_skb_one_core
  metrics-server-11107   [016] ..s2. 721410.947731: ip_rcv <-__netif_receive_skb_one_core
 qemu-system-x86-1694912 [010] ..s2. 721410.958488: ip_rcv <-__netif_receive_skb_one_core
 .websockify-wra-1695198 [002] ..s2. 721410.958555: ip_rcv <-__netif_receive_skb_one_core
 Chrome_ChildIOT-18110   [011] ..s2. 721410.958940: ip_rcv <-__netif_receive_skb_one_core
 Chrome_ChildIOT-18110   [011] ..s2. 721410.958951: ip_rcv <-__netif_receive_skb_one_core
 .websockify-wra-1695198 [002] ..s2. 721410.959012: ip_rcv <-__netif_receive_skb_one_core
 .websockify-wra-1695198 [002] ..s2. 721410.959016: ip_rcv <-__netif_receive_skb_one_core
      k3s-server-5700    [002] ..s2. 721410.978739: ip_rcv <-__netif_receive_skb_one_core
      k3s-server-5697    [004] ..s2. 721410.978773: ip_rcv <-__netif_receive_skb_one_core
      k3s-server-5697    [004] ..s2. 721410.978780: ip_rcv <-__netif_receive_skb_one_core
```

sudo trace-cmd start -e kvmmmu:kvm_mmu_get_page

```txt
 qemu-system-x86-1695082 [012] ...1. 722267.552895: kvm_mmu_get_page: sp gen 0 gfn 14b600 l1 8-byte q0 direct wux nxe ad root 0 sync new
 qemu-system-x86-1695082 [012] ...1. 722267.552906: <stack trace>
 => trace_event_raw_event_kvm_mmu_get_page
 => tdp_mmu_init_sp
 => kvm_tdp_mmu_map
 => kvm_tdp_page_fault
 => kvm_mmu_page_fault
 => vmx_handle_exit
 => kvm_arch_vcpu_ioctl_run
 => kvm_vcpu_ioctl
 => __x64_sys_ioctl
 => do_syscall_64
 => entry_SYSCALL_64_after_hwframe
```

## 参考资料
- https://man7.org/linux/man-pages/man1/trace-cmd-record.1.html
- https://github.com/rostedt/trace-cmd

## 总结
- trace-cmd 最核心的功能实际上也就是 perf-ftrace 都可以实现，没有必要单独掌握了。

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
