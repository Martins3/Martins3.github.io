## Documentation/timers/

### Documentation/timers/highres.rst
完全没更新

### Documentation/timers/hrtimers.rst

理解下，为什么还有人使用 jiifies 的 timer 的?

### Documentation/timers/no_hz.rst


### Documentation/timers/timekeeping.rst
- https://www.kernel.org/doc/html/latest/timers/timekeeping.html

### 细节
- Documentation/timers/delay_sleep_functions.rst : 如何插入一个 delay
  - https://www.kernel.org/doc/html/latest/timers/delay_sleep_functions.html
- Documentation/timers/hpet.rst : 还有 hpte

## Documentation/core-api/timekeeping.rst

## Documentation/virt/kvm/x86/timekeeping.rst

2. X86_FEATURE_TSC_RELIABLE : 用来说明什么?

- kvm_arch_tsc_set_attr

tsc 含有很多属性，这都是 13900k 含有的:

- tsc
- rdtscp
- constant_tsc
- nonstop_tsc
- tsc_known_freq
- tsc_deadline_timer
- tsc_adjust

kvm 相关的有:
tsc_scaling
tsc_offset

### Documentation/virt/kvm/devices/vcpu.rst
https://www.kernel.org/doc/html/latest/virt/kvm/devices/vcpu.html#group-kvm-vcpu-tsc-ctrl

> 4. GROUP: KVM_VCPU_TSC_CTRL 中描述了

我靠，这是在说什么啊？

# timer

## 时钟

https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/virtualization_deployment_and_administration_guide/chap-kvm_guest_timing_management

Guest virtual machines without accurate time keeping may experience issues with network applications and processes, as session validity, migration, and other network activities rely on timestamps to remain correct.

KVM avoids these issues by providing guest virtual machines with a paravirtualized clock (kvm-clock).
However, it is still important to test timing before attempting activities that may be affected by time keeping inaccuracies, such as guest _migration_.

By default, the guest synchronizes its time with the hypervisor as follows:

- When the guest system boots, the guest reads the time from the emulated Real Time Clock (RTC).
- When the NTP protocol is initiated, it automatically synchronizes the guest clock. Afterwards, during normal guest operation, NTP performs clock adjustments in the guest.
- When a guest is resumed after a pause or a restoration process, a command to synchronize the guest clock to a specified value should be issued by the management software (such as virt-manager). This synchronization works only if the QEMU guest agent is installed in the guest and supports the feature. The value to which the guest clock synchronizes is usually the host clock value.

- [ ] trace code :
  - [ ] read emulated RTC
  - [ ] NTP synchronize guest clock
  - [ ] resumed / pause : qemu synchronize guest clock

Modern Intel and AMD CPUs provide a constant Time Stamp Counter (TSC).
The count frequency of the constant TSC does **not vary** when the CPU core itself changes frequency, for example to comply with a power-saving policy.
A CPU with a **constant TSC frequency** is necessary in order to use the TSC as a clock source for KVM guests.

## 找到代码证据，从 cmos 中获取到系统时间是如何实现的

kvmclock_init 中注册了，类似的地方也是注册了 cmos 的获取到 wallclock

```c
	x86_platform.get_wallclock = kvm_get_wallclock;
```

## ..
https://oenhan.com/kvm-pv-kvmclock-tsc

## 这是在说一个事情吗?
- https://lore.kernel.org/lkml/20210303122657.23400-1-ann.zhuangyanying@huawei.com/T/#mb1d36bbd230ff8041d2d1ade8c2b101e70cc00ac
- https://lore.kernel.org/lkml/87o8dxf597.ffs@nanos.tec.linutronix.de/T/
## wowotech
http://www.wowotech.net/timer_subsystem/time-subsyste-architecture.html

## suse
### 1
https://www.suse.com/c/cpu-isolation-introduction-part-1/

It performs many jobs:

- Run expired general purpose timer callbacks
- Elapse posix CPU timers and run those that have expired
- Timekeeping: maintain internal clock (jiffies) and external clock (gettimeofday())
- Scheduler: maintain internal state, fairness and priorities (task preemption)
- Maintain global load average
- Maintain perf events, etc……

### 6
https://www.suse.com/c/cpu-isolation-nohz_full-troubleshooting-tsc-clocksource-by-suse-labs-part-6/


## 勉强看看
https://github.com/0xAX/linux-insides/tree/master/Timers

## 附带
- https://github.com/dterei/tsc
  - 两个指令
- https://en.wikipedia.org/wiki/Intel_8253

## vmware 的总结
https://www.vmware.com/files/pdf/techpaper/Timekeeping-In-VirtualMachines.pdf

## blog
https://tcbbd.moe/linux/qemu-kvm/kvm-time/ : 基本正确

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
