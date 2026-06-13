## 其中部分内容分析到
https://github.com/ChinaLinuxKernel/CLK/blob/master/CLK2021/3-1%20AMD%E6%9E%B6%E6%9E%84%E8%99%9A%E6%8B%9F%E6%9C%BA%E6%80%A7%E8%83%BD%E6%8E%A2%E7%B4%A2%E4%B8%8E%E5%AE%9E%E8%B7%B5.pdf

其他的内容也可以看看:
- https://github.com/ChinaLinuxKernel/CLK/tree/master

## 很多机器默认不打开的
具体看 : avic_want_avic_enabled

## svm_enable_irq_window


## vgif
```c
/* enable/disable Virtual GIF */
int vgif = true;
module_param(vgif, int, 0444);
```

原来是用来屏蔽 SMM 的 :
https://stackoverflow.com/questions/59530216/how-to-disable-software-smi-system-management-interrupt-in-windows

## x2apic 和 x2avic 是啥关系?

## 到底中断都是怎么注入的
```txt
@[
    svm_complete_interrupt_delivery+5
    __apic_accept_irq+298
    kvm_irq_delivery_to_apic_fast+320
    kvm_irq_delivery_to_apic+103
    kvm_apic_send_ipi+175
    kvm_x2apic_icr_write+38
    handle_fastpath_set_msr_irqoff+399
    kvm_arch_vcpu_ioctl_run+1412
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 976
@[
    svm_complete_interrupt_delivery+5
    __apic_accept_irq+298
    kvm_apic_local_deliver+127
    kvm_inject_apic_timer_irqs+43
    kvm_arch_vcpu_ioctl_run+2349
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 1168
```

虚拟机中也是这样的，很好没有太大的区别:
```txt
@[
    svm_complete_interrupt_delivery+5
    __apic_accept_irq+298
    kvm_apic_local_deliver+127
    kvm_inject_apic_timer_irqs+43
    kvm_arch_vcpu_ioctl_run+2349
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+194
    entry_SYSCALL_64_after_hwframe+121
]: 448
```

```c
void svm_complete_interrupt_delivery(struct kvm_vcpu *vcpu, int delivery_mode,
				     int trig_mode, int vector)
{
	/*
	 * apic->apicv_active must be read after vcpu->mode.
	 * Pairs with smp_store_release in vcpu_enter_guest.
	 */
	bool in_guest_mode = (smp_load_acquire(&vcpu->mode) == IN_GUEST_MODE);

	/* Note, this is called iff the local APIC is in-kernel. */
	if (!READ_ONCE(vcpu->arch.apic->apicv_active)) {
		/* Process the interrupt via kvm_check_and_inject_events(). */
		kvm_make_request(KVM_REQ_EVENT, vcpu);
		kvm_vcpu_kick(vcpu);
		return;
	}

	trace_kvm_apicv_accept_irq(vcpu->vcpu_id, delivery_mode, trig_mode, vector);
	if (in_guest_mode) {
		/*
		 * Signal the doorbell to tell hardware to inject the IRQ.  If
		 * the vCPU exits the guest before the doorbell chimes, hardware
		 * will automatically process AVIC interrupts at the next VMRUN.
		 */
		avic_ring_doorbell(vcpu);
	} else {
		/*
		 * Wake the vCPU if it was blocking.  KVM will then detect the
		 * pending IRQ when checking if the vCPU has a wake event.
		 */
		kvm_vcpu_wake_up(vcpu);
	}
}
```


执行路径居然是这样的，
```txt
 26)               |  svm_complete_interrupt_delivery [kvm_amd]() {
 26)               |    kvm_vcpu_kick [kvm]() {
 26)               |      rcuwait_wake_up() {
 26)   0.200 us    |        __rcu_read_lock();
 26)               |        wake_up_process() {
 26)               |          try_to_wake_up() {
 26)   0.131 us    |            _raw_spin_lock_irqsave();
 26)               |            select_task_rq_fair() {
 26)   0.120 us    |              __rcu_read_lock();
 26)   0.110 us    |              available_idle_cpu();
 26)   0.291 us    |              available_idle_cpu();
 26)   0.110 us    |              available_idle_cpu();
 26)   0.141 us    |              __rcu_read_unlock();
 26)   2.034 us    |            }
 26)               |            ttwu_queue_wakelist() {
 26)               |              __smp_call_single_queue() {
 26)   0.270 us    |                call_function_single_prep_ipi();
 26)               |                native_send_call_func_single_ipi() {
 26)   0.240 us    |                  default_send_IPI_single_phys();
 26)   0.501 us    |                }
 26)   1.393 us    |              }
 26)   2.385 us    |            }
 26)   0.120 us    |            _raw_spin_unlock_irqrestore();
 26)   5.941 us    |          }
 26)   6.231 us    |        }
 26)   0.090 us    |        __rcu_read_unlock();
 26)   6.943 us    |      }
 26)   7.213 us    |    }
 26)   8.155 us    |  }
```

原来居然是 avic 默认是没有打开，但是打开了似乎也没什么特殊的东西:
```c
/*
 * enable / disable AVIC.  Because the defaults differ for APICv
 * support between VMX and SVM we cannot use module_param_named.
 */
static bool avic;
module_param(avic, bool, 0444);
```
好吧，只是没有想到，的确使用的是这么简陋的方法。

## 啊！ 为什么 enable_apicv 没有打开啊 ！
哦，原来虚拟机中根本就没有 avic

```sh
	enable_apicv = avic = avic && avic_hardware_setup();
```

## 似乎大多数的代码处理
- avic_incomplete_ipi_interception
- avic_unaccelerated_access_interception

```txt
	[SVM_EXIT_AVIC_INCOMPLETE_IPI]		= avic_incomplete_ipi_interception,
	[SVM_EXIT_AVIC_UNACCELERATED_ACCESS]	= avic_unaccelerated_access_interception,
```

但是是因为没有设置 cat /sys/module/kvm_amd/parameters/avic

## avic 没有打开的时候，不会去触发，那么默认的场景是什么样子的

难道是 apic 来模拟的吗?

## 到底中断是如何注入的？
- kvm_arch_vcpu_create : 到底时不时正确的做法?
  - 我很迷茫！

## 在 amd 中观测到了这么 exit
```txt
@[
    apic_update_ppr+1
    apic_set_eoi+143
    kvm_lapic_sync_from_vapic+259
    kvm_arch_vcpu_ioctl_run+3804
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+184
    entry_SYSCALL_64_after_hwframe+119
]: 659
@[
    apic_update_ppr+1
    kvm_set_cr8+32
    svm_vcpu_run+1260
    kvm_arch_vcpu_ioctl_run+1376
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+184
    entry_SYSCALL_64_after_hwframe+119
]: 2768
```

## 为什么 avic 默认关闭的
server 上也是吗?

## 为什么打开了 avic ，还是可以观测到大量的 apic_update_ppr

```txt
@[
    apic_update_ppr+1
    apic_set_eoi+143
    kvm_lapic_sync_from_vapic+259
    kvm_arch_vcpu_ioctl_run+3804
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+184
    entry_SYSCALL_64_after_hwframe+119
]: 124
@[
    apic_update_ppr+1
    kvm_set_cr8+32
    svm_vcpu_run+1260
    kvm_arch_vcpu_ioctl_run+1376
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+184
    entry_SYSCALL_64_after_hwframe+119
]: 467
```

- svm_vcpu_run 中
  - sync_cr8_to_lapic 中 svm_is_intercept 的返回结果为 false ，导致继续向下走
      - kvm_set_cr8


## 如何理解 cr8 intercept ?
vcpu_enter_guest 中
```c
	if (kvm_check_request(KVM_REQ_EVENT, vcpu) || req_int_win ||
	    kvm_xen_has_interrupt(vcpu)) {
      // ...
		if (kvm_lapic_enabled(vcpu)) {
			update_cr8_intercept(vcpu);
			kvm_lapic_sync_to_vapic(vcpu);
		}
	}
```
update_cr8_intercept -> svm_update_cr8_intercept

这里是在动态的计算，到底是为了什么?

## 为什么 avic 打开的时候， kvm_guest_apic_eoi_write 中可以看到，还是走的 KVM_PV_EOI_BIT

## 看看这个
- https://lpc.events/event/17/contributions/1524/attachments/1372/2976/12%20Secure-AVIC_LPC2023_20231113.pdf

## cat /sys/module/kvm_amd/parameters/avic
如何理解?

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
