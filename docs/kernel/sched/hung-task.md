# hung task 机制

每次上下文切换的时候，
```c
struct task_struct {
// ...
#ifdef CONFIG_DETECT_HUNG_TASK
	unsigned long			last_switch_count;
	unsigned long			last_switch_time;
#endif
// ...
}
```

- watchdog : 启动一个循环线程
  - check_hung_uninterruptible_tasks
    - for_each_process_thread : check_hung_task ，就是检查一个 task 如果处于如下状态，那么报告
      - ((state & TASK_UNINTERRUPTIBLE) && !(state & TASK_WAKEKILL) && !(state & TASK_NOLOAD))
  - schedule_timeout_interruptible : 睡眠 120s，然后重新检测

也就是进入到内核中，并且在用户态杀不掉的时候，那么会出现问题。

- 让当前进入 unkillable ，然后等待 lock ，应该可以造成这种现象。

整理一下
https://blog.cloudflare.com/es-la/searching-for-the-cause-of-hung-tasks-in-the-linux-kernel/

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
