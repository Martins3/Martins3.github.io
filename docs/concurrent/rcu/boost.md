# rcu boost

https://stackoverflow.com/questions/32260422/how-rcu-reader-section-is-protected-from-preemption?rq=3

```txt
config RCU_BOOST
	bool "Enable RCU priority boosting"
	depends on (RT_MUTEXES && PREEMPT_RCU && RCU_EXPERT) || PREEMPT_RT
	default y if PREEMPT_RT
	help
	  This option boosts the priority of preempted RCU readers that
	  block the current preemptible RCU grace period for too long.
	  This option also prevents heavy loads from blocking RCU
	  callback invocation.

	  Say Y here if you are working with real-time apps or heavy loads
	  Say N here if you are unsure.
```

## rcu booost 和 rcu_expedited 什么关系
RCU boost 和 rcu_expedited 都用于缩短 RCU grace period，但解决的是不同环节的问题。

```txt
 机制                      解决的问题                                                     主要手段
━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 RCU boost                 某个被抢占的 RCU reader 长时间得不到运行，阻塞 grace period    临时提高阻塞 reader 的调度优先级
────────────────────────  ─────────────────────────────────────────────────────────────  ─────────────────────────────────────────────────────────────────
 expedited grace period    普通 grace period 等待 quiescent state 太慢                    用 IPI、强制调度等更积极的方式，让 CPU 尽快报告 quiescent state
```

它们的关系可以概括为：

```txt
expedited GP
    ├─ 催促 CPU 尽快进入/报告 quiescent state
    └─ 如果仍被 preempted RCU reader 阻塞
           └─ RCU boost 可以提高该 reader 的优先级，使其尽快退出临界区
```

具体来说：

- synchronize_rcu() 等待普通 grace period。
- synchronize_rcu_expedited() 发起 expedited grace period，代价更高，但通常更快。
- CONFIG_RCU_BOOST 主要用于 PREEMPT_RCU，处理低优先级 reader 被抢占后长期阻塞 GP 的优先级反转问题。
- 对普通 GP，RCU 通常等待一段时间后才 boost 阻塞 reader。
- 对 expedited GP，阻塞 expedited GP 的 reader 会更积极、通常立即进入 boost 处理，不再等待普通 GP 的 boost 延迟。
- 如果没有 CONFIG_RCU_BOOST，expedited GP 仍然存在并能催促 CPU，但遇到一个长期得不到调度的 preempted reader 时，仍可能被它拖延。

所以二者不是同一个开关，也不是互相替代：

> rcu_expedited 加速“寻找 quiescent state”，RCU boost 加速“让阻塞中的 reader 完成”。它们可以协同工作。

另外，rcu_expedited 可能指：

- 启动参数 rcupdate.rcu_expedited=1
- sysfs/debugfs 对应开关
- synchronize_rcu_expedited() API

这些控制的是 expedited grace period，不等价于 CONFIG_RCU_BOOST。

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
