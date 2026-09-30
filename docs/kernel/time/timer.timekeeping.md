## tk_read_base

```c
/**
 * struct tk_read_base - base structure for timekeeping readout
 * @clock:	Current clocksource used for timekeeping.
 * @mask:	Bitmask for two's complement subtraction of non 64bit clocks
 * @cycle_last: @clock cycle value at last update
 * @mult:	(NTP adjusted) multiplier for scaled math conversion
 * @shift:	Shift value for scaled math conversion
 * @xtime_nsec: Shifted (fractional) nano seconds offset for readout
 * @base:	ktime_t (nanoseconds) base time for readout
 * @base_real:	Nanoseconds base value for clock REALTIME readout
 *
 * This struct has size 56 byte on 64 bit. Together with a seqcount it
 * occupies a single 64byte cache line.
 *
 * The struct is separate from struct timekeeper as it is also used
 * for a fast NMI safe accessors.
 *
 * @base_real is for the fast NMI safe accessor to allow reading clock
 * realtime from any context.
 */
struct tk_read_base {
	struct clocksource	*clock;
	u64			mask;
	u64			cycle_last;
	u32			mult;
	u32			shift;
	u64			xtime_nsec;
	ktime_t			base;
	u64			base_real;
};
```

似乎是为了 ntp 而准备的！

因为过一段时间，tsc 时间就会和 ntp 时间出现一些差别，
所以当 ntp 进行时间刷新的时候，需要记录下当时的 cycle_last ，之后在此基础上进行计算

- do_settimeofday64
  - timekeeping_forward_now

### shift

nsec 的精度是不够的，为了方便描述，默认保存 shift 的数值，如果需要，将其装换为

- posix_get_realtime_coarse
  - ktime_get_coarse_real_ts64
    - tk_xtime

```c
	ts.tv_sec = tk->xtime_sec;
	ts.tv_nsec = (long)(tk->tkr_mono.xtime_nsec >> tk->tkr_mono.shift);
```

tv_nsec 中记录的时间比

### base

```c
void ktime_get_real_ts64(struct timespec64 *ts)
		ts->tv_sec = tk->xtime_sec; // timekeeping_init 中初始化为 wall time
		nsecs = timekeeping_get_ns(&tk->tkr_mono);

ktime_t ktime_get(void)
		base = tk->tkr_mono.base; // 默认应该是 0 ，但是校准过时钟之后，会从一个新的地方开始计算
		nsecs = timekeeping_get_ns(&tk->tkr_mono);
```
## 如何进行 cycle 和 hz 的装换的
是通过进行校准的:
```c
	clocksource_register_hz(&clocksource_counter, arch_timer_rate);
```

时钟频率是从 dts 中得到的，非常的合理:
```c
/*
 * For historical reasons, when probing with DT we use whichever (non-zero)
 * rate was probed first, and don't verify that others match. If the first node
 * probed has a clock-frequency property, this overrides the HW register.
 */
static void __init arch_timer_of_configure_rate(u32 rate, struct device_node *np)
{
	/* Who has more than one independent system counter? */
	if (arch_timer_rate)
		return;

	if (of_property_read_u32(np, "clock-frequency", &arch_timer_rate))
		arch_timer_rate = rate;

	/* Check the timer frequency. */
	if (validate_timer_rate())
		pr_warn("frequency not available\n");
}
```

## 假如 guest 开启了 ntp ，但是 host 没有开启 ntp ，会导致 guest 被 host 带偏吗?
不会，只要 tsc 是稳定的，ntp 会校准过来的

## 加入 host 开启 ntp ，当时 host 进行时间调整的时候，guest 有感知吗?
直到同步的时候，才会将感知传递过去。

## 调整 kvm-clock 的时间 + 暂停 QEMU + 加上手动同步时间基本可以完美解决

```diff
diff --git a/arch/x86/kernel/kvmclock.c b/arch/x86/kernel/kvmclock.c
index 5b2c15214a6b..7cfcbd1d1d5d 100644
--- a/arch/x86/kernel/kvmclock.c
+++ b/arch/x86/kernel/kvmclock.c
@@ -344,6 +344,6 @@ void __init kvmclock_init(void)
 	    !check_tsc_unstable())
 		kvm_clock.rating = 299;

-	clocksource_register_hz(&kvm_clock, NSEC_PER_SEC);
+	clocksource_register_hz(&kvm_clock, NSEC_PER_SEC / 2);
 	pv_info.name = "KVM";
 }
```

然后使用 c/clock_time.c 观察
```txt
CLOCK_REALTIME : 1717334434.270 (19876 days + 13h 20m 34s)
CLOCK_TAI      : 1717334434.270 (19876 days + 13h 20m 34s)
CLOCK_MONOTONIC:       9719.500 ( 2h 41m 59s)
CLOCK_BOOTTIME :       9719.500 ( 2h 41m 59s)
CLOCK_MONOTONIC_RAW:      10799.037 ( 2h 59m 59s)
```

观察 QEMU 的运行的时间:
```txt
 ps  -p 1197850  -o etime
   ELAPSED
   01:29:52
```

## tk_update_ktime_data : 中，原来时钟中断的时候都是在刷新 raw_sec 作为基础的！
```txt
	tk->tkr_raw.base = ns_to_ktime(tk->raw_sec * NSEC_PER_SEC);
```
```txt
  timekeeping_advance
  update_wall_time
  tick_nohz_handler
  __hrtimer_run_queues
  hrtimer_interrupt
  __sysvec_apic_timer_interrupt
  sysvec_apic_timer_interrupt
  asm_sysvec_apic_timer_interrupt
  default_idle
  default_idle_call
  do_idle
  cpu_startup_entry
  start_secondary
  common_startup_64
    60

  timekeeping_update
  timekeeping_advance
  update_wall_time
  tick_irq_enter
  irq_enter_rcu
  sysvec_apic_timer_interrupt
  asm_sysvec_apic_timer_interrupt
  default_idle
  default_idle_call
  do_idle
  cpu_startup_entry
  start_secondary
  common_startup_64
    125
```

为什么要这样设计?


## 这个函数的作用是什么?
ktime_get_boot_fast_ns

在这个函数中，最后的结构是这个样子的:
```c
	do {
		seq = read_seqcount_latch(&tkf->seq);
		tkr = tkf->base + (seq & 0x01);
		now = ktime_to_ns(tkr->base);
		now += timekeeping_get_ns(tkr);
	} while (read_seqcount_latch_retry(&tkf->seq, seq));
```

1. 为什么需要使用 seqlock ? 而且 read_seqcount_latch 的变种是为了什么?
2. 类似的结构都收集一下，理解一些这个问题?

- 将 `update_vdso_data` 刷新到 vdso 中。

## 为什么需要把 timekeeping_update_from_shadow 中再次刷到
其他的结构体中:

```c
	update_vsyscall(tk);
	update_pvclock_gtod(tk, action & TK_CLOCK_WAS_SET);
```

例如 update_pvclock_gtod 一路进入到的地方是:

完全的拷贝，有意义吗?

```c
static void update_pvclock_gtod(struct timekeeper *tk)
{
	struct pvclock_gtod_data *vdata = &pvclock_gtod_data;

	write_seqcount_begin(&vdata->seq);

	/* copy pvclock gtod data */
	vdata->clock.vclock_mode	= tk->tkr_mono.clock->vdso_clock_mode;
	vdata->clock.cycle_last		= tk->tkr_mono.cycle_last;
	vdata->clock.mask		= tk->tkr_mono.mask;
	vdata->clock.mult		= tk->tkr_mono.mult;
	vdata->clock.shift		= tk->tkr_mono.shift;
	vdata->clock.base_cycles	= tk->tkr_mono.xtime_nsec;
	vdata->clock.offset		= tk->tkr_mono.base;

	vdata->raw_clock.vclock_mode	= tk->tkr_raw.clock->vdso_clock_mode;
	vdata->raw_clock.cycle_last	= tk->tkr_raw.cycle_last;
	vdata->raw_clock.mask		= tk->tkr_raw.mask;
	vdata->raw_clock.mult		= tk->tkr_raw.mult;
	vdata->raw_clock.shift		= tk->tkr_raw.shift;
	vdata->raw_clock.base_cycles	= tk->tkr_raw.xtime_nsec;
	vdata->raw_clock.offset		= tk->tkr_raw.base;

	vdata->wall_time_sec            = tk->xtime_sec;

	vdata->offs_boot		= tk->offs_boot;

	write_seqcount_end(&vdata->seq);
}
```
## 统计一下都是那些 clock source 的

```c
static struct clocksource kvm_clock = {
	.name	= "kvm-clock",
	.read	= kvm_clock_get_cycles,
	.rating	= 400,
	.mask	= CLOCKSOURCE_MASK(64),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
	.id     = CSID_X86_KVM_CLK,
	.enable	= kvm_cs_enable,
};
```

有的问题必想想的还要牛逼啊
drivers/clocksource/hyperv_timer.c

drivers/clocksource/acpi_pm.c

drivers/clocksource/i8253.c


## 3.10 在时间向前大幅跳跃的时候，为什么会卡主?

```txt
PID: 0        TASK: ffff903a3d743180  CPU: 7    COMMAND: "swapper/7"
    [exception RIP: __getnstimeofday64+153]
    RIP: ffffffff99f07c59  RSP: ffff903af6fc3d58  RFLAGS: 00000286
    RAX: 8dc23093719f8651  RBX: ffff903af6fc3d90  RCX: 000000006822d3ef
    RDX: 00000000eaa9c987  RSI: 00000352b5c94bcf  RDI: 00066dca92cde4ed
    RBP: ffff903af6fc3d70   R8: 0000000000800008   R9: 0000000000000000
    R10: 0000000000000000  R11: 00000000c476ec70  R12: ffffffff9aa34a80
    R13: 00000000000e71fa  R14: ffff903af36a88c0  R15: ffff903af45ca900
    CS: 0010  SS: 0018
 #0 [ffff903af6fc3d78] getnstimeofday64 at ffffffff99f07c9e
 #1 [ffff903af6fc3d88] ktime_get_real at ffffffff99f07d55
 #2 [ffff903af6fc3db0] netif_receive_skb_internal at ffffffff9a454be6
 #3 [ffff903af6fc3de0] napi_gro_receive at ffffffff9a455838
 #4 [ffff903af6fc3e08] virtnet_poll at ffffffffc032b4a5 [virtio_net]
 #5 [ffff903af6fc3e78] net_rx_action at ffffffff9a4551cf
 #6 [ffff903af6fc3ef8] __do_softirq at ffffffff99ea4c15
 #7 [ffff903af6fc3f68] call_softirq at ffffffff9a5974ec
 #8 [ffff903af6fc3f80] do_softirq at ffffffff99e2f715
 #9 [ffff903af6fc3fa0] irq_exit at ffffffff99ea4f95
#10 [ffff903af6fc3fb8] do_IRQ at ffffffff9a598936
```

主线:
```c
/**
 * ktime_get_real - get the real (wall-) time in ktime_t format
 *
 * returns the time in ktime_t format
 */
ktime_t ktime_get_real(void)
{
	struct timespec64 now;

	getnstimeofday64(&now); // 这里 	timespec64_add_ns(ts, nsecs); 的累积不是除法，而是一遍遍的做减法

	return timespec64_to_ktime(now);
}

```

主线中: ktime_get_real -> ktime_get_with_offset
```c
ktime_t ktime_get_with_offset(enum tk_offsets offs)
{
	struct timekeeper *tk = &tk_core.timekeeper;
	unsigned int seq;
	ktime_t base, *offset = offsets[offs];
	u64 nsecs;

	WARN_ON(timekeeping_suspended);

	do {
		seq = read_seqcount_begin(&tk_core.seq);
		base = ktime_add(tk->tkr_mono.base, *offset);
		nsecs = timekeeping_get_ns(&tk->tkr_mono);

	} while (read_seqcount_retry(&tk_core.seq, seq));

	return ktime_add_ns(base, nsecs);

}
```
这里就没有这个转换，所以也没有关系。

## 执行到 update_vsyscall 说明了什么来着

应该就是 timer interrupt 的位置:
```txt
+ sudo bpftrace -e 'kprobe:update_vsyscall { @[curtask->comm] = count() } interval:s:1000 { exit(); }'
Attached 2 probes

@[nvim]: 1
@[renderer]: 1
@[clash-verge]: 1
@[swapper/21]: 1
@[swapper/15]: 1
@[swapper/29]: 1
@[dockerd]: 1
@[swapper/25]: 1
@[async-runtime]: 1
@[CPU 7/KVM]: 1
@[swapper/31]: 1
@[IO mon_iothread]: 1
@[msedge]: 1
@[swapper/11]: 1
@[runc]: 1
@[CPU 8/KVM]: 1
@[CPU 4/KVM]: 1
@[swapper/13]: 2
@[CPU 5/KVM]: 2
@[CPU 12/KVM]: 2
@[swapper/5]: 2
@[systemd-oomd]: 2
@[CPU 2/KVM]: 3
@[swapper/23]: 3
@[Chrome_ChildIOT]: 3
@[VizCompositorTh]: 3
@[swapper/7]: 4
@[gnome-shell]: 4
@[swapper/26]: 4
@[CPU 1/KVM]: 4
@[nginx]: 4
@[CPU 0/KVM]: 5
@[pg_isready]: 5
@[WebKitWebProces]: 6
@[swapper/16]: 7
@[runc:[2:INIT]]: 8
@[swapper/19]: 9
@[swapper/20]: 15
@[swapper/9]: 17
@[swapper/3]: 17
@[swapper/18]: 21
@[swapper/1]: 25
@[qemu-system-x86]: 25
@[swapper/17]: 68
@[swapper/6]: 117
@[python3]: 118
@[swapper/14]: 169
@[swapper/8]: 187
@[swapper/4]: 189
@[swapper/2]: 262
@[swapper/12]: 374
@[swapper/10]: 401
@[swapper/0]: 464
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
