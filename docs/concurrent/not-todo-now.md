# 并发原子操作问题分析

1. https://www.kawabangga.com/posts/4777
> 虽然 atomic 也是一个 CAS 操作

atomic 和 CAS 没有什么关系吧

2. https://github.com/cpp-taskflow/cpp-taskflow
  - https://github.com/taskflow/awesome-parallel-computing
  - 仅仅是任务编排？

3. 时钟也是需要 ordered 的
如果不使用 memory barrier ，为什么就会导致 tsc 返回的时间出现回退 ？
可以尝试一下.

```c
/*
 * We used to compare the TSC to the cycle_last value in the clocksource
 * structure to avoid a nasty time-warp. This can be observed in a
 * very small window right after one CPU updated cycle_last under
 * xtime/vsyscall_gtod lock and the other CPU reads a TSC value which
 * is smaller than the cycle_last reference value due to a TSC which
 * is slightly behind. This delta is nowhere else observable, but in
 * that case it results in a forward time jump in the range of hours
 * due to the unsigned delta calculation of the time keeping core
 * code, which is necessary to support wrapping clocksources like pm
 * timer.
 *
 * This sanity check is now done in the core timekeeping code.
 * checking the result of read_tsc() - cycle_last for being negative.
 * That works because CLOCKSOURCE_MASK(64) does not mask out any bit.
 */
static u64 read_tsc(struct clocksource *cs)
{
	return (u64)rdtsc_ordered();
}
```

正好这个项目演示了在用户态如何读取 tsc 的输出:
https://github.com/dterei/tsc


## 似乎可以解释一个问题，如果一个 process 在查询，另外一个 process 在删除
对于这个目录是可以不用上锁的原因就是用了 rcu 吧

- https://lwn.net/Articles/649729 RCU-walk: faster pathname lookup in Linux

## 如何理解
```c
static u64 kvm_steal_clock(int cpu)
{
	u64 steal;
	struct kvm_steal_time *src;
	int version;

	src = &per_cpu(steal_time, cpu);
	do {
		version = src->version;
		virt_rmb();
		steal = src->steal;
		virt_rmb();
	} while ((version & 1) || (version != src->version));

	return steal;
}
```

record_steal_time 中对应的:

4.19 之前的写法是:
```c
	smp_wmb();

	vcpu->arch.st.steal.steal += current->sched_info.run_delay -
		vcpu->arch.st.last_steal;
	vcpu->arch.st.last_steal = current->sched_info.run_delay;

	kvm_write_guest_cached(vcpu->kvm, &vcpu->arch.st.stime,
		&vcpu->arch.st.steal, sizeof(struct kvm_steal_time));

	smp_wmb();
```

6.12 的写法是:
```c
	unsafe_get_user(version, &st->version, out);
	if (version & 1)
		version += 1;  /* first time write, random junk */

	version += 1;
	unsafe_put_user(version, &st->version, out);

	smp_wmb();
```

所以这里的问题是:
1. virt_rmb 和 smp_rmb 在什么时候会有不同，至少在 x86 中，他们是有不同的
2. 为什么 6.12 的写法中，没有匹配的 smp_rmb ?


## 为什么 x86 需要的代码这里需要使用 release
```c
static inline void kvm_apic_set_x2apic_id(struct kvm_lapic *apic, u32 id)
{
	u32 ldr = kvm_apic_calc_x2apic_ldr(id);

	WARN_ON_ONCE(id != apic->vcpu->vcpu_id);

	kvm_lapic_set_reg(apic, APIC_ID, id);
	kvm_lapic_set_reg(apic, APIC_LDR, ldr);
	atomic_set_release(&apic->vcpu->kvm->arch.apic_map_dirty, DIRTY);
}
```

## 使用调度器来调试
https://mp.weixin.qq.com/s/qm6xywiNWUAPf9NdhNlefQ

## 居然代码中是需要感知到这些

```diff
History:        #0
Commit:         a7b480e7f30b3813353ec009f10f2ac7a6669f3b
Author:         Borislav Petkov <borislav.petkov@amd.com>
Committer:      H. Peter Anvin <hpa@zytor.com>
Author Date:    Fri 22 Jan 2010 11:01:03 PM CST
Committer Date: Sat 23 Jan 2010 08:05:42 AM CST

x86, lib: Add wbinvd smp helpers

Add wbinvd_on_cpu and wbinvd_on_all_cpus stubs for executing wbinvd on a
particular CPU.

[ hpa: renamed lib/smp.c to lib/cache-smp.c ]
[ hpa: wbinvd_on_all_cpus() returns int, but wbinvd() returns
  void.  Thus, the former cannot be a macro for the latter,
  replace with an inline function. ]

Signed-off-by: Borislav Petkov <borislav.petkov@amd.com>
LKML-Reference: <1264172467-25155-2-git-send-email-bp@amd64.org>
Signed-off-by: H. Peter Anvin <hpa@zytor.com>
```

## 分布式中的各种一致性


## 回顾一下操作系统中是如何讲解并行的
为什么操作系统中的大多数内容都是讲锁?
- Modern Operating System
- 现在操作系统(上交)
- Three Easy pices

## 有趣的研究
https://github.com/open-s4c

## 看看 pthread 在 windows 或者 macos 中的替代品

## 参考 perf book 4.2.5 中的内容看看 atomic 内容

## What every systems programmer should know about concurrency
https://github.com/mrkline/concurrency-primer?tab=readme-ov-file
https://assets.bitbashing.io/papers/concurrency-primer.pdf

2020 写的，其实不错的

## windows mutex 曾经出现过 bug
https://news.ycombinator.com/item?id=39581664

- https://jamesbornholt.com/blog/memory-models/
  - https://news.ycombinator.com/item?id=40350201

## 如果分析了二进制翻译器的代码，那么将是绝杀!
https://github.com/saagarjha/TSOEnabler

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
