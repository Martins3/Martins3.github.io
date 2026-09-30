## sysfs
1. /proc/timer_list 接口可以获得内核中的时间子系统的相关信息。例如：系统中的当前正在使用的 clock source 设备、clock event 设备和 tick device 的信息。

```txt
cat /sys/devices/system/clocksource/clocksource0/current_clocksource
cat /sys/devices/system/clocksource/clocksource0/available_clocksource
```

## stop cont

配合 hmp stop cont

## 修改 kvm : 让 __pvclock_read_cycles 的返回值不是从 0 开始的
```diff
diff --git a/arch/x86/kvm/x86.c b/arch/x86/kvm/x86.c
index ffe580169c93..de9f5da27adb 100644
--- a/arch/x86/kvm/x86.c
+++ b/arch/x86/kvm/x86.c
@@ -3251,7 +3252,7 @@ static int kvm_guest_time_update(struct kvm_vcpu *v)
 	}

 	vcpu->hv_clock.tsc_timestamp = tsc_timestamp;
-	vcpu->hv_clock.system_time = kernel_ns + v->kvm->arch.kvmclock_offset;
+	vcpu->hv_clock.system_time = kernel_ns ;
 	vcpu->last_guest_tsc = tsc_timestamp;

 	/* If the host uses TSC clocksource, then it is stable */
```
guest os 启动之后，执行 for i in {1..100}; do date && sleep 1 ; done 可以发现。开机之后，
其时间 date 输出的时间并不正确，但是随后 ntp 会将时间修正为正确的。

### boottime clock 时间会受 ntp 影响吗?

不受影响。
1. 因为 ktime_get 也就是 boottime ，这里只是 offset 修改了
2. 而且 ntp 修改的是 tk->xtime_sec ，所以可以将 date 获取的时间校准正确，但是不会影响 boottime 。
```txt
[    6.690302] ktime_get 6686559473
[    6.690416] ktime_get_real_ts64 sec:1717375593 tv_sec:779030012
[   13.537964] kvmclock_store : action = 1 current=bash
[   13.538245] ktime_get 13539973508
[   13.538401] ktime_get_real_ts64 sec:1716361182 tv_sec:708860525
[   14.900529] kvmclock_store : action = 1 current=bash
[   14.900768] ktime_get 14906814641
[   14.900895] ktime_get_real_ts64 sec:1716361184 tv_sec:75671308
[   15.719889] kvmclock_store : action = 1 current=bash
[   15.720124] ktime_get 15726171443
[   15.720246] ktime_get_real_ts64 sec:1716361184 tv_sec:895023827
```

### 无论 clocksource 是 tsc 还是 kvm-clock ，以上结果都是如此，如何理解！

- kvm_guest_time_update 调整 kvm 后，将 clocksource 从 kvm-clock 切换为
tsc 之后，还是可以测试上到上述的效果。

因为在 tsc 的模式下，wallclock 也是从 kvmclock 哪里获取的

如果这个时候将 kvmclock 完全禁用掉，也就是这样的:
```txt
	kernel_args+="clocksource=tsc "
	kernel_args+="no-kvmclock "
```

tsc 的时钟是不受影响的。

### 在运行的过程中，如果刷新了 master clock ，效果如何?
当前问题的主线还是，看上去每次刷新的时候，都是在使用 host clock 刷新 guest clock ，也是导致问题的原因吗?

## [ ] 修改 kvm : 让 __pvclock_read_cycles 的返回值受 host 的 kvm_guest_time_update 影响

增加 sysfs 接口，然后修改 kvm_guest_time_update 。


## [x] 调整 tsc 的频率，让 CLOCK_MONOTONIC_RAW 的时钟错误，但是 realtime 的时间是正确的

code/module/c/clock_time.c

1. 开始的时候
```txt
➜  c ./clock_time.out
CLOCK_REALTIME : 1717321766.319 (19876 days +  9h 49m 26s)
CLOCK_TAI      : 1717321766.319 (19876 days +  9h 49m 26s)
CLOCK_MONOTONIC:       1192.710 ( 0h 19m 52s)
CLOCK_BOOTTIME :       1192.710 ( 0h 19m 52s)
CLOCK_MONOTONIC_RAW:       1309.280 ( 0h 21m 49s)
```

2. 等待一段时间之后
```txt
CLOCK_REALTIME : 1717322190.512 (19876 days +  9h 56m 30s)
CLOCK_TAI      : 1717322190.512 (19876 days +  9h 56m 30s)
CLOCK_MONOTONIC:       1616.903 ( 0h 26m 56s)
CLOCK_BOOTTIME :       1616.903 ( 0h 26m 56s)
CLOCK_MONOTONIC_RAW:       1780.605 ( 0h 29m 40s)
```

3. 使用 ntp 校准时间:
```txt
➜  c ./clock_time.out
CLOCK_REALTIME : 1717321972.836 (19876 days +  9h 52m 52s)
CLOCK_TAI      : 1717321972.836 (19876 days +  9h 52m 52s)
CLOCK_MONOTONIC:       1715.272 ( 0h 28m 35s)
CLOCK_BOOTTIME :       1715.272 ( 0h 28m 35s)
CLOCK_MONOTONIC_RAW:       1889.904 ( 0h 31m 29s)
```

忽然，所有的 clocksource 的问题都理解了。
1. ntp 只能校准 realtime ，校准的方法是调整 realtime 的 base 时间。
2. 但是 CLOCK_MONOTONIC_RAW 是不能校准的，因为这是定义。

## [x] 有办法知道不进行 ntpd 的校准，CLOCK_MONOTONIC_RAW 的时间的差距吗?

使用 code/module/c/clock_time.c ，然后找到这个机器到底是什么时候启动的（获取到启动的日志)

## 在 asahi linux 下观察到的

```sh
cat /sys/devices/system/clocksource/clocksource0/current_clocksource
cat /sys/devices/system/clocksource/clocksource0/available_clocksource
```

```txt
arch_sys_counter
arch_sys_counter
```

1. arm guest linux : 同上
2. arm host linux : 同上

实在是有趣的设计！

## [x] gdb 调试导致虚拟机暂停，那么 date 会自动跟进吗？

无论是 arm 还是 x86 都是存在一个问题，date 的时间会出现扭曲的。换言之，
长时间无法将时钟校准回来。

会的，自动更新为最新的数值。

两个场景都是都出现过，但是自动更新才是争取的。

**完全混乱了**
### arm 和 x86 在 hmp stop cont

分别测试
```txt
for i in {1..100}; do ./clock_time.out  && sleep 1  ; done
```

```txt
./time-jump.out
```

```txt
for i in {1..100}; do date && sleep 1 ; done
```
时间是连续的，就像是没有事情发生一样。

### 如果是 gdb 来控制，时间会自动同步上

### 如果是 systemctl suspend，wakeup 之后，时间会自动同步上

也是没有问题的
```txt
./time-jump.out
```

## windows 的时间进行测试下
使用 qemu 的 rtc 选项。

## 在 guest 中 hacking kvm_clock_get_cycles 的输出

### 让 kvm_clock_get_cycles 向后 jump

结果会出现这个错误，而且 sleep 时间会显示为 0:
```txt
clocksource: Long readout interval, skipping watchdog check: cs_nsec: 25205330173 wd_nsec: 495960828
```

并且 sleep 5 会变成 sleep 5 + jump 的时间。 ????


看上去 sleep 不是卡到 do_nanosleep 的
```txt
➜  ~ cat /proc/2023/stack
[<0>] do_signal_stop+0x214/0x240
[<0>] get_signal+0x32c/0x780
[<0>] arch_do_signal_or_restart+0x8e/0x2a0
[<0>] syscall_exit_to_user_mode+0x8a/0x280
[<0>] do_syscall_64+0xfc/0x210
[<0>] entry_SYSCALL_64_after_hwframe+0x77/0x7f
```


### 让 kvm_clock_get_cycles 向前 jump
```txt
[   97.839772] rcu: INFO: rcu_preempt self-detected stall on CPU
[   97.840032] rcu:     0-...!: (1 ticks this GP) idle=5b6c/1/0x4000000000000000 softirq=10835/10835 fqs=0
[   97.840395] rcu:     (t=49418 jiffies g=5713 q=31 ncpus=2)
[   97.840591] rcu: rcu_preempt kthread timer wakeup didn't happen for 49416 jiffies! g5713 f0x0 RCU_GP_WAIT_FQS(5) ->state=0x402
[   97.841011] rcu:     Possible timer handling issue on cpu=0 timer-softirq=1384
[   97.841272] rcu: rcu_preempt kthread starved for 49420 jiffies! g5713 f0x0 RCU_GP_WAIT_FQS(5) ->state=0x402 ->cpu=0
[   97.841657] rcu:     Unless rcu_preempt kthread gets sufficient CPU time, OOM is now expected behavior.
[   97.841994] rcu: RCU grace-period kthread stack dump:
[   97.842185] task:rcu_preempt     state:I stack:14448 pid:16    tgid:16    ppid:2      flags:0x00004000
[   97.842533] Call Trace:
[   97.842628]  <TASK>
[   97.842712]  __schedule+0x53b/0x1390
[   97.842858]  ? update_curr+0x15d/0x3c0
[   97.843001]  ? _raw_spin_lock_irqsave+0x40/0xa0
[   97.843176]  ? preempt_count_sub+0x4b/0x60
[   97.843331]  ? schedule_timeout+0xbb/0x1b0
[   97.843486]  schedule+0xca/0x170
[   97.843610]  schedule_timeout+0xbb/0x1b0
[   97.843759]  ? __pfx_process_timeout+0x10/0x10
[   97.843928]  rcu_gp_fqs_loop+0x1c4/0x910
[   97.844077]  ? _raw_spin_unlock_irq+0x29/0x50
[   97.844251]  ? preempt_count_sub+0x4b/0x60
[   97.844407]  rcu_gp_kthread+0x26/0x260
[   97.844539]  ? __pfx_rcu_gp_kthread+0x10/0x10
[   97.844695]  kthread+0xf8/0x120
[   97.844814]  ? __pfx_kthread+0x10/0x10
[   97.844949]  ret_from_fork+0x37/0x50
[   97.845082]  ? __pfx_kthread+0x10/0x10
[   97.845223]  ret_from_fork_asm+0x1a/0x30
[   97.845368]  </TASK>
[   97.845454] CPU: 0 PID: 764 Comm: kmsg Not tainted 6.9.1 #19
[   97.845662] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[   97.845903] RIP: 0010:__se_sys_clock_nanosleep+0x11d/0x160
[   97.846108] Code: 00 60 3e 18 81 31 d2 48 85 c0 0f 95 c2 89 91 dc 05 00 00 48 89 81 e0 05 00 00 4d 8b 5c 24 30 48 89 e2 44 89 f7 89 de 41 ff d3 <0f> 1f 00 48 98 eb 07 48 c7 c0 a1 ff ff ff 65 48 8b 0c 25 28 00 00
[   97.846773] RSP: 0018:ffffc90011b4bec0 EFLAGS: 00000286
[   97.846957] RAX: 0000000000000000 RBX: 0000000000000000 RCX: 3a36686ac0ca0300
[   97.847207] RDX: 000000000001ceac RSI: ffffffff824eaeba RDI: ffffc90011b4be40
[   97.847448] RBP: ffffc90011b4bf48 R08: ffff8881204a6c00 R09: 0000000000000001
[   97.847694] R10: 00000016c7b45e24 R11: 0000000000000000 R12: ffffffff8282a470
[   97.847934] R13: ffff8881204a6c00 R14: 0000000000000000 R15: 00007ffff82da070
[   97.848195] FS:  00007f5152785740(0000) GS:ffff88813ae00000(0000) knlGS:0000000000000000
[   97.848464] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[   97.848656] CR2: 00007ff184568010 CR3: 0000000105aae000 CR4: 0000000000750ef0
[   97.848895] PKRU: 55555554
[   97.848989] Call Trace:
[   97.849075]  <IRQ>
[   97.849147]  ? rcu_dump_cpu_stacks+0xe0/0x140
[   97.849299]  ? print_cpu_stall+0x162/0x2d0
[   97.849437]  ? rcu_sched_clock_irq+0x36a/0x660
[   97.849588]  ? __cgroup_account_cputime_field+0x6b/0x90
[   97.849757]  ? update_process_times+0x75/0xa0
[   97.849899]  ? tick_nohz_handler+0xc4/0x120
[   97.850034]  ? __pfx_tick_nohz_handler+0x10/0x10
[   97.850208]  ? __hrtimer_run_queues+0xff/0x2c0
[   97.850361]  ? hrtimer_interrupt+0xf4/0x390
[   97.850505]  ? __sysvec_apic_timer_interrupt+0x4f/0x160
[   97.850683]  ? sysvec_apic_timer_interrupt+0x71/0x90
[   97.850856]  </IRQ>
[   97.850932]  <TASK>
[   97.851007]  ? asm_sysvec_apic_timer_interrupt+0x1a/0x20
[   97.851191]  ? do_nanosleep+0x5a/0x140
[   97.851554]  ? __se_sys_clock_nanosleep+0x11d/0x160
[   97.851723]  do_syscall_64+0xef/0x210
[   97.851850]  ? exc_page_fault+0xb2/0x1f0
[   97.851986]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
[   97.852166] RIP: 0033:0x7f5152853b97
[   97.852322] Code: 1f 40 00 f3 0f 1e fa 83 ff 03 74 77 83 ff 02 b8 fa ff ff ff 49 89 ca 0f 44 f8 80 3d 8a a8 10 00 00 74 10 b8 e6 00 00 00 0f 05 <f7> d8 c3 66 0f 1f 44 00 00 48 83 ec 28 48 89 54 24 10 89 74 24 0c
[   97.853059] RSP: 002b:00007ffff82da058 EFLAGS: 00000202 ORIG_RAX: 00000000000000e6
[   97.853334] RAX: ffffffffffffffda RBX: ffffffffffffff80 RCX: 00007f5152853b97
[   97.853574] RDX: 00007ffff82da070 RSI: 0000000000000000 RDI: 0000000000000000
[   97.853815] RBP: 0000000000000071 R08: 000000000000006d R09: 0000000000000000
[   97.854054] R10: 00007ffff82da070 R11: 0000000000000202 R12: 00007ffff82da218
[   97.854298] R13: 00000000004011e0 R14: 0000000000000000 R15: 00007f51529ae000
[   97.854538]  </TASK>
```

不受影响的原因是因为 vdso 吗?


### 修改时间之后, shutdown 无法正常工作
```txt
➜  ~ cat /sys/kernel/test/foo

100000000000000
➜  ~ shutdown now
```

## 无则加勉
- 调用 sleep 参数，那些时间是可以被修改?


## 默认启动之后，不去修改 Guest os 的频率
```txt
[84856.872484] [read tsc value :show_host_tsc:6] 2995200
```

虚拟机中的频率是:
```txt
[   24.751257] [read tsc value :show_host_tsc:6] 2995200
```
## 最后一战

### 原来，暂停虚拟机也是会更新 masterclock 的

在 qemu 中，使用 hmp 的 stop 和 cont，

可以发现 kvm 中的 kvm_set_guest_paused 被调用，同时 kernel 中可以找到如下日志:
```txt
Authorized users only. All activities may be monitored and reported.
bogon login: [   30.789843] clocksource: Long readout interval, skipping watchdog check: cs_nsec: 2347897591 wd_nsec: 504001571
```
按道理不应该，因为虚拟机恢复之后，系统的时间应该继续运行。


### qemu 居然真的在周期性的刷新这个代码

这是什么恐怖故事
```txt
+ sudo bpftrace -e 'kprobe:kvm_guest_time_update { @[kstack(bpftrace)] = count(); }'
[sudo] password for martins3:
Attaching 1 probe...
^C

@[
    kvm_guest_time_update+1
    vcpu_enter_guest.constprop.0+1657
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+555
    __x64_sys_ioctl+153
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 32
```

## sleep(1) 的时候，sleep 使用的时间是多少?

- do_nanosleep
  - hrtimer_sleeper_start_expires -> 最后到达
```c
/*
 * Program the next event, relative to now
 */
static int lapic_next_event(unsigned long delta,
			    struct clock_event_device *evt)
{
	apic_write(APIC_TMICT, delta);
	return 0;
}

static int lapic_next_deadline(unsigned long delta,
			       struct clock_event_device *evt)
{
	u64 tsc;

	/* This MSR is special and need a special fence: */
	weak_wrmsr_fence();

	tsc = rdtsc();
	wrmsrl(MSR_IA32_TSC_DEAkDLINE, tsc + (((u64) delta) * TSC_DIVISOR));
	return 0;
}
```
所以，无论 kvm-clcok 如何跳变，都是应该是 1s 之后醒过来才对。


实际上，`lapic_next_event` 根本不被调用.

`lapic_next_deadline` 才是正确的，

## 曾经 periodic tick 更新了如下的东西 ，如果现在停止更新，那么就应该思考如何刷新下面的内容

- jiffies
- process accounting
- global load accounting
- timekeeping
- POSIX timers
- RCU callbacks
- hrtimers
- irq_work

1. 测试下到底什么时候是完全没有时钟 ticket 的
   sudo perf trace -e kvm:kvm_exit -t 3023637

## 看看这个函数在时间下的影响
timekeeping_resume

## 让机器在 smm 中暂停一会会有什么效果
时钟可以跳变吗?

## 部分测试在这里
docs/concurrent/rcu/stall.md

## 如果调整 tsc ，那么 rdtsc 结果会变化，但是 kvm-clock 的结果不会变化，是吗?

## 测试在 mono 和 raw 在暂停的时候的时间跳变问题

### 测试程序

### 现象

https://docs.google.com/document/d/1wWlpYYPzKYd-Q2HmTPOsMmDyTpLWq3R3NtpaHI828ZA/edit?tab=t.0#heading=h.6n80jnry5p0x

的确我们可以非常的容易复现这个问题:
```txt
Seconds: 249, Nanoseconds: 182197523
Seconds: 249, Nanoseconds: 192365117
Seconds: 249, Nanoseconds: 202539249
Seconds: 249, Nanoseconds: 212731253
Seconds: 249, Nanoseconds: 222891607
Seconds: 277, Nanoseconds: 664157493 # 暂停之后，mono 是直接跳了

found bug
Seconds: 277, Nanoseconds: 664157493
Seconds: 249, Nanoseconds: 304922850
```

1. 和时钟源没有关系
```txt
cat /sys/devices/system/clocksource/clocksource0/current_clocksource
cat /sys/devices/system/clocksource/clocksource0/available_clocksource

kvm-clock
kvm-clock tsc acpi_pm
```

2. 和 CLOCK_MONOTONIC_RAW 和 CLOCK_MONOTONIC 没有关系

### 使用 src/m 中 test_kernel_clock_jump 可以发现问题

当 clocksource 是 tsc 的时候:
```txt
[  234.464404] raw      : 358 331 382 631
[  234.464511] real     : 1746927085553012985
[  234.464609] clocktai : 1746927085553111096
[  234.464707] boottime : 358678310766
[  234.464785] mono     : 358678389575
[  234.464864] mono     : 358678468307
[  234.464957] rdtsc    : 359145418782

# qhm 将虚拟机暂停之后

[  234.551737] raw      : 407 167 615 912
[  234.551849] real     : 1746927134389304483
[  234.551961] clocktai : 1746927134389415559
[  234.552128] boottime : 407514685288
[  234.552213] mono     : 407514769544
[  234.552301] mono     : 407514857836
[  234.552385] rdtsc    : 407981700121
```

1. 如何解释这里的时间戳是不变的
2. 切换为 kvm-clock 会不同吧，毕竟有自动保存和恢复
  - 但是，无法解释 tsc 在暂停恢复的时候也有啊

切换为 kvmclock 之后:
```txt
[   49.223813] raw      : 48 893 576 819
[   49.223939] real     : 1746939909041837933
[   49.224035] clocktai : 1746939909041934664
[   49.224132] boottime : 49279338357
[   49.224208] mono     : 49 279 413 702
[   49.224283] rdtsc    : 148948709871

// 暂停

[   49.327805] raw      : 48 997 569 524
[   49.327916] real     : 1746939909145815909
[   49.328026] clocktai : 1746939909145925286
[   49.328130] boottime : 49383336731
[   49.328213] mono     : 49 383 419 187
[   49.328298] rdtsc    : 822579077949
```

可以看到，rdtsc 的结果出现了很大的变化，但是
raw 和 mono 几乎没有变化的，这个是符合预期的。


出现问题的时候，内核一定会出现错误吗?
```txt
clocksource: Long readout interval, skipping watchdog check: cs_nsec: 480551464902 wd_nsec: 468071567
```
似乎不是


### 似乎是 vdso 的问题啊

1. guest os 的 clocksource 为 kvmclock ，在用户态直接 rdtsc 不会有问题，内核中 ktime_get 也不会有问题
2. 将 guest os 中的时钟源替换为 tsc 也不会有问题

所以，应该是 vdso 的问题，当更新 pvti 的时候出现错误

### KVM_REQ_MCLOCK_INPROGRESS 是做什么的，似乎完全没有用啊

#### 为什么 vdso 的时间回退防护去掉了

4.19 还是有的，还是说移动到了一个特定的位置

但是这个刷新不够及时:
```c
#ifdef CONFIG_PARAVIRT_CLOCK
/*
 * This is the vCPU 0 pvclock page.  We only use pvclock from the vDSO
 * if the hypervisor tells us that all vCPUs can get valid data from the
 * vCPU 0 page.
 */
extern struct pvclock_vsyscall_time_info pvclock_page
	__attribute__((visibility("hidden")));
#endif
```

这个地址想不到这么
```c
	pvclock_page = vclock_pages + VDSO_PAGE_PVCLOCK_OFFSET * PAGE_SIZE;
```
在 vvar_vclock_fault ，然后找到，是在 kvmclock_init 中

```c
	this_cpu_write(hv_clock_per_cpu, &hv_clock_boot[0]);
	kvm_register_clock("primary cpu clock");
	pvclock_set_pvti_cpu0_va(hv_clock_boot);
```

```c
static __always_inline struct pvclock_vcpu_time_info *this_cpu_pvti(void)
{
	return &this_cpu_read(hv_clock_per_cpu)->pvti;
}
```

所以，这里的问题在于，只有在 CPU 0 运行的时候才可以知道

### 暂停恢复的时候，tsc 作为系统 MSR 寄存器，并不会

实际上，qhm 下 stop / cont 的时候完全不会去调用 kvm_set_msr_common 。
不过这个也是合理的， 既然状态本来就是在内核中，那么就不会去写了。

但是，热迁移的时候回去设置的 tsc 的。

这是一个 bug 吧，为什么不去设置呢？

### [ ] vmware 中暂停恢复会有时间跳变吗?

## 新的 bug 

如果让时间回退，那么
```txt
        data.clock = s->clock - 10 * NANOSECONDS_PER_SECOND;
```



## 既然 kvmclock 会去 touch watchdog ， 那么还是会出发 rcu stall 
内核模块测试的那个并不是导致问题的原因，那个没有感受到时间跳变。
而且奇怪的问题在于，既然 kvm-clock 不会导致时间跳变，为什么需要 touch watchdog ?

### 似乎 softlock up 和 hardlock up 不会被时间向前跳变影响的

因为是通过多次中断来看的，所以不会影响的。

## 到底什么环境中测试到的 rcustall 的?

## chronyd 的检测

### 时间大幅向前跳
```txt
May 12 23:31:40 localhost systemd[1]: Starting NTP client/server...
May 12 23:31:40 localhost chronyd[648]: chronyd version 4.3 starting (+CMDMON +NTP +REFCLOCK +RTC +PRIVDROP +SCFILTER +SIGN>
May 12 23:31:40 localhost chronyd[648]: Frequency -3.337 +/- 3.061 ppm read from /var/lib/chrony/drift
May 12 23:31:40 localhost systemd[1]: Started NTP client/server.
May 12 23:31:45 localhost.localdomain chronyd[648]: Selected source 202.118.1.130 (pool.ntp.org)
May 12 23:31:51 localhost.localdomain chronyd[648]: Source 162.159.200.123 replaced with 84.16.73.33 (pool.ntp.org)
May 12 23:33:38 localhost.localdomain chronyd[648]: Forward time jump detected!
May 12 23:33:38 localhost.localdomain chronyd[648]: Can't synchronise: no selectable sources
```

- 4.19 环境中检测是类似的:
```txt
-- Logs begin at Tue 2025-05-13 09:37:42 CST, end at Tue 2025-05-13 10:38:45 CST. --
May 13 09:37:43 localhost.localdomain systemd[1]: Starting NTP client/server...
May 13 09:37:43 localhost.localdomain chronyd[729]: chronyd version 3.4 starting (+CMDMON +NTP +REFCLOCK +RTC +PRIVDROP +SC
May 13 09:37:43 localhost.localdomain chronyd[729]: Frequency -2.273 +/- 0.781 ppm read from /var/lib/chrony/drift
May 13 09:37:43 localhost.localdomain systemd[1]: Started NTP client/server.
May 13 09:38:17 localhost.localdomain chronyd[729]: Selected source 119.28.183.184
May 13 10:38:30 localhost.localdomain chronyd[729]: Forward time jump detected!
May 13 10:38:30 localhost.localdomain chronyd[729]: Can't synchronise: no selectable sources
```

同时在内核中可以观察到:
```txt
[ 3649.725688] systemd-journald[2105]: File /run/log/journal/bcb849f7bc824fa9be90674843874a5d/system.journal corrupted or uncleanly shut down, renaming and replacing.
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
