# wait_queue
wait_queue 的机制 : 将自己加入到队列，然后睡眠，之后其他的 thread 利用 wake up 将队列出队, 并且执行事先注册好的函数，这个函数一般就是 try_to_wake_up, 从而达到 wait 事件的过程自己在睡眠

- [x] wait_event : sleep until a condition gets true
- [x] add_wait_queue  : 加入队列， 当 wake_up 的时候会调用被 init_waitqueue_func_entry 初始化的函数
- [x] wake_up : 将队列中间的 entry delte 并且执行事先注册的函数

- [ ] exclusive task

- wake_up
  - `__wake_up_common_lock`
    - `__wake_up_common`

- wait_event
  - `__wait_event`

```c
#define ___wait_event(wq_head, condition, state, exclusive, ret, cmd)       \
({                                      \
    __label__ __out;                            \
    struct wait_queue_entry __wq_entry;                 \
    long __ret = ret;   /* explicit shadow */               \
                                        \
    init_wait_entry(&__wq_entry, exclusive ? WQ_FLAG_EXCLUSIVE : 0);    \
    for (;;) {                              \
        long __int = prepare_to_wait_event(&wq_head, &__wq_entry, state);\
                                        \
        if (condition)                          \
            break;                          \
                                        \
        if (___wait_is_interruptible(state) && __int) {         \
            __ret = __int;                      \
            goto __out;                     \
        }                               \
                                        \
        cmd;                                \
    }                                   \
    finish_wait(&wq_head, &__wq_entry);                 \
__out:  __ret;                                  \
})
// 其中的，init_wait_entry 将会设置被移动出来队列的时候，设置的 function 导致其被自动运行
```

## 一些有趣的接口
- `wait_event_lock_irq`
- #define wait_event_lock_irq_cmd(wq_head, condition, lock, cmd)			\

## waitqueue
1. https://github.com/cirosantilli/linux-kernel-module-cheat/blob/master/kernel_modules/wait_queue.c : 还是要好好分析一下其中的代码呀 !


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
