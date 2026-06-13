## rcu boost

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
