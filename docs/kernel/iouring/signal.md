# iouring 对于 signal 的改造
<!-- ef57c075-0622-4f00-a044-0079eae0e8fc -->

## TODO
```diff
History:        #0
Commit:         a7c01fa93aeb03ab76cd3cb2107990dd160498e6
Author:         Jason A. Donenfeld <Jason@zx2c4.com>
Committer:      Eric W. Biederman <ebiederm@xmission.com>
Author Date:    2022年07月12日 星期二 07时21分23秒
Committer Date: 2022年07月18日 星期一 22时53分38秒

signal: break out of wait loops on kthread_stop()

I was recently surprised to learn that msleep_interruptible(),
wait_for_completion_interruptible_timeout(), and related functions
simply hung when I called kthread_stop() on kthreads using them. The
solution to fixing the case with msleep_interruptible() was more simply
to move to schedule_timeout_interruptible(). Why?

The reason is that msleep_interruptible(), and many functions just like
it, has a loop like this:

        while (timeout && !signal_pending(current))
                timeout = schedule_timeout_interruptible(timeout);

The call to kthread_stop() woke up the thread, so schedule_timeout_
interruptible() returned early, but because signal_pending() returned
true, it went back into another timeout, which was never woken up.

This wait loop pattern is common to various pieces of code, and I
suspect that the subtle misuse in a kthread that caused a deadlock in
the code I looked at last week is also found elsewhere.

So this commit causes signal_pending() to return true when
kthread_stop() is called, by setting TIF_NOTIFY_SIGNAL.

The same also probably applies to the similar kthread_park()
functionality, but that can be addressed later, as its semantics are
slightly different.

Cc: Eric W. Biederman <ebiederm@xmission.com>
Signed-off-by: Jason A. Donenfeld <Jason@zx2c4.com>
v1: https://lkml.kernel.org/r/20220627120020.608117-1-Jason@zx2c4.com
v2: https://lkml.kernel.org/r/20220627145716.641185-1-Jason@zx2c4.com
v3: https://lkml.kernel.org/r/20220628161441.892925-1-Jason@zx2c4.com
v4: https://lkml.kernel.org/r/20220711202136.64458-1-Jason@zx2c4.com
v5: https://lkml.kernel.org/r/20220711232123.136330-1-Jason@zx2c4.com
Signed-off-by: Eric W. Biederman <ebiederm@xmission.com>

diff --git a/kernel/kthread.c b/kernel/kthread.c
index 544fd4097406..4507004ca01c 100644
--- a/kernel/kthread.c
+++ b/kernel/kthread.c
@@ -704,6 +704,7 @@ int kthread_stop(struct task_struct *k)
 	kthread = to_kthread(k);
 	set_bit(KTHREAD_SHOULD_STOP, &kthread->flags);
 	kthread_unpark(k);
+	set_tsk_thread_flag(k, TIF_NOTIFY_SIGNAL);
 	wake_up_process(k);
 	wait_for_completion(&kthread->exited);
 	ret = kthread->result;
```
原来这里不是真的设置的地方，真正的地方在:


真正的地方在:
```c
/*
 * Returns 'true' if kick_process() is needed to force a transition from
 * user -> kernel to guarantee expedient run of TWA_SIGNAL based task_work.
 */
static inline bool __set_notify_signal(struct task_struct *task)
{
	return !test_and_set_tsk_thread_flag(task, TIF_NOTIFY_SIGNAL) &&
	       !wake_up_state(task, TASK_INTERRUPTIBLE);
}
```

## 问题
1. 为什么 thread info flags 不将架构无关的部分抽象到特定的位置 ?
2. 无论如何还是想不明白，不就是让类似 workqueue 的取消? 真的有必要这样高

`__set_notify_signal` 最终会被 io_wq_cancel_cb 调用

## TODO
似乎还是需要进入 wq 中，才可以理解为什么有 signal ，但是这样还是无法理解为什么需要添加一个额外的 TIF 标志，

如果只是需要唤醒 wq ，那么没必要做这么多修改，
但是如果是无处不在的等待 (signal_pending)，那么

似乎这就是答案? TIF_NOTIFY_SIGNAL 没有锁的问题?
```diff
History:        #0
Commit:         03941ccfda161c2680147fa5ab92aead2a79cac1
Author:         Jens Axboe <axboe@kernel.dk>
Author Date:    Fri 09 Oct 2020 06:01:33 PM EDT
Committer Date: Sat 12 Dec 2020 11:17:38 AM EST

task_work: remove legacy TWA_SIGNAL path

All archs now support TIF_NOTIFY_SIGNAL.

Signed-off-by: Jens Axboe <axboe@kernel.dk>

diff --git a/kernel/task_work.c b/kernel/task_work.c
index 15b087286bea..9cde961875c0 100644
--- a/kernel/task_work.c
+++ b/kernel/task_work.c
@@ -5,34 +5,6 @@

 static struct callback_head work_exited; /* all we need is ->next == NULL */

-/*
- * TWA_SIGNAL signaling - use TIF_NOTIFY_SIGNAL, if available, as it's faster
- * than TIF_SIGPENDING as there's no dependency on ->sighand. The latter is
- * shared for threads, and can cause contention on sighand->lock. Even for
- * the non-threaded case TIF_NOTIFY_SIGNAL is more efficient, as no locking
- * or IRQ disabling is involved for notification (or running) purposes.
- */
-static void task_work_notify_signal(struct task_struct *task)
-{
-#if defined(TIF_NOTIFY_SIGNAL)
-	set_notify_signal(task);
-#else
-	unsigned long flags;
-
-	/*
-	 * Only grab the sighand lock if we don't already have some
-	 * task_work pending. This pairs with the smp_store_mb()
-	 * in get_signal(), see comment there.
-	 */
-	if (!(READ_ONCE(task->jobctl) & JOBCTL_TASK_WORK) &&
-	    lock_task_sighand(task, &flags)) {
-		task->jobctl |= JOBCTL_TASK_WORK;
-		signal_wake_up(task, 0);
-		unlock_task_sighand(task, &flags);
-	}
-#endif
-}
-
 /**
  * task_work_add - ask the @task to execute @work->func()
  * @task: the task which should run the callback
@@ -76,7 +48,7 @@ int task_work_add(struct task_struct *task, struct callback_head *work,
 		set_notify_resume(task);
 		break;
 	case TWA_SIGNAL:
-		task_work_notify_signal(task);
+		set_notify_signal(task);
 		break;
 	default:
 		WARN_ON_ONCE(1);
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
