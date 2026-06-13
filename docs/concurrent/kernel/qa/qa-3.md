## smp_store_release 为什么不可以满足需求，而必须使用 smp_wmb

没有特别的原因吧，只是用起来比较方便

## 具体例子

### rcuwait

```c
int rcuwait_wake_up(struct rcuwait *w)
{
	int ret = 0;
	struct task_struct *task;

	rcu_read_lock();

	/*
	 * Order condition vs @task, such that everything prior to the load
	 * of @task is visible. This is the condition as to why the user called
	 * rcuwait_wake() in the first place. Pairs with set_current_state()
	 * barrier (A) in rcuwait_wait_event().
	 *
	 *    WAIT                WAKE
	 *    [S] tsk = current	  [S] cond = true
	 *        MB (A)	          MB (B)
	 *    [L] cond		        [L] tsk
	 */
	smp_mb(); /* (B) */

	task = rcu_dereference(w->task);
	if (task)
		ret = wake_up_process(task);
	rcu_read_unlock();

	return ret;
}
EXPORT_SYMBOL_GPL(rcuwait_wake_up);
```

```c
#define ___rcuwait_wait_event(w, condition, state, ret, cmd)
({
	long __ret = ret;
	prepare_to_rcuwait(w);
	for (;;) {
		/*
		 * Implicit barrier (A) pairs with (B) in
		 * rcuwait_wake_up().
		 */
		set_current_state(state);
		if (condition)
			break;

		if (signal_pending_state(state, current)) {
			__ret = -EINTR;
			break;
		}

		cmd;
	}
	finish_rcuwait(w);
	__ret;
})

#define rcuwait_wait_event(w, condition, state)
	___rcuwait_wait_event(w, condition, state, 0, schedule())
```

这里讨论了半天，说明为什么这里 smp_rmb 修改为 smp_mb
https://lore.kernel.org/all/1543590656-7157-1-git-send-email-prsood@codeaurora.org/T/#m72249361e3d32ae81ea414bad269a6843dde5afd

核心就是在 commit message 中:
```diff
commit 6dc080eeb2ba01973bfff0d79844d7a59e12542e
Author: Prateek Sood <prsood@codeaurora.org>
Date:   Fri Nov 30 20:40:56 2018 +0530

    sched/wait: Fix rcuwait_wake_up() ordering

    For some peculiar reason rcuwait_wake_up() has the right barrier in
    the comment, but not in the code.

    This mistake has been observed to cause a deadlock in the following
    situation:

        P1                                  P2

        percpu_up_read()                    percpu_down_write()
          rcu_sync_is_idle() // false
                                              rcu_sync_enter()
                                              ...
          __percpu_up_read()

    [S] ,-  __this_cpu_dec(*sem->read_count)
        |   smp_rmb();
    [L] |   task = rcu_dereference(w->task) // NULL
        |
        |                               [S]     w->task = current
        |                                       smp_mb();
        |                               [L]     readers_active_check() // fail
        `-> <store happens here>

    Where the smp_rmb() (obviously) fails to constrain the store.
```

在这里 P1 : rcuwait_wake_up

P2 : rcuwait_wait_event

如果正确的配置 barrier ，

说明一下，为什么对于这个模式，barrier 就够了，
```txt
condition = 1

rcuwait_wake_up
```

这个居然也是经典的 store load 案例。

### set_current_state 和 __set_current_state 的关系

```c
/*
 * set_current_state() includes a barrier so that the write of current->__state
 * is correctly serialised wrt the caller's subsequent test of whether to
 * actually sleep:
 *
 *   for (;;) {
 *	    set_current_state(TASK_UNINTERRUPTIBLE);
 *	    if (CONDITION)
 *	       break;
 *
 *	    schedule();
 *   }
 *   __set_current_state(TASK_RUNNING);
 *
 * If the caller does not need such serialisation (because, for instance, the
 * CONDITION test and condition change and wakeup are under the same lock) then
 * use __set_current_state().
 *
 * The above is typically ordered against the wakeup, which does:
 *
 *   CONDITION = 1;
 *   wake_up_state(p, TASK_UNINTERRUPTIBLE);
 *
 * where wake_up_state()/try_to_wake_up() executes a full memory barrier before
 * accessing p->__state.
 *
 * Wakeup will do: if (@state & p->__state) p->__state = TASK_RUNNING, that is,
 * once it observes the TASK_UNINTERRUPTIBLE store the waking CPU can issue a
 * TASK_RUNNING store which can collide with __set_current_state(TASK_RUNNING).
 *
 * However, with slightly different timing the wakeup TASK_RUNNING store can
 * also collide with the TASK_UNINTERRUPTIBLE store. Losing that store is not
 * a problem either because that will result in one extra go around the loop
 * and our @cond test will save the day.
 *
 * Also see the comments of try_to_wake_up().
 */
#define __set_current_state(state_value)
	do {
		debug_normal_state_change((state_value));
		WRITE_ONCE(current->__state, (state_value));
	} while (0)

#define set_current_state(state_value)
	do {
		debug_normal_state_change((state_value));
		smp_store_mb(current->__state, (state_value));
	} while (0)
```

以上的内容简化一下，就是:

```txt
while(true){
  set_current_state(TASK_UNINTERRUPTIBLE); // A: 设置状态
  if (CONDITION)                           // B: 检查条件
      break;
  schedule();
}
```

```txt
CONDITION = 1;                           // X: 改变条件
if (@state & p->__state)                 // Y: wake_up_state 的简化逻辑
  p->__state = TASK_RUNNING
```

注意，
```c
#define TASK_RUNNING			    0x00000000
#define TASK_INTERRUPTIBLE		0x00000001
#define TASK_UNINTERRUPTIBLE	0x00000002
```
也就是，wake_up_state 逻辑中，发现如果当前的 task state 是 TASK_RUNNING
那么就直接跳过。


为什么会出现问题和 rcuwait 中类似了。

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
