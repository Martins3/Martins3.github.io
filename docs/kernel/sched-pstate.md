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
