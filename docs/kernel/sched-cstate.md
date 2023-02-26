## 主要的问题
1. 是如何选择的 driver 的，为什么最后选择的 intel_idle
2. governor 最终选择规则是什么
3. 进入到那个 state 是如何确定的

## sleep 的等级
- https://docs.kernel.org/admin-guide/pm/sleep-states.html


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


##  s2idle
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

- [ ]  select_idle_routine 中似乎有选择
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
