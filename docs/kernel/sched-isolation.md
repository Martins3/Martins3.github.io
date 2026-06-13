# cpu isolation

- https://stackoverflow.com/questions/38112585/tickless-kernel-isolcpus-nohz-full-and-rcu-nocbs

- https://www.suse.com/c/cpu-isolation-housekeeping-and-tradeoffs-part-4/ : 这个连续好几篇文章
- https://www.suse.com/c/cpu-isolation-full-dynticks-part2/
  - cputime accounting 可以理解，但是 RCU 的不能理解
    - rcu writer 需要知道什么可以开始回收了，这个需要有人通知。

使用场景:
- virtualization hosts that want to maximize CPU resources for the guest
- CPU bound benchmarks for stable results
- specific real time needs, etc……

## 使用
如果打开了
```c
	if (housekeeping_cpu(cpu, HK_TYPE_TICK))
		arch_scale_freq_tick();
```

## 据说，当采用 full dynticks 的时候，会让一些上下文切换开销变大
如下的上下文。
- Syscalls
- Exceptions (page faults, traps, ……)
- IRQs
的开销变大。

开销来源是 cputime accounting 和 RCU tracking and ordering

- [ ] 应该存在一个统一的入口吧!
- 简单找了下，没有找到对应位置的证据。

## 分析一下 sched_isolation.c
- [ ] housekeeping_affine

## 提供了两个 kernel 参数
1. `nohz_full=` 自动包含了 `rcu_nocbs`
2. `isolcpus=` : 控制到底 isolate 啥
3. `rcu_nocbs=`
