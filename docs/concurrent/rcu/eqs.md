## 如何理解什么 extended quiescent state

也许先理解了 eqs 才可以理解 context_tracking 吧 ?

```c
/*
 * Is the current CPU in an extended quiescent state?
 *
 * No ordering, as we are sampling CPU-local information.
 */
static __always_inline bool rcu_dynticks_curr_cpu_in_eqs(void)
{
	return !(raw_atomic_read(this_cpu_ptr(&context_tracking.state)) & RCU_DYNTICKS_IDX);
}
```

https://www.suse.com/c/cpu-isolation-full-dynticks-part2/

其中的 3.2 RCU quiescent states reporting 解释的很好:


1. 如果有 tick ，就没有必要使用这个概念，因为 : Because that would inflict a costly atomic operation with a full memory barrier on every user/kernel round trip. Also the duty to report quiescent states eventually falls to other CPUs so one must be aware of the exported cost that implies.
2. 如果没有，那么利用进入到 idle 和 userspace 可以知道，CPU 一定不是在 rcu critical region 中的

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
