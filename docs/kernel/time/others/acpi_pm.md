# acpi_pm

虚拟机物理机都存在的

 #0  acpi_pm_tmr_read (...) at ../hw/acpi/core.c:528
   #1  memory_region_read_accessor          ../system/memory.c:440
   #2  access_with_adjusted_size            ../system/memory.c:568
   #3  memory_region_dispatch_read1         ../system/memory.c:1392
   #4  memory_region_dispatch_read          ../system/memory.c:1426
   #5  flatview_read_continue_step          ../system/physmem.c:3384
   #6  flatview_read_continue               ../system/physmem.c:3425
   #7  flatview_read                        ../system/physmem.c:3455
   #8  address_space_read_full (as=address_space_io, addr=45064, len=4)   ../system/physmem.c:3468
   #9  kvm_handle_io (direction=0)          ../accel/kvm/kvm-all.c:3128
   #10 kvm_cpu_exec                         ../accel/kvm/kvm-all.c:3512
   #11 kvm_vcpu_thread_fn                   ../accel/kvm/kvm-accel-ops.c:54
   #12 qemu_thread_start                    ../util/qemu-thread-posix.c:414


  cat-469  [001] d..2.  pmtimer: (acpi_pm_read+0x4/0x20)
                     <stack trace>
    => kprobe_trace_func
    => kprobe_dispatcher
    => kprobe_ftrace_handler
    => 0xffffffffc04290da              <- kprobe 优化跳板（模块区），不是内核函数
    => acpi_pm_read
    => ktime_get
    => kcpustat_field
    => uptime_proc_show
    => seq_read_iter
    => vfs_read
    => ksys_read
    => do_syscall_64
    => entry_SYSCALL_64_after_hwframe

```txt
acpi_pm_read
  ktime_get
  tcp_rcv_established
  tcp_v4_do_rcv
  tcp_v4_rcv
  ip_protocol_deliver_rcu
  ip_local_deliver_finish
  __netif_receive_skb_one_core
  process_backlog
  __napi_poll
  net_rx_action
  handle_softirqs
  do_softirq
  __local_bh_enable_ip
  __dev_queue_xmit
  ip_finish_output2
  ip_output
  __ip_queue_xmit
  __tcp_transmit_skb
  tcp_write_xmit
  __tcp_push_pending_frames
  tcp_sendmsg_locked
  tcp_sendmsg
  __sys_sendto
  __x64_sys_sendto
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
```

## vdso 失效

vDSO 的代码路径还会被调用，但在 acpi_pm 下它每次都会在"拿硬
件计数器"这一步失败，然后立刻 fallback 到真正的 syscall()。
换句话说：功能仍然正确，加速完全失效
——clock_gettime/gettimeofday 退化成真系统调用，而在 KVM
guest 里这个系统调用还会附带一次 read port 0xb008 的 VM exit
。

机制（源码）

1. vDSO 数据页里的 clock_mode 跟着当前 clocksource 走

kernel/time/vsyscall.c 中 update_vsyscall()：

```c
  clock_mode = tk->tkr_mono.clock->vdso_clock_mode;
  vc[CS_HRES_COARSE].clock_mode = clock_mode;
  vc[CS_RAW].clock_mode        = clock_mode;   /* RAW 也一起
变 */
```

2. acpi_pm 没有声明 vdso_clock_mode

include/linux/clocksource.h 里 struct clocksource 的
vdso_clock_mode 默认是 0：

┌─────────────────────────────────┬────────────────────────┐
│ clocksource                     │ vdso_clock_mode        │
├─────────────────────────────────┼────────────────────────┤
│ clocksource_tsc                 │ VDSO_CLOCKMODE_TSC     │
│ (arch/x86/kernel/tsc.c)         │                        │
├─────────────────────────────────┼────────────────────────┤
│ kvm_clock                       │ VDSO_CLOCKMODE_PVCLOCK │
│ (arch/x86/kernel/kvmclock.c)    │                        │
├─────────────────────────────────┼────────────────────────┤
│ clocksource_acpi_pm             │ 未设置 →               │
│ (drivers/clocksource/acpi_pm.c) │ VDSO_CLOCKMODE_NONE    │
└─────────────────────────────────┴────────────────────────┘

3. 失败 → 回退 syscall 的链路

arch/x86/include/asm/vdso/gettimeofday.h 中
__arch_get_hw_counter() 只认 TSC/PVCLOCK/HVCLOCK，其它一律
return U64_MAX；紧接着 arch_vdso_cycles_ok() 判 (s64)cycles >= 0，U64_MAX 当作负数 → 失败。于是 lib/vdso/gettimeofday.c中：


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
