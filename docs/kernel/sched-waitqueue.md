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
