## context_tracking

简而言之是用来做 context switch tracking ，然后来辅助 rcu 的。

kernel/context_tracking.c

```c
struct context_tracking {
#ifdef CONFIG_CONTEXT_TRACKING_USER
	/*
	 * When active is false, probes are unset in order
	 * to minimize overhead: TIF flags are cleared
	 * and calls to user_enter/exit are ignored. This
	 * may be further optimized using static keys.
	 */
	bool active;
	int recursion;
#endif
#ifdef CONFIG_CONTEXT_TRACKING
	atomic_t state;
#endif
#ifdef CONFIG_CONTEXT_TRACKING_IDLE
	long dynticks_nesting;		/* Track process nesting level. */
	long dynticks_nmi_nesting;	/* Track irq/NMI nesting level. */
#endif
};
```

## 基本观察
```txt
🧀  zcat /proc/config.gz | grep CONTEXT_TRACKING
CONFIG_CONTEXT_TRACKING=y
CONFIG_CONTEXT_TRACKING_IDLE=y
CONFIG_CONTEXT_TRACKING_USER=y
# CONFIG_CONTEXT_TRACKING_USER_FORCE is not set
CONFIG_HAVE_CONTEXT_TRACKING_USER=y
CONFIG_HAVE_CONTEXT_TRACKING_USER_OFFSTACK=y
```

```txt
config CONTEXT_TRACKING_USER
	bool
	depends on HAVE_CONTEXT_TRACKING_USER
	select CONTEXT_TRACKING
	help
	  Track transitions between kernel and user on behalf of RCU and
	  tickless cputime accounting. The former case relies on context
	  tracking to enter/exit RCU extended quiescent states.

config CONTEXT_TRACKING_USER_FORCE
	bool "Force user context tracking"
	depends on CONTEXT_TRACKING_USER
	default y if !NO_HZ_FULL
	help
	  The major pre-requirement for full dynticks to work is to
	  support the user context tracking subsystem. But there are also
	  other dependencies to provide in order to make the full
	  dynticks working.

	  This option stands for testing when an arch implements the
	  user context tracking backend but doesn't yet fulfill all the
	  requirements to make the full dynticks feature working.
	  Without the full dynticks, there is no way to test the support
	  for user context tracking and the subsystems that rely on it: RCU
	  userspace extended quiescent state and tickless cputime
	  accounting. This option copes with the absence of the full
	  dynticks subsystem by forcing the user context tracking on all
	  CPUs in the system.

	  Say Y only if you're working on the development of an
	  architecture backend for the user context tracking.

	  Say N otherwise, this option brings an overhead that you
	  don't want in production.
```

```txt
config CONTEXT_TRACKING
	bool

config CONTEXT_TRACKING_IDLE
	bool
	select CONTEXT_TRACKING
	help
	  Tracks idle state on behalf of RCU.
```

### 简单分析下
```txt
sudo bpftrace -e "tracepoint:rcu:rcu_dyntick { @[kstack] = count(); }"
```

这个是 rcu_dyntick ，居然可以得到这样的结果:
```txt
@[
    ct_kernel_enter.isra.0+188
    ct_kernel_enter.isra.0+188
    ct_idle_exit+30
    cpuidle_enter_state+811
    cpuidle_enter+45
    do_idle+436
    cpu_startup_entry+41
    start_secondary+284
    common_startup_64+318
]: 31214
```

- ct_kernel_exit
- ct_kernel_enter

## 这么复杂吗?
- https://lore.kernel.org/linux-kernel//465c71e018de9800ba22a84b9c16f56f99aabefd.camel@kernel.org/T/#m72d553398b545c724d5a531b51eb57941abddc74

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
