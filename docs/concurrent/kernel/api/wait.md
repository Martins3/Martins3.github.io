# wait

## 核心函数

```c
#define ___wait_event(wq_head, condition, state, exclusive, ret, cmd)
// 其中的，init_wait_entry 将会设置被移动出来队列的时候，设置的 cmd 导致其被自动运行
```

```c
/*
 * The core wakeup function. Non-exclusive wakeups (nr_exclusive == 0) just
 * wake everything up. If it's an exclusive wakeup (nr_exclusive == small +ve
 * number) then we wake that number of exclusive tasks, and potentially all
 * the non-exclusive tasks. Normally, exclusive tasks will be at the end of
 * the list and any non-exclusive tasks will be woken first. A priority task
 * may be at the head of the list, and can consume the event without any other
 * tasks being woken.
 *
 * There are circumstances in which we can try to wake a task which has already
 * started to run but is not in state TASK_RUNNING. try_to_wake_up() returns
 * zero in this (rare) case, and we handle it by continuing to scan the queue.
 */
static int __wake_up_common(struct wait_queue_head *wq_head, unsigned int mode,
			int nr_exclusive, int wake_flags, void *key)
```

遍历链表:

1. 最多唤醒 nr_exclusive
2. 将 exclusive 的放到尾端，非 exclusive 的自动唤醒
3. 高优先级的仅仅唤醒一次

## 为什么存在使用定制的 wait up 函数的需求?

1. 包括 wait_event 在内的函数都是使用这个来定义的:

```c
void init_wait_entry(struct wait_queue_entry *wq_entry, int flags)
{
	wq_entry->flags = flags;
	wq_entry->private = current;
	wq_entry->func = autoremove_wake_function; // wake up 并且从队列中拿出去
	INIT_LIST_HEAD(&wq_entry->entry);
}
```

2. 部分是这样的

```c
int woken_wake_function(struct wait_queue_entry *wq_entry, unsigned mode, int sync, void *key)
{
	/* Pairs with the smp_store_mb() in wait_woken(). */
	smp_mb(); /* C */
	wq_entry->flags |= WQ_FLAG_WOKEN;

	return default_wake_function(wq_entry, mode, sync, key);
}
```
和 wait_woken 配合使用

3. 部分是这样的，只有 default_wake_function
```c
#define __WAITQUEUE_INITIALIZER(name, tsk) {					\
	.private	= tsk,							\
	.func		= default_wake_function,				\
	.entry		= { NULL, NULL } }

#define DECLARE_WAITQUEUE(name, tsk)						\
	struct wait_queue_entry name = __WAITQUEUE_INITIALIZER(name, tsk)
```

## 太牛了

居然存在这种考虑:

https://lwn.net/Articles/628628/

例如，但是现在还存在好多这种写法，例如:
raid1_write_request

好吧，前提

## head 的 spinlock_t 是做啥的?

可以换成 rcu 吗?

```c
struct wait_queue_head {
	spinlock_t		lock;
	struct list_head	head;
};
```

## 这里也有一些解释

```c
/*
 * The page wait code treats the "wait->flags" somewhat unusually, because
 * we have multiple different kinds of waits, not just the usual "exclusive"
 * one.
 *
 * We have:
 *
 *  (a) no special bits set:
 *
 *	We're just waiting for the bit to be released, and when a waker
 *	calls the wakeup function, we set WQ_FLAG_WOKEN and wake it up,
 *	and remove it from the wait queue.
 *
 *	Simple and straightforward.
 *
 *  (b) WQ_FLAG_EXCLUSIVE:
 *
 *	The waiter is waiting to get the lock, and only one waiter should
 *	be woken up to avoid any thundering herd behavior. We'll set the
 *	WQ_FLAG_WOKEN bit, wake it up, and remove it from the wait queue.
 *
 *	This is the traditional exclusive wait.
 *
 *  (c) WQ_FLAG_EXCLUSIVE | WQ_FLAG_CUSTOM:
 *
 *	The waiter is waiting to get the bit, and additionally wants the
 *	lock to be transferred to it for fair lock behavior. If the lock
 *	cannot be taken, we stop walking the wait queue without waking
 *	the waiter.
 *
 *	This is the "fair lock handoff" case, and in addition to setting
 *	WQ_FLAG_WOKEN, we set WQ_FLAG_DONE to let the waiter easily see
 *	that it now has the lock.
 */
static int wake_page_function(wait_queue_entry_t *wait, unsigned mode, int sync, void *arg)
```

## 想不到 ldd3 上居然分析过

- https://www.makelinux.net/ldd3/chp-6-sect-2.shtml

- https://stackoverflow.com/questions/39893500/linux-wait-queue-combination-of-exclusive-and-non-exclusive

- https://jacktang816.github.io/post/jingqun/

## waitqueue

do_wait() : 每个 `task_struct->signal->wait_chldexit` 上放置 wait queue

```c
  // TODO child_wait_callback 函数调用的时机 : 元素加入 还是 元素离开
  // child_wait_callback 会唤醒 current
	init_waitqueue_func_entry(&wo->child_wait, child_wait_callback);
	wo->child_wait.private = current; // 用于唤醒
	add_wait_queue(&current->signal->wait_chldexit, &wo->child_wait);

 // 最终去掉，如果捕获了多个 thread
 remove_wait_queue
```

wake up : do_notify_parent_cldstop 和 do_notify_parent

## [ ] 如何理解 TASK_NORMAL

```c
/* Convenience macros for the sake of wake_up(): */
#define TASK_NORMAL			(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)


#define wake_up(x)			__wake_up(x, TASK_NORMAL, 1, NULL)
#define wake_up_nr(x, nr)		__wake_up(x, TASK_NORMAL, nr, NULL)
#define wake_up_all(x)			__wake_up(x, TASK_NORMAL, 0, NULL)
#define wake_up_locked(x)		__wake_up_locked((x), TASK_NORMAL, 1)
#define wake_up_all_locked(x)		__wake_up_locked((x), TASK_NORMAL, 0)
```

什么叫做又 TASK_INTERRUPTIBLE 又 TASK_UNINTERRUPTIBLE 的?

## [ ]  DECLARE_WAITQUEUE 和 DEFINE_WAIT 的区别

找到的内容不多:
```txt
commit 3e3b5c087799e536871c8261b05bc28e4783c8da
Author: Jiri Slaby <jirislaby@kernel.org>
Date:   Thu Jun 11 14:33:37 2009 +0100

    tty: use prepare/finish_wait

    Use prepare_to_wait and finish_wait instead of add_wait_queue and
    remove_wait_queue.

    This avoids us setting a task state.

    Signed-off-by: Jiri Slaby <jirislaby@gmail.com>
    Signed-off-by: Alan Cox <alan@linux.intel.com>
    Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

似乎 DECLARE_WAITQUEUE 基本放弃了?

但是 tun_ring_recv 正好演示了一个例子，而且确实是增加了设置 set_current_state 的操作 :
```txt
	__set_current_state(TASK_RUNNING);
```
但是很多使用这个的地方也不会重新设置 TASK_RUNNING

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
