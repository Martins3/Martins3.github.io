# Linux scheduelr


- https://mp.weixin.qq.com/s/0LM25OrpFCCcokSCMv--Mg
  - 大致分析 idle driver 的作用已经整个 idle 代码的流程

- https://vstinner.github.io/intel-cpus.html : cstate pstate 的介绍

- 关键的内核文档: https://docs.kernel.org/admin-guide/pm/working-state.html

- cpuidle.off = 1

## https://stackoverflow.com/questions/57471862/uses-of-the-monitor-mwait-instructions
- 不知道为什么，mwait 会导致 guest 退出

- cpuidle.off=1 : 用于关闭 idle 的 subsystem 的
- hlt which just goes into the lightest sleep
- but user-space versions umonitor / umwait were added recently to Tremont (next-gen Goldmont atom)
- The idle kernel parameter is used, which takes one of the following values: poll, halt, nomwait.

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
