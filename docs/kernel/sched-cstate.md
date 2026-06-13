## 主要的问题

1. 是如何选择的 driver 的，为什么最后选择的 intel_idle
2. governor 最终选择规则是什么
3. 进入到那个 state 是如何确定的

## 简单来说

- intel idle : 执行 mwait 指令
- acpi idle : 执行 mwait hlt 以及 ioport
- halt poll : halt 之前 poll 一段时间

> https://github.com/ChinaLinuxKernel/CLK/blob/master/CLK2021/3-1%20AMD%E6%9E%B6%E6%9E%84%E8%99%9A%E6%8B%9F%E6%9C%BA%E6%80%A7%E8%83%BD%E6%8E%A2%E7%B4%A2%E4%B8%8E%E5%AE%9E%E8%B7%B5.pdf

## 感觉 AMD crash 可能和这个有关

```txt
    cpuidle.off=1   [CPU_IDLE]
                        disable the cpuidle sub-system

    cpuidle.governor=
                    [CPU_IDLE] Name of the cpuidle governor to use.


    idle=           [X86]
                        Format: idle=poll, idle=halt, idle=nomwait
                        Poll forces a polling idle loop that can slightly
                        improve the performance of waking up a idle CPU, but
                        will use a lot of power and make the system run hot.
                        Not recommended.
                        idle=halt: Halt is forced to be used for CPU idle.
                        In such case C2/C3 won't be used again.
                        idle=nomwait: Disable mwait for CPU C-states
```

amd 的机器上:

```txt
🧀  cat /sys/devices/system/cpu/cpuidle/available_governors
menu
🧀  cat /sys/devices/system/cpu/cpuidle/current_driver
acpi_idle
```

先修改为 idle=halt 尝试一下

```txt
🧀  cat /sys/devices/system/cpu/cpuidle/available_governors
menu
🧀  cat /sys/devices/system/cpu/cpuidle/current_driver
none
```

设置命令行参数为:
processor.max_cstate=1 intel_idle.max_cstate=0

```txt
cat /sys/devices/system/cpu/cpuidle/current_driver
```

- [ ] 这个 max_cstate 可以动态修改吗?
- [ ] 这两个 cstate 是啥关系啊
     - [ ] https://jeremyeder.com/2012/11/14/processor-max_cstate-intel_idle-max_cstate-and-devcpu_dma_latency/

## 什么东西？？？？

https://groups.google.com/g/mechanical-sympathy/c/Ubm9_71ONTc

> 有这种事情?
>
> idle=poll will force c0 state, removing that would lock in to c1.

## https://access.redhat.com/solutions/2895271

tuned : 这是什么程序?

> 似乎代码有点问题，最后的解决方法是:
> intel_idle.max_cstate=0 processor.max_cstate=1 intel_pstate=disable

## cstate 状态表格

https://gist.github.com/Brainiarc7/8dfd6bb189b8e6769bb5817421aec6d1

## sleep 的等级

- https://docs.kernel.org/admin-guide/pm/sleep-states.html

## 怎么观察下，一般进入的都是什么 c state 状态?

### 介绍各种 /sys/power 的接口

## suspend 的时候

- 大致流程:
     - https://askubuntu.com/questions/792166/how-does-exactly-suspend-works-in-ubuntu
     - https://docs.kernel.org/admin-guide/pm/suspend-flows.html ：更加详细

1. 为什么 gnome 可以可以来设置系统的 suspend 时间
2. 而且 gnome 可以设置 power button 是关机还是 suspend
3. 通过按钮实现的 suspend 和通过 idel 的 suspend 是什么关系

- systemctl suspend
- systemctl hibernate

感觉 suspend 技术在 qemu 中不是太正常啊。

- systemd 中是什么角色
- kernel driver 的如何处理 pm 之类的

## s2idle

cpuidle_idle_call 中注释:

```c
	/*
	 * Suspend-to-idle ("s2idle") is a system state in which all user space
	 * has been frozen, all I/O devices have been suspended and the only
	 * activity happens here and in interrupts (if any). In that case bypass
	 * the cpuidle governor and go straight for the deepest idle state
	 * available.  Possibly also suspend the local tick and the entire
	 * timekeeping to prevent timer interrupts from kicking us out of idle
	 * until a proper wakeup interrupt happens.
	 */
```

只有中断才可以导致出来。

## 一共存在那些指令来进入睡眠

mwait
halt
pause

任何 cpu 都有 init 进程:

- start_secondary
- rest_init
  都会调用到 `cpu_startup_entry` 中

## 更加深度的睡眠为什么不可以

- https://mp.weixin.qq.com/s/0LM25OrpFCCcokSCMv--Mg
     - 大致分析 idle driver 的作用已经整个 idle 代码的流程

cat /sys/devices/system/cpu/cpuidle/available_governors

- cpuidle core
- cpuidle governors
- cpuidle drivers

- do_idle

     - cpu_idle_poll : 命令行参数可以选择这个
     - cpuidle_idle_call
          - cpuidle_get_device
          - cpuidle_get_cpu_driver
          - default_idle_call
          - idle_should_enter_s2idle
          - cpuidle_find_deepest_state
          - tick_nohz_idle_stop_tick : 如果
          - cpuidle_select
               - cpuidle_governor::select : 给定 driver 和 device，进行选择如何进入下一个 C state 状态。
          - call_cpuidle
               - cpuidle_enter
                    - cpuidle_enter_state
                         - tick_broadcast_enter : 告诉其他 CPU，我要进入睡眠了，到时候唤醒我
                         - cpuidle_state::enter

- cpuidle_get_device 是做啥的

问题:

1. 如果 nohz，是如何唤醒的?

## 看 driver 是如何注册的

```txt
#0  acpi_processor_power_init (pr=pr@entry=0xffff888101045400) at drivers/acpi/processor_idle.c:1377
#1  0xffffffff81849d02 in __acpi_processor_start (device=device@entry=0xffff888100f96800) at drivers/acpi/processor_driver.c:172
#2  0xffffffff81849d5d in acpi_processor_start (dev=<optimized out>) at drivers/acpi/processor_driver.c:203
#3  0xffffffff81ae65da in call_driver_probe (drv=0xffffffff83010220 <acpi_processor_driver>, dev=0xffff88833375b9e8) at drivers/base/dd.c:560
#4  really_probe (dev=dev@entry=0xffff88833375b9e8, drv=drv@entry=0xffffffff83010220 <acpi_processor_driver>) at drivers/base/dd.c:639
#5  0xffffffff81ae687d in __driver_probe_device (drv=drv@entry=0xffffffff83010220 <acpi_processor_driver>, dev=dev@entry=0xffff88833375b9e8) at drivers/base/dd.c:778
#6  0xffffffff81ae6909 in driver_probe_device (drv=drv@entry=0xffffffff83010220 <acpi_processor_driver>, dev=dev@entry=0xffff88833375b9e8) at drivers/base/dd.c:808
#7  0xffffffff81ae6b85 in __driver_attach (data=0xffffffff83010220 <acpi_processor_driver>, dev=0xffff88833375b9e8) at drivers/base/dd.c:1194
#8  __driver_attach (dev=0xffff88833375b9e8, data=0xffffffff83010220 <acpi_processor_driver>) at drivers/base/dd.c:1134
#9  0xffffffff81ae40a4 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff83010220 <acpi_processor_driver>, fn=fn@entry=0xffffffff81ae6b00 <__driver_attach>) at drivers/base/bus.c:301
#10 0xffffffff81ae5f89 in driver_attach (drv=drv@entry=0xffffffff83010220 <acpi_processor_driver>) at drivers/base/dd.c:1211
#11 0xffffffff81ae5885 in bus_add_driver (drv=drv@entry=0xffffffff83010220 <acpi_processor_driver>) at drivers/base/bus.c:618
#12 0xffffffff81ae7e7d in driver_register (drv=drv@entry=0xffffffff83010220 <acpi_processor_driver>) at drivers/base/driver.c:246
#13 0xffffffff8383ed65 in acpi_processor_driver_init () at drivers/acpi/processor_driver.c:266
#14 0xffffffff81001a40 in do_one_initcall (fn=0xffffffff8383ed40 <acpi_processor_driver_init>) at init/main.c:1306
#15 0xffffffff837eb28f in do_initcall_level (command_line=0xffff888100bb9800 "root", level=6) at init/main.c:1379
#16 do_initcalls () at init/main.c:1395
#17 do_basic_setup () at init/main.c:1414
#18 kernel_init_freeable () at init/main.c:1634
#19 0xffffffff82211895 in kernel_init (unused=<optimized out>) at init/main.c:1522
#20 0xffffffff81002999 in ret_from_fork () at arch/x86/entry/entry_64.S:308
#21 0x0000000000000000 in ?? ()
```

- haltpoll_init at drivers/cpuidle/cpuidle-haltpoll.c:103
- intel_idle_init at drivers/idle/intel_idle.c:2036

三个都找到；

- cpuidle_register_device 的注册是类似的方法，但是虚拟机中没有调用位置。

居然是 x86_match_cpu 的 stepping 匹配不上。

## 分析下 cpuidle_state::enter 的注册者

```c
void cpuidle_poll_state_init(struct cpuidle_driver *drv)
{
	struct cpuidle_state *state = &drv->states[0];

	snprintf(state->name, CPUIDLE_NAME_LEN, "POLL");
	snprintf(state->desc, CPUIDLE_DESC_LEN, "CPUIDLE CORE POLL IDLE");
	state->exit_latency = 0;
	state->target_residency = 0;
	state->exit_latency_ns = 0;
	state->target_residency_ns = 0;
	state->power_usage = -1;
	state->enter = poll_idle;
	state->flags = CPUIDLE_FLAG_POLLING;
}
```

### POLL

drivers/cpuidle/poll_state.c 中 poll_idle
最后只是调用

```c
static __always_inline void rep_nop(void)
{
	asm volatile("rep; nop" ::: "memory");
}
```

- [ ] 既然 idle=poll 可以调用到 cpuidle_idle_call
- [ ] cpu_idle_poll

idle_setup 中，存在三个 idle=poll, nomwait, halt

- [ ] select_idle_routine 中似乎有选择
     - 如何才可以让这个选择到 POLL

### acpi_processor_setup_cstates

才发现，也是调用到 mwait_idle_with_hints 中的.

### acpi_processor_setup_lpi_states

### haltpoll_exit

### drivers/idle/intel_idle.c

- 这里定义这么多，是做啥的？

- intel_idle_init
     - x86_match_cpu

```c
static const struct x86_cpu_id intel_idle_ids[] __initconst = {
    // ...
	X86_MATCH_INTEL_FAM6_MODEL(SANDYBRIDGE,		&idle_cpu_snb),
    // ...
}

static const struct idle_cpu idle_cpu_snb __initconst = {
	.state_table = snb_cstates,
	.disable_promotion_to_c1e = true,
};

static struct cpuidle_state snb_cstates[] __initdata = {
	{
		.name = "C1",
		.desc = "MWAIT 0x00",
		.flags = MWAIT2flg(0x00),
		.exit_latency = 2,
		.target_residency = 2,
		.enter = &intel_idle,
		.enter_s2idle = intel_idle_s2idle, },
	{
		.name = "C1E",
		.desc = "MWAIT 0x01",
		.flags = MWAIT2flg(0x01) | CPUIDLE_FLAG_ALWAYS_ENABLE,
		.exit_latency = 10,
		.target_residency = 20,
		.enter = &intel_idle,
		.enter_s2idle = intel_idle_s2idle, },
	{
		.name = "C3",
		.desc = "MWAIT 0x10",
		.flags = MWAIT2flg(0x10) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 80,
		.target_residency = 211,
		.enter = &intel_idle,
		.enter_s2idle = intel_idle_s2idle, },
	{
		.name = "C6",
		.desc = "MWAIT 0x20",
		.flags = MWAIT2flg(0x20) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 104,
		.target_residency = 345,
		.enter = &intel_idle,
		.enter_s2idle = intel_idle_s2idle, },
	{
		.name = "C7",
		.desc = "MWAIT 0x30",
		.flags = MWAIT2flg(0x30) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 109,
		.target_residency = 345,
		.enter = &intel_idle,
		.enter_s2idle = intel_idle_s2idle, },
	{
		.enter = NULL }
};
```

随手一观测，结果发现了这么多:

```txt
[sudo] password for martins3:
Attaching 1 probe...
^C

@[
    intel_idle+1
    cpuidle_enter_state+137
    cpuidle_enter+41
    do_idle+492
    cpu_startup_entry+25
    rest_init+204
    arch_call_rest_init+10
    start_kernel+1766
    secondary_startup_64_no_verify+224
]: 7954
@[
    intel_idle+1
    cpuidle_enter_state+137
    cpuidle_enter+41
    do_idle+492
    cpu_startup_entry+25
    start_secondary+271
    secondary_startup_64_no_verify+224
]: 46400
```

最核心的位置:

```c
/*
 * This uses new MONITOR/MWAIT instructions on P4 processors with PNI,
 * which can obviate IPI to trigger checking of need_resched.
 * We execute MONITOR against need_resched and enter optimized wait state
 * through MWAIT. Whenever someone changes need_resched, we would be woken
 * up from MWAIT (without an IPI).
 *
 * New with Core Duo processors, MWAIT can take some hints based on CPU
 * capability.
 */
static __always_inline void mwait_idle_with_hints(unsigned long eax, unsigned long ecx)
{
	if (static_cpu_has_bug(X86_BUG_MONITOR) || !current_set_polling_and_test()) {
		if (static_cpu_has_bug(X86_BUG_CLFLUSH_MONITOR)) {
			mb();
			clflush((void *)&current_thread_info()->flags);
			mb();
		}

		__monitor((void *)&current_thread_info()->flags, 0, 0);
		if (!need_resched())
			__mwait(eax, ecx);
	}
	current_clr_polling();
}
```

关于 monitor 和 mwait 指令 : https://stackoverflow.com/questions/57471862/uses-of-the-monitor-mwait-instructions

- [ ] 超线程中，如果使用 mwait 指令将一个 SMT core 设置为 C7，另一个 SMT 不是 C7，可以接受吗？

#### C7 都支持了，那么是不是 mwait 支持所有的状态

#### 那么 suspend 指的是那种状态

## 虚拟化场景下，Guest 是处于那种状态，为什么会自动探测到该状态

## 有区别 enter_dead 和 enter_s2idle ？

```c
struct cpuidle_state {
	char		name[CPUIDLE_NAME_LEN];
	char		desc[CPUIDLE_DESC_LEN];

	s64		exit_latency_ns;
	s64		target_residency_ns;
	unsigned int	flags;
	unsigned int	exit_latency; /* in US */
	int		power_usage; /* in mW */
	unsigned int	target_residency; /* in US */

	int (*enter)	(struct cpuidle_device *dev,
			struct cpuidle_driver *drv,
			int index);

	int (*enter_dead) (struct cpuidle_device *dev, int index);

	/*
	 * CPUs execute ->enter_s2idle with the local tick or entire timekeeping
	 * suspended, so it must not re-enable interrupts at any point (even
	 * temporarily) or attempt to change states of clock event devices.
	 *
	 * This callback may point to the same function as ->enter if all of
	 * the above requirements are met by it.
	 */
	int (*enter_s2idle)(struct cpuidle_device *dev,
			    struct cpuidle_driver *drv,
			    int index);
};
```

## 据说是存在总的功耗限制的，那是在哪里？

## cpuidle_governor

这是物理机上观测到的:

```txt
@[
    menu_select+1
    do_idle+450
    cpu_startup_entry+25
    rest_init+204
    arch_call_rest_init+10
    start_kernel+1766
    secondary_startup_64_no_verify+224
]: 150
@[
    menu_select+1
    do_idle+450
    cpu_startup_entry+25
    start_secondary+271
    secondary_startup_64_no_verify+224
]: 2582
```

这个不知道为什么，连断点都打不上:

```c
static struct cpuidle_governor haltpoll_governor = {
	.name =			"haltpoll",
	.rating =		9,
	.enable =		haltpoll_enable_device,
	.select =		haltpoll_select,
	.reflect =		haltpoll_reflect,
};

static int __init init_haltpoll(void)
{
	if (kvm_para_available())
		return cpuidle_register_governor(&haltpoll_governor);

	return 0;
}
```

### 通过 sysfs 可以观测到这些

```c
static DEVICE_ATTR(available_governors, 0444, show_available_governors, NULL);
static DEVICE_ATTR(current_driver, 0444, show_current_driver, NULL);
static DEVICE_ATTR(current_governor, 0644, show_current_governor,
				   store_current_governor);
static DEVICE_ATTR(current_governor_ro, 0444, show_current_governor, NULL);
```

### 为什么 periodic timer tick system 中使用，menu 在 tickless system 中使用

## 为什么虚拟机中最后总是走到这里的，或者说，虚拟化中一共存在那些特殊处理

```c
/*
 * We use this if we don't have any better idle routine..
 */
void __cpuidle default_idle(void)
{
	raw_safe_halt();
	raw_local_irq_disable();
}
```

相关的介绍为:

- https://www.kernel.org/doc/Documentation/cpuidle/sysfs.txt

在一个正经虚拟机中效果如下:

```txt
:/sys/devices/system/cpu/cpuidle# cat available_governors
ladder menu teo haltpoll
:/sys/devices/system/cpu/cpuidle# cat current_driver
none
:/sys/devices/system/cpu/cpuidle# cat current_governor
menu
```

- ladder 是阶梯式的，需要逐级向下
- menu 是

## governor

```c
static struct cpuidle_governor menu_governor = {
    .name =     "menu",
    .rating =   20,
    .enable =   menu_enable_device,
    .select =   menu_select,
    .reflect =  menu_reflect,
};
```

```c
static struct cpuidle_governor ladder_governor = {
    .name =     "ladder",
    .rating =   10,
    .enable =   ladder_enable_device,
    .select =   ladder_select_state,
    .reflect =  ladder_reflect,
};
```

```c

static struct cpuidle_governor haltpoll_governor = {
    .name =         "haltpoll",
    .rating =       9,
    .enable =       haltpoll_enable_device,
    .select =       haltpoll_select,
    .reflect =      haltpoll_reflect,
};
```

## [ ] 为什么 firmware 可以修改 cstate 的状态

## 虚拟机为什么是这个 idle 选择，不能进入到 acpi_idle_enter 中

```txt
0  default_idle () at arch/x86/kernel/process.c:730
#1  0xffffffff81f46f0c in default_idle_call () at kernel/sched/idle.c:109
#2  0xffffffff8114b0ed in cpuidle_idle_call () at kernel/sched/idle.c:191
#3  do_idle () at kernel/sched/idle.c:303
#4  0xffffffff8114b324 in cpu_startup_entry (state=state@entry=CPUHP_AP_ONLINE_IDLE) at kernel/sched/idle.c:400
#5  0xffffffff810e0518 in start_secondary (unused=<optimized out>) at arch/x86/kernel/smpboot.c:262
#6  0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
#7  0x0000000000000000 in ?? ()
```

- do_idle

     - cpu_idle_poll
          - cpu_relax : 使用 nop 指令
     - cpuidle_idle_call
          - cpuidle_select
          - call_cpuidle
               - cpuidle_enter
                    - cpuidle_enter_state
                         - cpuidle_state::enter

- [ ] acpi_processor_setup_cstates

## s2idle

## cpuidle_device

```c
struct cpuidle_device {
    unsigned int        registered:1;
    unsigned int        enabled:1;
    unsigned int        poll_time_limit:1;
    unsigned int        cpu;
    ktime_t         next_hrtimer;

    int         last_state_idx;
    u64         last_residency_ns;
    u64         poll_limit_ns;
    u64         forced_idle_latency_limit_ns;
    struct cpuidle_state_usage  states_usage[CPUIDLE_STATE_MAX];
    struct cpuidle_state_kobj *kobjs[CPUIDLE_STATE_MAX];
    struct cpuidle_driver_kobj *kobj_driver;
    struct cpuidle_device_kobj *kobj_dev;
    struct list_head    device_list;

#ifdef CONFIG_ARCH_NEEDS_CPU_IDLE_COUPLED
    cpumask_t       coupled_cpus;
    struct cpuidle_coupled  *coupled;
#endif
```

## cpuidle_driver

```c
struct cpuidle_driver {
    const char      *name;
    struct module       *owner;

        /* used by the cpuidle framework to setup the broadcast timer */
    unsigned int            bctimer:1;
    /* states array must be ordered in decreasing power consumption */
    struct cpuidle_state    states[CPUIDLE_STATE_MAX];
    int         state_count;
    int         safe_state_index;

    /* the driver handles the cpus in cpumask */
    struct cpumask      *cpumask;

    /* preferred governor to switch at register time */
    const char      *governor;
};
```

## cpuidle_state::enter

- default_enter_idle
- poll_idle
- acpi_idle_enter
- acpi_idle_lpi_enter

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

## 记录下内核 suspend 的时候日志

```txt
[ 4676.538450] Freezing user space processes ... (elapsed 0.001 seconds) done.
[ 4676.539462] OOM killer disabled.
[ 4676.539463] Freezing remaining freezable tasks ... (elapsed 0.000 seconds) done.
[ 4676.540447] printk: Suspending console(s) (use no_console_suspend to debug)
[ 4676.541780] serial 00:01: disabled
[ 4676.550388] sd 7:0:0:0: [sda] Synchronizing SCSI cache
[ 4676.550509] sd 7:0:0:0: [sda] Stopping disk
[ 4677.180945] ACPI: PM: Preparing to enter system sleep state S3
[ 4677.383949] ACPI: PM: Saving platform NVS memory
[ 4677.384114] Disabling non-boot CPUs ...
[ 4677.386049] smpboot: CPU 1 is now offline
[ 4677.390134] smpboot: CPU 2 is now offline
[ 4677.394149] smpboot: CPU 3 is now offline
[ 4677.395947] smpboot: CPU 4 is now offline
[ 4677.399695] smpboot: CPU 5 is now offline
[ 4677.401469] smpboot: CPU 6 is now offline
[ 4677.405137] smpboot: CPU 7 is now offline
[ 4677.406854] smpboot: CPU 8 is now offline
[ 4677.410387] smpboot: CPU 9 is now offline
[ 4677.412106] smpboot: CPU 10 is now offline
[ 4677.415545] smpboot: CPU 11 is now offline
[ 4677.417199] smpboot: CPU 12 is now offline
[ 4677.420588] smpboot: CPU 13 is now offline
[ 4677.422594] smpboot: CPU 14 is now offline
[ 4677.425617] smpboot: CPU 15 is now offline
[ 4677.427257] smpboot: CPU 16 is now offline
[ 4677.429094] smpboot: CPU 17 is now offline
[ 4677.430911] smpboot: CPU 18 is now offline
[ 4677.432704] smpboot: CPU 19 is now offline
[ 4677.434607] smpboot: CPU 20 is now offline
[ 4677.436346] smpboot: CPU 21 is now offline
[ 4677.437986] smpboot: CPU 22 is now offline
[ 4677.439768] smpboot: CPU 23 is now offline
[ 4677.441433] smpboot: CPU 24 is now offline
[ 4677.443037] smpboot: CPU 25 is now offline
[ 4677.444674] smpboot: CPU 26 is now offline
[ 4677.446255] smpboot: CPU 27 is now offline
[ 4677.447957] smpboot: CPU 28 is now offline
[ 4677.449621] smpboot: CPU 29 is now offline
[ 4677.451307] smpboot: CPU 30 is now offline
[ 4677.452866] smpboot: CPU 31 is now offline
[ 4677.458966] ACPI: PM: Low-level resume complete
[ 4677.459045] ACPI: PM: Restoring platform NVS memory
[ 4677.459941] Enabling non-boot CPUs ...
[ 4677.459973] x86: Booting SMP configuration:
[ 4677.459974] smpboot: Booting Node 0 Processor 1 APIC 0x1
[ 4677.462700] CPU1 is up
[ 4677.462717] smpboot: Booting Node 0 Processor 2 APIC 0x8
[ 4677.469558] CPU2 is up
[ 4677.469573] smpboot: Booting Node 0 Processor 3 APIC 0x9
[ 4677.472228] CPU3 is up
[ 4677.472242] smpboot: Booting Node 0 Processor 4 APIC 0x10
[ 4677.478896] CPU4 is up
[ 4677.478920] smpboot: Booting Node 0 Processor 5 APIC 0x11
[ 4677.481522] CPU5 is up
[ 4677.481539] smpboot: Booting Node 0 Processor 6 APIC 0x18
[ 4677.488191] CPU6 is up
[ 4677.488208] smpboot: Booting Node 0 Processor 7 APIC 0x19
[ 4677.490826] CPU7 is up
[ 4677.490842] smpboot: Booting Node 0 Processor 8 APIC 0x20
[ 4677.497352] CPU8 is up
[ 4677.497369] smpboot: Booting Node 0 Processor 9 APIC 0x21
[ 4677.499978] CPU9 is up
[ 4677.499992] smpboot: Booting Node 0 Processor 10 APIC 0x28
[ 4677.506515] CPU10 is up
[ 4677.506531] smpboot: Booting Node 0 Processor 11 APIC 0x29
[ 4677.509159] CPU11 is up
[ 4677.509176] smpboot: Booting Node 0 Processor 12 APIC 0x30
[ 4677.515730] CPU12 is up
[ 4677.515746] smpboot: Booting Node 0 Processor 13 APIC 0x31
[ 4677.518419] CPU13 is up
[ 4677.518435] smpboot: Booting Node 0 Processor 14 APIC 0x38
[ 4677.525045] CPU14 is up
[ 4677.525061] smpboot: Booting Node 0 Processor 15 APIC 0x39
[ 4677.527801] CPU15 is up
[ 4677.527816] smpboot: Booting Node 0 Processor 16 APIC 0x40
[ 4677.533130] core: cpu_atom PMU driver: PEBS-via-PT
[ 4677.533133] ... version:                5
[ 4677.533134] ... bit width:              48
[ 4677.533134] ... generic registers:      6
[ 4677.533135] ... value mask:             0000ffffffffffff
[ 4677.533135] ... max period:             00007fffffffffff
[ 4677.533136] ... fixed-purpose events:   3
[ 4677.533136] ... event mask:             000000070000003f
[ 4677.533996] CPU16 is up
[ 4677.534014] smpboot: Booting Node 0 Processor 17 APIC 0x42
[ 4677.537254] CPU17 is up
[ 4677.537271] smpboot: Booting Node 0 Processor 18 APIC 0x44
[ 4677.540537] CPU18 is up
[ 4677.540554] smpboot: Booting Node 0 Processor 19 APIC 0x46
[ 4677.543842] CPU19 is up
[ 4677.543861] smpboot: Booting Node 0 Processor 20 APIC 0x48
[ 4677.550142] CPU20 is up
[ 4677.550160] smpboot: Booting Node 0 Processor 21 APIC 0x4a
[ 4677.553499] CPU21 is up
[ 4677.553522] smpboot: Booting Node 0 Processor 22 APIC 0x4c
[ 4677.556881] CPU22 is up
[ 4677.556897] smpboot: Booting Node 0 Processor 23 APIC 0x4e
[ 4677.560278] CPU23 is up
[ 4677.560296] smpboot: Booting Node 0 Processor 24 APIC 0x50
[ 4677.566716] CPU24 is up
[ 4677.566733] smpboot: Booting Node 0 Processor 25 APIC 0x52
[ 4677.570206] CPU25 is up
[ 4677.570226] smpboot: Booting Node 0 Processor 26 APIC 0x54
[ 4677.573726] CPU26 is up
[ 4677.573744] smpboot: Booting Node 0 Processor 27 APIC 0x56
[ 4677.577275] CPU27 is up
[ 4677.577293] smpboot: Booting Node 0 Processor 28 APIC 0x58
[ 4677.583801] CPU28 is up
[ 4677.583819] smpboot: Booting Node 0 Processor 29 APIC 0x5a
[ 4677.587408] CPU29 is up
[ 4677.587425] smpboot: Booting Node 0 Processor 30 APIC 0x5c
[ 4677.591024] CPU30 is up
[ 4677.591041] smpboot: Booting Node 0 Processor 31 APIC 0x5e
[ 4677.594654] CPU31 is up
[ 4677.599280] ACPI: PM: Waking up from system sleep state S3
[ 4677.616406] serial 00:01: activated
[ 4677.620357] nvme nvme0: 32/0/0 default/read/poll queues
[ 4677.625557] sd 7:0:0:0: [sda] Starting disk
[ 4677.625717] nvme nvme1: 8/0/0 default/read/poll queues
[ 4677.939926] ata5: SATA link down (SStatus 4 SControl 300)
[ 4677.940355] ata6: SATA link down (SStatus 4 SControl 300)
[ 4677.940371] ata7: SATA link down (SStatus 4 SControl 300)
[ 4680.540286] ata8: SATA link up 6.0 Gbps (SStatus 133 SControl 300)
[ 4680.558782] ata8.00: configured for UDMA/133
[ 4680.572655] OOM killer enabled.
[ 4680.572656] Restarting tasks ...
[ 4680.572716] usb 1-10: USB disconnect, device number 4
[ 4680.574360] done.
[ 4680.574366] random: crng reseeded on system resumption
```

## [ ] 为什么 amd 机器上也存在 intel_idle

```txt
cd /sys/module/intel_idle/parameters🔒
🧀  l
Permissions Size User Date Modified Name
.r--r--r--  4.1k root 25 6月  10:38  force_irq_on
.r--r--r--  4.1k root 25 6月  10:38  max_cstate
.r--r--r--  4.1k root 25 6月  10:38  no_acpi
.r--r--r--  4.1k root 25 6月  10:38  preferred_cstates
.r--r--r--  4.1k root 25 6月  10:38  states_off
.r--r--r--  4.1k root 25 6月  10:38  use_acpi
```

## material

amd 中:
processor.max_cstate=1 intel_idle.max_cstate=0

```txt
[nix-shell:~]$ cpupower idle-info
CPUidle driver: acpi_idle
CPUidle governor: menu
analyzing CPU 30:

Number of idle states: 2
Available idle states: POLL C1
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 1355
Duration: 58461
C1:
Flags/Description: ACPI FFH MWAIT 0x0
Latency: 1
Usage: 285497
Duration: 7742516699
```

processor.max_cstate=0 intel_idle.max_cstate=0

```txt
CPUidle driver: acpi_idle
CPUidle governor: menu
analyzing CPU 3:

Number of idle states: 2
Available idle states: POLL C1
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 259
Duration: 14922
C1:
Flags/Description: ACPI FFH MWAIT 0x0
Latency: 1
Usage: 24263
Duration: 118205206
```

amd 去掉参数:

```txt
[nix-shell:~]$  cpupower idle-info
CPUidle driver: acpi_idle
CPUidle governor: menu
analyzing CPU 12:

Number of idle states: 4
Available idle states: POLL C1 C2 C3
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 370
Duration: 12570
C1:
Flags/Description: ACPI FFH MWAIT 0x0
Latency: 1
Usage: 8766
Duration: 532364
C2:
Flags/Description: ACPI IOPORT 0x414
Latency: 18
Usage: 7982
Duration: 2914635
C3:
Flags/Description: ACPI IOPORT 0x415
Latency: 350
Usage: 11615
Duration: 53058960
```

intel

```txt
🧀  cpupower idle-info

CPUidle driver: intel_idle
CPUidle governor: menu
analyzing CPU 12:

Number of idle states: 4
Available idle states: POLL C1_ACPI C2_ACPI C3_ACPI
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 45923
Duration: 1045586
C1_ACPI:
Flags/Description: ACPI FFH MWAIT 0x0
Latency: 1
Usage: 1991675
Duration: 318432463
C2_ACPI:
Flags/Description: ACPI FFH MWAIT 0x21
Latency: 127
Usage: 1977297
Duration: 1396686456
C3_ACPI:
Flags/Description: ACPI FFH MWAIT 0x60
Latency: 1048
Usage: 2351264
Duration: 39850857801
```

amd 中的这个真有趣啊，
frequency should be within 1.50 GHz and 2.50 GHz.

```txt
[nix-shell:~]$ cpupower frequency-info
analyzing CPU 24:
  driver: acpi-cpufreq
  CPUs which run at the same hardware frequency: 24
  CPUs which need to have their frequency coordinated by software: 24
  maximum transition latency:  Cannot determine or is not supported.
  hardware limits: 1.50 GHz - 5.46 GHz
  available frequency steps:  2.50 GHz, 1.50 GHz
  available cpufreq governors: performance schedutil
  current policy: frequency should be within 1.50 GHz and 2.50 GHz.
                  The governor "performance" may decide which speed to use
                  within this range.
  current CPU frequency: Unable to call hardware
  current CPU frequency: 3.49 GHz (asserted by call to kernel)
  boost state support:
    Supported: yes
    Active: no
```

## pcm 中显示有 c7 ?

```txt
 Instructions retired:  332 M ; Active cycles:  405 M ; Time (TSC): 2996 Mticks ; C0 (active,non-halted) core residency: 0.24 %

 C1 core residency: 7.11 %; C3 core residency: 0.00 %; C6 core residency: 50.74 %; C7 core residency: 41.90 %;
 C0 package residency: 100.00 %; C2 package residency: 0.00 %; C4 package residency: 0.00 %; C6 package residency: 0.00 %;
                             ┌─────────────────────────────────────────────────────────────────────────────────┐
 Core    C-state distribution│111111666666666666666666666666666666666666666667777777777777777777777777777777777│
                             └─────────────────────────────────────────────────────────────────────────────────┘
                             ┌────────────────────────────────────────────────────────────────────────────────┐
 Package C-state distribution│00000000000000000000000000000000000000000000000000000000000000000000000000000000│
                             └────────────────────────────────────────────────────────────────────────────────┘

 PHYSICAL CORE IPC                 : 1.64 => corresponds to 27.38 % utilization for cores in active state
 Instructions per nominal CPU cycle: 0.01 => corresponds to 0.12 % core utilization over time interval
 SMI count: 0
 ---------------------------------------------------------------------------------------------------------------
 MEM (GB)->|  READ |  WRITE | CPU energy |
 ---------------------------------------------------------------------------------------------------------------
  SKT   0     0.57     0.26      17.73
 ---------------------------------------------------------------------------------------------------------------
```

## https://metebalci.com/blog/a-minimum-complete-tutorial-of-cpu-power-management-c-states-and-p-states/

## processor.max_cstate=1 意味着什么

## 这个文件夹下的内容都需要仔细读读

- Documentation/admin-guide/pm/intel_idle.rst

- do_idle
     - cpuidle_idle_call
          - call_cpuidle
               - cpuidle_enter
                    - cpuidle_enter_state
                         - acpi_idle_enter
          - acpi_idle_do_entry - acpi_safe_halt

- acpi_idle_enter_s2idle
- acpi_idle_enter : 标准注册的
- acpi_idle_enter_bm : enters C3 with proper BM handling
     - acpi_idle_do_entry

进行 io idle 的。

```c
/**
 * acpi_idle_do_entry - enter idle state using the appropriate method
 * @cx: cstate data
 *
 * Caller disables interrupt before call and enables interrupt after return.
 */
static void __cpuidle acpi_idle_do_entry(struct acpi_processor_cx *cx)
{
	perf_lopwr_cb(true);

	if (cx->entry_method == ACPI_CSTATE_FFH) {
		/* Call into architectural FFH based C-state */
		acpi_processor_ffh_cstate_enter(cx);
	} else if (cx->entry_method == ACPI_CSTATE_HALT) {
		acpi_safe_halt();
	} else {
		io_idle(cx->address);
	}

	perf_lopwr_cb(false);
}
```
