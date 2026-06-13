# 记录调试 kvm 的一个有趣问题
为什么我构建的 kernel 会让 host CPU 的频率始终最高

虽然 CPU 的利用率不高，但是所有的 CPU 的频率都是最高的。

使用 kvm_stat 做对比:
```txt
kvm statistics - summary

 Event                                         Total %Total CurAvg/s
 kvm_exit                                      13988   16.4    66300
 kvm_entry                                     13985   16.4    66300
 kvm_hv_timer_state                            13938   16.4    66120
 kvm_msr                                        6985    8.2    33160
 kvm_pv_tlb_flush                               6969    8.2    33045
 kvm_apicv_accept_irq                           6961    8.2    33035
 kvm_apic_accept_irq                            6960    8.2    33035
 kvm_vcpu_wakeup                                6962    8.2    33030
 kvm_wait_lapic_expire                          6952    8.2    32990
 kvm_ioapic_set_irq                              440    0.5     2049
 kvm_set_irq                                     440    0.5     2049
 kvm_pic_set_irq                                 440    0.5     2049
 kvm_halt_poll_ns                                 12    0.0       60
 kvm_msi_set_irq                                   6    0.0       30
 kvm_fast_mmio                                     6    0.0       30
 kvm_apic_ipi                                      3    0.0       15
 kvm_apic                                          3    0.0       15
 kvm_unmap_hva_range                               1    0.0        5
 Total                                         85051          403315
```

openEuler 的:
```txt
kvm statistics - summary

 Event                                         Total %Total CurAvg/s
 kvm_ioapic_set_irq                             9044   20.6     2004
 kvm_set_irq                                    9044   20.6     2004
 kvm_pic_set_irq                                9044   20.6     2004
 kvm_exit                                       3343    7.6      645
 kvm_entry                                      3343    7.6      645
 kvm_hv_timer_state                             2959    6.8      570
 kvm_msr                                G        2369    5.4      452
 kvm_apicv_accept_irq                            787    1.8      152
 kvm_apic_accept_irq                             787    1.8      152
 kvm_vcpu_wakeup                                 715    1.6      139
 kvm_pv_tlb_flush                                605    1.4      122
 kvm_apic_ipi                                    488    1.1      100
 kvm_apic                                        488    1.1      100
 kvm_halt_poll_ns                                338    0.8       72
 kvm_wait_lapic_expire                           279    0.6       46
 kvm_page_fault                                  168    0.4       37
 kvm_msi_set_irq                                  20    0.0        7
 kvm_ple_window_update                             4    0.0        0
 kvm_unmap_hva_range                               2    0.0        0
 Total                                         43827            9250
```
使用 perf top -e kvm:msr 可以发现是虚拟机的 exit 的 reason 是 MSR_IA32_TSC_DEADLINE


也就是这里:
```c
fastpath_t handle_fastpath_set_msr_irqoff(struct kvm_vcpu *vcpu)
{
	u32 msr = kvm_rcx_read(vcpu);
	u64 data;
	fastpath_t ret = EXIT_FASTPATH_NONE;

	kvm_vcpu_srcu_read_lock(vcpu);

	switch (msr) {
	case APIC_BASE_MSR + (APIC_ICR >> 4):
		data = kvm_read_edx_eax(vcpu);
		if (!handle_fastpath_set_x2apic_icr_irqoff(vcpu, data)) {
			kvm_skip_emulated_instruction(vcpu);
			ret = EXIT_FASTPATH_EXIT_HANDLED;
		}
		break;
	case MSR_IA32_TSC_DEADLINE:
		data = kvm_read_edx_eax(vcpu);
		if (!handle_fastpath_set_tscdeadline(vcpu, data)) {
			kvm_skip_emulated_instruction(vcpu);
			ret = EXIT_FASTPATH_REENTER_GUEST;
		}
		break;
	default:
		break;
	}

	if (ret != EXIT_FASTPATH_NONE)
		trace_kvm_msr_write(msr, data);

	kvm_vcpu_srcu_read_unlock(vcpu);

	return ret;
}
```

进入到虚拟机中分析:

```txt
🦭 🧀  funccount t:timer:hrtimer_expire_entry -i 1
Tracing 1 functions for "b't:timer:hrtimer_expire_entry'"... Hit Ctrl-C to end.

FUNC                                    COUNT
timer:hrtimer_expire_entry              32087

FUNC                                    COUNT
timer:hrtimer_expire_entry              32064

FUNC                                    COUNT
timer:hrtimer_expire_entry              32063
```

如果是 oe 的 kernel 中观察:

```txt
➜  ~ funccount t:timer:hrtimer_expire_entry -i 1
Tracing 1 functions for "b't:timer:hrtimer_expire_entry'"... Hit Ctrl-C to end.

FUNC                                    COUNT
timer:hrtimer_expire_entry                173

FUNC                                    COUNT
timer:hrtimer_expire_entry                156
^C
FUNC                                    COUNT
timer:hrtimer_expire_entry                 67
Detaching...
```

oe 的 kconfig
```txt
➜  ~ zcat /proc/config.gz | grep HZ
CONFIG_NO_HZ_COMMON=y
# CONFIG_HZ_PERIODIC is not set
# CONFIG_NO_HZ_IDLE is not set
CONFIG_NO_HZ_FULL=y
CONFIG_NO_HZ=y
# CONFIG_HZ_100 is not set
# CONFIG_HZ_250 is not set
# CONFIG_HZ_300 is not set
CONFIG_HZ_1000=y
CONFIG_HZ=1000
CONFIG_MACHZ_WDT=m
```

我构建的:
```txt
🦭 🧀  zcat /proc/config.gz| grep HZ
CONFIG_NO_HZ_COMMON=y
# CONFIG_HZ_PERIODIC is not set
# CONFIG_NO_HZ_IDLE is not set
CONFIG_NO_HZ_FULL=y
CONFIG_NO_HZ=y
# CONFIG_HZ_100 is not set
# CONFIG_HZ_250 is not set
# CONFIG_HZ_300 is not set
CONFIG_HZ_1000=y
CONFIG_HZ=1000
# CONFIG_MACHZ_WDT is not set
```

检查问题 kernel 的 backtrace :
```txt
@[
    enqueue_hrtimer+134
    enqueue_hrtimer+134
    __hrtimer_run_queues+351
    hrtimer_interrupt+244
    __sysvec_apic_timer_interrupt+79
    sysvec_apic_timer_interrupt+113
    asm_sysvec_apic_timer_interrupt+26
    default_idle+19
    default_idle_call+71
    do_idle+244
    cpu_startup_entry+42
    start_secondary+156
    common_startup_64+318
]: 18956

🦭 🧀  funccount t:timer:hrtimer_start -i 1
Tracing 1 functions for "b't:timer:hrtimer_start'"... Hit Ctrl-C to end.

FUNC                                    COUNT
timer:hrtimer_start                     32098
```

是的，在中断中立刻启动中断。

最后，发现是忘记了关闭 kernel 启动参数

```txt
	kernel_args+="nohz=off "
```
忘记了之前是什么时候关闭的了。

## 其他的实验

```sh
sudo perf top -e kvm:kvm_exit
```

都是不部署任何任务的情况下

关闭时钟中断之后

```txt
Samples: 887K of event 'kvm:kvm_exit', 1 Hz, Event count (approx.): 47982 lost: 0/0 dr
Overhead  Trace output
  36.22%  reason MSR_WRITE rip 0xffffffff8110ac58 info 0 0
  34.28%  reason HLT rip 0xffffffff823fa01e info 0 0
   3.19%  reason EPT_MISCONFIG rip 0xffffffff81a42401 info 0 0
   3.19%  reason EPT_MISCONFIG rip 0xffffffff81a4243a info 0 0
   2.87%  reason IO_INSTRUCTION rip 0xffffffff81b0759a info 3f80000 0
   2.16%  reason EPT_VIOLATION rip 0xffffffff823ef54b info d82 0
   1.64%  reason IO_INSTRUCTION rip 0xffffffff81b07558 info 3fd0008 0
   1.44%  reason MSR_WRITE rip 0xffffffff811121a2 info 0 0
```

打开时钟之后:

```txt
Samples: 10K of event 'kvm:kvm_exit', 1 Hz, Event count (approx.): 7073 lost: 0/0 drop: 0/0
Overhead  Trace output
  59.47%  reason MSR_WRITE rip 0xffffffff8110ac58 info 0 0
  29.76%  reason HLT rip 0xffffffff823fa01e info 0 0
   2.38%  reason PREEMPTION_TIMER rip 0xffffffff8110ac5a info 0 0
   1.05%  reason PREEMPTION_TIMER rip 0xffffffff812346f6 info 0 0
   0.96%  reason MSR_WRITE rip 0xffffffff811121a2 info 0 0
   0.79%  reason IO_INSTRUCTION rip 0xffffffff81b0759a info 3f90000 0
   0.79%  reason PREEMPTION_TIMER rip 0xffffffff823f8ee0 info 0 0
   0.55%  reason PREEMPTION_TIMER rip 0xffffffff81223221 info 0 0
   0.51%  reason PREEMPTION_TIMER rip 0xffffffff8122321d info 0 0
   0.51%  reason PREEMPTION_TIMER rip 0xffffffff8122335a info 0 0
   0.49%  reason PREEMPTION_TIMER rip 0xffffffff8110ac5c info 0 0
```

可以看到 MSR_WRITE 的比例明显上升，相当的有趣的

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
