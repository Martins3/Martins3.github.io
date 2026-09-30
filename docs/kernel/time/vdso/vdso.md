# vdso

## 大致实现

提供一个代码给其他的程序执行，在其中需要执行 rdtsc 。 虚拟机使用 tsc 和
kvm-clock 的区别就是，kvm-clock 会使用 pvti 会进行更加复杂的计算的。

当 tsc 或者 kvm-clock 不可用，让 rdtsc 不可使用，那么 syscall 失败，进而
去退化为 syscall 。

## vdso 存在的东西已经很多了，可以整理下

https://mp.weixin.qq.com/s/mlsdi8X-bhOS44zyuEC6Gw

这个是做什么的?

scripts/Makefile.vdsoinst

## 其实 man vdso 就已经提供很多东西了

## 看看 vsyscall 和 vdso 处理 timer 的区别

## 使用 fio 来对比测试 vdso 的性能影响

### 关闭 vdso 的 map

vdso=1

```txt
➜  ~ cat /proc/self/maps
55d0a2539000-55d0a253b000 r--p 00000000 08:03 3164743                    /usr/bin/cat
55d0a253b000-55d0a2540000 r-xp 00002000 08:03 3164743                    /usr/bin/cat
55d0a2540000-55d0a2543000 r--p 00007000 08:03 3164743                    /usr/bin/cat
55d0a2543000-55d0a2544000 r--p 00009000 08:03 3164743                    /usr/bin/cat
55d0a2544000-55d0a2545000 rw-p 0000a000 08:03 3164743                    /usr/bin/cat
55d0a4ed1000-55d0a4ef2000 rw-p 00000000 00:00 0                          [heap]
7f2566fde000-7f2567000000 rw-p 00000000 00:00 0
7f2567000000-7f2568239000 r--p 00000000 08:03 3148100                    /usr/lib/locale/locale-archive
7f2568248000-7f256824b000 rw-p 00000000 00:00 0
7f256824b000-7f2568274000 r--p 00000000 08:03 3149573                    /usr/lib64/libc.so.6
7f2568274000-7f25683c5000 r-xp 00029000 08:03 3149573                    /usr/lib64/libc.so.6
7f25683c5000-7f2568415000 r--p 0017a000 08:03 3149573                    /usr/lib64/libc.so.6
7f2568415000-7f2568419000 r--p 001c9000 08:03 3149573                    /usr/lib64/libc.so.6
7f2568419000-7f256841b000 rw-p 001cd000 08:03 3149573                    /usr/lib64/libc.so.6
7f256841b000-7f2568428000 rw-p 00000000 00:00 0
7f2568438000-7f256843a000 rw-p 00000000 00:00 0
7f256843a000-7f256843e000 r--p 00000000 00:00 0                          [vvar]
7f256843e000-7f2568440000 r-xp 00000000 00:00 0                          [vdso]
7f2568440000-7f2568441000 r--p 00000000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7f2568441000-7f2568466000 r-xp 00001000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7f2568466000-7f256846f000 r--p 00026000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7f256846f000-7f2568471000 r--p 0002e000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7f2568471000-7f2568473000 rw-p 00030000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7ffefa04f000-7ffefa070000 rw-p 00000000 00:00 0                          [stack]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
```

vdso=0

```txt
55f454429000-55f45442b000 r--p 00000000 08:03 3164743                    /usr/bin/cat
55f45442b000-55f454430000 r-xp 00002000 08:03 3164743                    /usr/bin/cat
55f454430000-55f454433000 r--p 00007000 08:03 3164743                    /usr/bin/cat
55f454433000-55f454434000 r--p 00009000 08:03 3164743                    /usr/bin/cat
55f454434000-55f454435000 rw-p 0000a000 08:03 3164743                    /usr/bin/cat
55f487df7000-55f487e18000 rw-p 00000000 00:00 0                          [heap]
7f0304800000-7f0305a39000 r--p 00000000 08:03 3148100                    /usr/lib/locale/locale-archive
7f0305a6f000-7f0305a93000 rw-p 00000000 00:00 0
7f0305a93000-7f0305abc000 r--p 00000000 08:03 3149573                    /usr/lib64/libc.so.6
7f0305abc000-7f0305c0d000 r-xp 00029000 08:03 3149573                    /usr/lib64/libc.so.6
7f0305c0d000-7f0305c5d000 r--p 0017a000 08:03 3149573                    /usr/lib64/libc.so.6
7f0305c5d000-7f0305c61000 r--p 001c9000 08:03 3149573                    /usr/lib64/libc.so.6
7f0305c61000-7f0305c63000 rw-p 001cd000 08:03 3149573                    /usr/lib64/libc.so.6
7f0305c63000-7f0305c70000 rw-p 00000000 00:00 0
7f0305c80000-7f0305c81000 r--p 00000000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7f0305c81000-7f0305ca6000 r-xp 00001000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7f0305ca6000-7f0305caf000 r--p 00026000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7f0305caf000-7f0305cb1000 r--p 0002e000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7f0305cb1000-7f0305cb3000 rw-p 00030000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7ffd2348f000-7ffd234b0000 rw-p 00000000 00:00 0                          [stack]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
```

vdso=0 vsyscall=none

```txt
563da0516000-563da0518000 r--p 00000000 08:03 3164743                    /usr/bin/cat
563da0518000-563da051d000 r-xp 00002000 08:03 3164743                    /usr/bin/cat
563da051d000-563da0520000 r--p 00007000 08:03 3164743                    /usr/bin/cat
563da0520000-563da0521000 r--p 00009000 08:03 3164743                    /usr/bin/cat
563da0521000-563da0522000 rw-p 0000a000 08:03 3164743                    /usr/bin/cat
563da73ed000-563da740e000 rw-p 00000000 00:00 0                          [heap]
7fd541400000-7fd542639000 r--p 00000000 08:03 3148100                    /usr/lib/locale/locale-archive
7fd5427a8000-7fd5427cc000 rw-p 00000000 00:00 0
7fd5427cc000-7fd5427f5000 r--p 00000000 08:03 3149573                    /usr/lib64/libc.so.6
7fd5427f5000-7fd542946000 r-xp 00029000 08:03 3149573                    /usr/lib64/libc.so.6
7fd542946000-7fd542996000 r--p 0017a000 08:03 3149573                    /usr/lib64/libc.so.6
7fd542996000-7fd54299a000 r--p 001c9000 08:03 3149573                    /usr/lib64/libc.so.6
7fd54299a000-7fd54299c000 rw-p 001cd000 08:03 3149573                    /usr/lib64/libc.so.6
7fd54299c000-7fd5429a9000 rw-p 00000000 00:00 0
7fd5429b9000-7fd5429ba000 r--p 00000000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7fd5429ba000-7fd5429df000 r-xp 00001000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7fd5429df000-7fd5429e8000 r--p 00026000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7fd5429e8000-7fd5429ea000 r--p 0002e000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7fd5429ea000-7fd5429ec000 rw-p 00030000 08:03 3149570                    /usr/lib64/ld-linux-x86-64.so.2
7ffc641a9000-7ffc641ca000 rw-p 00000000 00:00 0                          [stack]
```

## vdso 中的考虑

vdso 中 `__cvdso_clock_gettime_common` 作为核心，很容易找到。

之所以，vdso 可以支持各种 clcok_gettime 的 clockid ，是因为提前纯除了 vda

这个 vda 的刷新是每一个 tick 都会发生的。

```txt
- asm_common_interrupt
  - common_interrupt
    - __common_interrupt
      - call_irq_handler
        - handle_irq
          - generic_handle_irq_desc
            - handle_level_irq
              - handle_irq_event
                - handle_irq_event_percpu
                  - __handle_irq_event_percpu
                    - timer_interrupt
                      - tick_handle_periodic
                        - tick_periodic
                          - update_wall_time
                            - timekeeping_advance
                              - timekeeping_update
                                - update_vsyscall
```

## 从 vdso 中获取时间无法保证时间不会 jump back 吗?

## 基本的执行流程

核心函数:

- __cvdso_clock_gettime_common
  - do_coarse
  - do_hres
    - __arch_get_hw_counter
    - vdso_calc_ns : 这里会检查时间回退

### 在 v4.19 的时候是没有的检查时间回退的

```c
notrace static int __always_inline do_monotonic(struct timespec *ts)
{
	unsigned long seq;
	u64 ns;
	int mode;

	do {
		seq = gtod_read_begin(gtod);
		mode = gtod->vclock_mode;
		ts->tv_sec = gtod->monotonic_time_sec;
		ns = gtod->monotonic_time_snsec;
		ns += vgetsns(&mode);
		ns >>= gtod->shift;
	} while (unlikely(gtod_read_retry(gtod, seq)));

	ts->tv_sec += __iter_div_u64_rem(ns, NSEC_PER_SEC, &ns);
	ts->tv_nsec = ns;

	return mode;
}

notrace static inline u64 vgetsns(int *mode)
{
	u64 v;
	cycles_t cycles;

	if (gtod->vclock_mode == VCLOCK_TSC)
		cycles = vread_tsc();

	/*
	 * For any memory-mapped vclock type, we need to make sure that gcc
	 * doesn't cleverly hoist a load before the mode check.  Otherwise we
	 * might end up touching the memory-mapped page even if the vclock in
	 * question isn't enabled, which will segfault.  Hence the barriers.
	 */
#ifdef CONFIG_PARAVIRT_CLOCK
	else if (gtod->vclock_mode == VCLOCK_PVCLOCK) {
		barrier();
		cycles = vread_pvclock(mode);
	}
#endif
#ifdef CONFIG_HYPERV_TSCPAGE
	else if (gtod->vclock_mode == VCLOCK_HVCLOCK) {
		barrier();
		cycles = vread_hvclock(mode);
	}
#endif
	else
		return 0;
	v = (cycles - gtod->cycle_last) & gtod->mask;
	return v * gtod->mult;
}
```

并不是，4.19 内核是回退时间的。这个需要测试一下。

## 当 unstable 的时候，那么 clocksource vdso 还是走的 tsc 吗?

显然不是的，但是 vdso 怎么知道需要:

```txt
🧀  sudo syscount
Tracing syscalls, printing top 10... Ctrl+C to quit.
^C[11:38:22]
SYSCALL                   COUNT
clock_gettime            234102
gettimeofday              10055
ppoll                      5408
futex                      4932
ioctl                      4821
read                       2875
recvmsg                    2513
write                      1451
epoll_wait                  952
io_submit                   772
```

答案在: lib/vdso/gettimeofday.c

```c
static __maybe_unused int
__cvdso_clock_gettime_data(const struct vdso_data *vd, clockid_t clock,
			   struct __kernel_timespec *ts)
{
	int ret = __cvdso_clock_gettime_common(vd, clock, ts);

	if (unlikely(ret))
		return clock_gettime_fallback(clock, ts);
	return 0;
}
```

此外，从 __arch_get_hw_counter 中也是看到这些变化的:

```c
if (likely(clock_mode == VDSO_CLOCKMODE_TSC))
	return (u64)rdtsc_ordered() & S64_MAX;
```

第三种方法: 使用 gdb ，比想象的要简单，layout asm + si ，可以清晰的看到走到
syscall 的。

## 看看 musl 如何实现 vdso 的

我靠，完全看不懂啊: src/internal/vdso.c

## ARM 的 vdso 也是有所不同的吧

arm 中使用什么指令读取时间?

## 似乎没有办法防护时间回退

虽然在 clocksource_delta 中有防护时间回退，但是如果通过 syscall
获取时间，防护似乎 会失败

- posix_get_monotonic_timespec
  - ktime_get_ts64
    - kvm_clock_get_cycles : 这里获取时间

```c
static inline u64 timekeeping_cycles_to_ns(const struct tk_read_base *tkr, u64 cycles)
{
	/* Calculate the delta since the last update_wall_time() */
	u64 mask = tkr->mask, delta = (cycles - tkr->cycle_last) & mask;

	/*
	 * This detects both negative motion and the case where the delta
	 * overflows the multiplication with tkr->mult.
	 */
	if (unlikely(delta > tkr->clock->max_cycles)) {
		/*
		 * Handle clocksource inconsistency between CPUs to prevent
		 * time from going backwards by checking for the MSB of the
		 * mask being set in the delta.
		 */
		if (delta & ~(mask >> 1))
			return tkr->xtime_nsec >> tkr->shift; // 如果发现这里是负数，那么直接使用累积的 ns 数量

		return delta_to_ns_safe(tkr, delta);
	}

	return ((delta * tkr->mult) + tkr->xtime_nsec) >> tkr->shift;
}
```

类似代码在 arch/x86/include/asm/vdso/gettimeofday.h

```c
/*
 * x86 specific calculation of nanoseconds for the current cycle count
 *
 * The regular implementation assumes that clocksource reads are globally
 * monotonic. The TSC can be slightly off across sockets which can cause
 * the regular delta calculation (@cycles - @last) to return a huge time
 * jump.
 *
 * Therefore it needs to be verified that @cycles are greater than
 * @vd->cycles_last. If not then use @vd->cycles_last, which is the base
 * time of the current conversion period.
 *
 * This variant also uses a custom mask because while the clocksource mask of
 * all the VDSO capable clocksources on x86 is U64_MAX, the above code uses
 * U64_MASK as an exception value, additionally arch_vdso_cycles_ok() above
 * declares everything with the MSB/Sign-bit set as invalid. Therefore the
 * effective mask is S64_MAX.
 */
static __always_inline u64 vdso_calc_ns(const struct vdso_data *vd, u64 cycles, u64 base)
{
	u64 delta = cycles - vd->cycle_last;

	/*
	 * Negative motion and deltas which can cause multiplication
	 * overflow require special treatment. This check covers both as
	 * negative motion is guaranteed to be greater than @vd::max_cycles
	 * due to unsigned comparison.
	 *
	 * Due to the MSB/Sign-bit being used as invalid marker (see
	 * arch_vdso_cycles_ok() above), the effective mask is S64_MAX, but that
	 * case is also unlikely and will also take the unlikely path here.
	 */
	if (unlikely(delta > vd->max_cycles)) {
		/*
		 * Due to the above mentioned TSC wobbles, filter out
		 * negative motion.  Per the above masking, the effective
		 * sign bit is now bit 62.
		 */
		if (delta & (1ULL << 62))
			return base >> vd->shift; // 也是如此，如果发现是负数，直接使用 ns 就可以了

		/* Handle multiplication overflow gracefully */
		return mul_u64_u32_add_u64_shr(delta & S64_MAX, vd->mult, base, vd->shift);
	}

	return ((delta * vd->mult) + base) >> vd->shift;
}
```

所以，这里也是防护不成功的。也非常难

## vdso 即便是静态编译，也会 vdso

/usr/bin/gcc -static lab6-jump.c 构建的，最后还是有 vdso

```txt
🤒  cat /proc/4147/maps
00400000-00401000 r--p 00000000 00:28 65126645                           /home/martins3/data/vn/docs/kernel/time/a.out
00401000-00474000 r-xp 00001000 00:28 65126645                           /home/martins3/data/vn/docs/kernel/time/a.out
00474000-00499000 r--p 00074000 00:28 65126645                           /home/martins3/data/vn/docs/kernel/time/a.out
00499000-0049e000 r--p 00099000 00:28 65126645                           /home/martins3/data/vn/docs/kernel/time/a.out
0049e000-004a0000 rw-p 0009e000 00:28 65126645                           /home/martins3/data/vn/docs/kernel/time/a.out
004a0000-004a6000 rw-p 00000000 00:00 0
004a6000-004c8000 rw-p 00000000 00:00 0                                  [heap]
7f4100f15000-7f4100f17000 r--p 00000000 00:00 0                          [vvar]
7f4100f17000-7f4100f19000 r--p 00000000 00:00 0                          [vvar_vclock]
7f4100f19000-7f4100f1b000 r-xp 00000000 00:00 0                          [vdso]
7ffd87b4d000-7ffd87b6e000 rw-p 00000000 00:00 0                          [stack]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
```

## 如果真的出现时间回退，那么其实无法防护的，

会出现 1hz 的回退

```txt
Seconds: 780, Nanoseconds: 470656038
Seconds: 780, Nanoseconds: 470656620
found bug
Last Seconds: 780, Nanoseconds: 470656620
Now : Seconds: 780, Nanoseconds: 469206041
->Seconds: 780, Nanoseconds: 469206041
->Seconds: 780, Nanoseconds: 469206041
->Seconds: 780, Nanoseconds: 469206041
->Seconds: 780, Nanoseconds: 469206041
->Seconds: 780, Nanoseconds: 469206041
->Seconds: 780, Nanoseconds: 469206041
->Seconds: 780, Nanoseconds: 469206041
->Seconds: 780, Nanoseconds: 469206041
->Seconds: 780, Nanoseconds: 469206041
```

之后的时间一直都是这个。

为什么 4.19 的防护也无效?

## 原来 vdso 也是有 debuginfo 的

/usr/lib/debug/usr/lib/modules/7.1.3-201.fc44.x86_64/vdso
```txt
vdso32.so.debug  vdso64.so.debug
```

## vdso 取时间和直接读 tsc 的开销对比

`vdso/vdso-tsc.c` 直接调用 libc 的 `clock_gettime`/`gettimeofday`/`time`，glibc
默认就把它们走 vDSO；TSC 一侧分别是裸 `RDTSC` 和把 `RDTSC` 用校准出的 mult/shift
换算成 ns（即 vDSO 内部做的一部分工作，但不经过 vvar 的 seqlock，也不做 clockid
分发）。

程序绑定到一个允许使用的 CPU，用 `LFENCE; RDTSC` 夹住被测调用，每组采样 20000 次取
最小值和单位数。TSC 频率用 `CLOCK_MONOTONIC` 忙等 200 ms 校准。所有路径都经过同一个
函数指针间接调用，`baseline` 行就是空的间接调用，用它扣掉调用与计时本身的固定开销。

2026 年在 i9-13900K、CPU 0、TSC 2.995 GHz 上运行：

| 路径                       | min/tick | min/ns | median/ns |
| -------------------------- | -------: | -----: | --------: |
| baseline（空调用）         |       18 |      6 |         7 |
| rdtsc raw                  |       40 |     13 |        14 |
| rdtsc -> ns                |       44 |     14 |        16 |
| clock_gettime MONOTONIC    |       46 |     15 |        16 |
| clock_gettime MONOTONIC_COARSE |   22 |      7 |         8 |
| clock_gettime REALTIME     |       46 |     15 |        16 |
| gettimeofday               |       50 |     16 |        17 |
| time                       |       20 |      6 |         8 |

结论：

- 走 hres 路径的 `clock_gettime` 与直接用 TSC 换算成 ns 的最小值很接近（46 对
  44 tick）。也就是说 vDSO 在 `RDTSC` 之上额外做的 clockid 分发、vvar seqlock 和
  `vdso_calc_ns` 回退检查，开销只比直接读 TSC 多几个 tick。
- 粗粒度路径不读 TSC：`CLOCK_MONOTONIC_COARSE`（22 tick）和 `time`(20 tick)
  只是从 vvar 里读每 tick 刷新的整数值，比 hres 路径快一倍。
- glibc 封装本身几乎不花钱，但三个函数的实现并不一样：
  `gettimeofday`/`time` 是 IFUNC，启动后直接解析到 vDSO 里的函数本身，`dladdr`
  显示它们就位于 `linux-vdso.so.1`，地址与 `__vdso_gettimeofday`、`__vdso_time`
  完全相同。只有 `clock_gettime` 是 libc 里的真实 wrapper：`__clock_gettime` 从
  `_rtld_global_ro` 取出启动时缓存的 `__vdso_clock_gettime` 指针，再间接 `call`
  进去。`dladdr` 显示它在 `libc.so.6` 偏移 0xeca90，与 vDSO 地址不同：
  ```asm
  mov    0x10f415(%rip),%rax   # _rtld_global_ro
  mov    0x2e0(%rax),%rax      # 缓存的 __vdso_clock_gettime
  test   %rax,%rax
  je     fallback              # vDSO 缺失或 clockid 不支持时退回 syscall
  push   %rbp
  mov    %rsp,%rbp
  call   *%rax
  ```
- 这几条额外指令加一次间接 call，在 46 tick 的最小值上测不出来。`clock_gettime`
  之所以不能像前两个那样直接 IFUNC 过去，是因为它还要处理
  `CLOCK_PROCESS_CPUTIME_ID` 这类 vDSO 不支持的 clockid，失败时得退回 `syscall`。
- 数值随硬件和系统状态变化；`mono_ns` 校准用 `clock_gettime`，所以 `rdtsc -> ns`
  的误差与 `g_mult` 的定点误差同量级。


需要注意:

glibc 确实把 COARSE 用在了 time() 上
关键在 sysdeps/unix/sysv/linux/time-clockid.h：

```c
  /* Timer used on clock_gettime for time implementation.  For Linux
     it uses the coarse version which returns the time at the last tick
     and mimic what time as syscall should return.  */
  #define TIME_CLOCK_GETTIME_CLOCKID CLOCK_REALTIME_COARSE
```
对比 sysdeps/generic/time-clockid.h（非 Linux/通用平台）用的是 CLOCK_REALTIME。

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
