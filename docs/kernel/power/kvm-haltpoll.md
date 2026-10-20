## haltpoll
<!-- ff77abae-593b-4997-abd1-9647408c5efd -->

1. kvm 参数中存在 haltpolling 操作:
https://docs.kernel.org/virt/guest-halt-polling.html
https://liujunming.top/2022/10/29/Notes-about-kvm-halt-polling/

2. 此外，存在一个 cstate governor 来控制:

```txt
➜  cpuidle cpupower idle-info
CPUidle driver: none
CPUidle governor: haltpoll
analyzing CPU 18:

CPU 18: No idle states
```

## 这个到底做啥来的
- drivers/cpuidle/governors/haltpoll.c
	- 这个显然，有一个 governors 就是 haltpoll，代码量很小，只有几百行
- drivers/cpuidle/cpuidle-haltpoll.c
	- 这个就是 idle driver 是 haltpoll ，也是代码量很小的


## 默认状态
```txt
-   99.98%     0.00%  swapper          [kernel.kallsyms]     [k] common_startup_64
   - common_startup_64
      - 96.86% start_secondary
           cpu_startup_entry
         - do_idle
            - 96.86% default_idle_call
                 default_idle
      - 3.13% x86_64_start_kernel
           x86_64_start_reservations
           start_kernel
           __pfx_kernel_init
           cpu_startup_entry
           do_idle
           default_idle_call
           default_idle
```

## 修改 governor

echo haltpoll >  /sys/devices/system/cpu/cpuidle/current_governor

```txt
➜  cpuidle cpupower idle-info
CPUidle driver: none
CPUidle governor: haltpoll
analyzing CPU 18:

CPU 18: No idle states
```
## 添加启动参数，重启

不过，为什么默认不打开?
```c
	if (!kvm_para_available() || !haltpoll_want()){
```

```txt
➜  ~ cpupower idle-info
CPUidle driver: haltpoll
CPUidle governor: haltpoll
analyzing CPU 26:

Number of idle states: 2
Available idle states: POLL haltpoll idle
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 951
Duration: 50468
haltpoll idle:
Flags/Description: default architecture idle
Latency: 1
Usage: 10032
Duration: 9535233
```

perf 的结果为:
```txt
-   99.98%     0.00%  swapper  [kernel.kallsyms]  [k] common_startup_64
   - common_startup_64
      - 96.85% start_secondary
           cpu_startup_entry
         - do_idle
            - 96.85% cpuidle_enter
               - cpuidle_enter_state
                  - 93.31% default_enter_idle
                       default_idle
                    3.54% poll_idle
      - 3.12% x86_64_start_kernel
           x86_64_start_reservations
           start_kernel
           __pfx_kernel_init
           cpu_startup_entry
           do_idle
           cpuidle_enter
         - cpuidle_enter_state
            - 2.35% default_enter_idle
                 default_idle
              0.78% poll_idle
```

## HALTPOLL_CPUIDLE

```c
config HALTPOLL_CPUIDLE
    tristate "Halt poll cpuidle driver"
    depends on X86 && KVM_GUEST
    default y
    help
     This option enables halt poll cpuidle driver, which allows to poll
     before halting in the guest (more efficient than polling in the
     host via halt_poll_ns for some scenarios).
```

## 那么 haltpoll 和 poll 有什么区别，如何控制?

## Documentation/virt/kvm/halt-polling.rst

基本理解:
1. halt polling interval : 	`kvm_vcpu->halt_poll_ns` ，guest halt 后，虚拟机还会 halt 多长时间
2. total block time : 从 halt 到虚拟机接受中断需要的时间
3. global max polling interval : `halt_poll_ns` 命令行参数，这个是认为的应该 polling 的时间

测试一下，这个说法是真的吗?

> Halt polling will only be conducted by the host when no other tasks are runnable on
  that cpu, otherwise the polling will cease immediately and schedule will be invoked to
  allow that other task to run. Thus this doesn't allow a guest to cause denial of service
  of the cpu.

如果系统中的负载很高，如何保证，kvm 的 polling 不会给其他的业务带来压力，也就是，现在切换到其他的
业务是需要时间的!

## lab

debugfs 中的内容也是需要看下:

执行 iperf3 的时候:
```txt
kvm statistics - summary

 Event                                         Total %Total CurAvg/s
 exits                                       1256164   44.3    71764
 halt_exits                                   520599   18.4    29651
 halt_attempted_poll                          509684   18.0    29078
 halt_successful_poll                         508457   17.9    28995
 irq_exits                                     14712    0.5      847
 host_state_reload                             12149    0.4      659
 halt_wakeup                                   12102    0.4      656
 preemption_reported                              48    0.0        3
 preemption_other                                  3    0.0        0
 halt_poll_success_hist                            2    0.0        0
 blocking                                         -2   -0.0        0
 Total                                       2833918          161655
```
## [ ] kvm_emulate_halt

halt 到底要不要退出?

在 arch/x86/include/asm/vmx.h 中是定义了这个的:
```c
#define CPU_BASED_HLT_EXITING                   VMCS_CONTROL_BIT(HLT_EXITING)
```

最好还是要退出的，不然把物理 CPU 占用了

一个机器上有很多 vCPU 都是退出的状态的。

似乎 handle_fastpath_hlt 和 kvm_emulate_halt 都有被调用

## 测试发现
当打开 iperf 的时候，可以发现:
```txt
Tracing 1 functions for "b'kvm_vcpu_check_block'"... Hit Ctrl-C to end.

FUNC                                    COUNT
kvm_vcpu_check_block                  7794616

FUNC                                    COUNT
kvm_vcpu_check_block                  7725424

FUNC                                    COUNT
kvm_vcpu_check_block                  8343019
```
而当 guest 没有负载的时候，这个函数的调用次数几乎为 0 的。

```txt
-  100.00%     0.00%  qemu-system-x86  libc.so.6                [.] __GI___ioctl
   - __GI___ioctl
      - 98.37% entry_SYSCALL_64_after_hwframe
           do_syscall_64
         - __x64_sys_ioctl
            - 98.37% kvm_vcpu_ioctl
               - kvm_arch_vcpu_ioctl_run
                  - 90.46% kvm_vcpu_halt
                     - 61.50% kvm_vcpu_check_block
                        - 47.89% kvm_arch_vcpu_runnable
                           - 30.90% kvm_cpu_has_interrupt
                              - 19.17% apic_has_interrupt_for_ppr
                                 - 18.32% vmx_sync_pir_to_irr
                                      8.97% vmx_set_rvi
                                      6.01% kvm_lapic_find_highest_irr
                              - 8.42% kvm_apic_has_interrupt
                                   6.94% __apic_update_ppr
                                1.64% kvm_apic_accept_pic_intr
                           - 10.58% vmx_interrupt_allowed
                              - 8.84% vmx_interrupt_blocked
                                   0.63% vmx_get_rflags
                          5.20% __srcu_read_lock
                          4.82% __srcu_read_unlock
                          0.74% apic_has_pending_timer
                     - 18.51% ktime_get
                          11.46% read_tsc
                  - 4.73% vmx_vcpu_run
                     - 1.63% kvm_load_host_xsave_state
                          native_write_msr
                     - 1.60% kvm_load_guest_xsave_state
                          native_write_msr
                    0.67% handle_fastpath_set_msr_irqoff
        1.49% vmx_vmexit
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
