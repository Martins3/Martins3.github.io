# QEMU 中的线程，事件循环和锁

QEMU 的执行流程大致来说是分为 io thread 和 vCPU thread 的。

![https://martins3.github.io/ppt/repo/2021-8-24/index.html](https://martins3.github.io/ppt/images/QEMU-ARCH.svg)

**一般来说**:
- 这个 io thread 就是指的是 main-loop.c 中 `qemu_main_loop` 执行的循环。
- vCPU 的取决于具体的 accel 是什么，`AccelOpsClass::create_vcpu_thread` 上会注册具体的 hook, 例如 kvm 注册的 kvm_start_vcpu_thread


对于双核配置，使用 gdb 的 `info thread` [^8][^9] 
```plain
  Id   Target Id                                             Frame
* 1    Thread 0x7fffeb1d4300 (LWP 1389363) "qemu-system-x86" 0x00007ffff61a6bf6 in __ppoll (fds=0x555556ad10f0, nfds=8, timeout=<optimized out>, timeout@entry=0x7fffffffd2c0, sigmask=sigmask@entry=0x0) at ../sysdeps/unix/sysv/linux/ppoll.c:44
  2    Thread 0x7fffeb073700 (LWP 1389367) "qemu-system-x86" syscall () at ../sysdeps/unix/sysv/linux/x86_64/syscall.S:38
  3    Thread 0x7fffea5fb700 (LWP 1389373) "gmain"           0x00007ffff61a6aff in __GI___poll (fds=0x5555569c41e0, nfds=1, timeout=-1) at ../sysdeps/unix/sysv/linux/poll.c:29
  4    Thread 0x7fffe9dfa700 (LWP 1389374) "gdbus"           0x00007ffff61a6aff in __GI___poll (fds=0x5555569cfe40, nfds=2, timeout=-1) at ../sysdeps/unix/sysv/linux/poll.c:29
  5    Thread 0x7fffe93f6700 (LWP 1389377) "qemu-system-x86" 0x00007ffff61a6bf6 in __ppoll (fds=0x7fffd4001ff0, nfds=1, timeout=<optimized out>, timeout@entry=0x0, sigmask=sigmask@entry=0x0) at ../sysdeps/unix/sysv/linux/ppoll.c:44
  6    Thread 0x7fffe8af4700 (LWP 1389378) "qemu-system-x86" 0x00007ffff6296618 in futex_abstimed_wait_cancelable (private=0, abstime=0x7fffe8af0220, clockid=0, expected=0, futex_word=0x555556730c78) at ../sysdeps/nptl/futex-internal.h:320
  7    Thread 0x7ffe51dff700 (LWP 1389381) "qemu-system-x86" 0x00007ffff61a850b in ioctl () at ../sysdeps/unix/syscall-template.S:78
  8    Thread 0x7ffe515fe700 (LWP 1389382) "qemu-system-x86" 0x00007ffff61a850b in ioctl () at ../sysdeps/unix/syscall-template.S:78
  9    Thread 0x7ffe48e29700 (LWP 1389385) "threaded-ml"     0x00007ffff61a6aff in __GI___poll (fds=0x7ffe38007170, nfds=3, timeout=-1) at ../sysdeps/unix/sysv/linux/poll.c:29
  10   Thread 0x7ffe29ddd700 (LWP 1389387) "qemu-system-x86" 0x00007ffff6296618 in futex_abstimed_wait_cancelable (private=0, abstime=0x7ffe29dd9220, clockid=0, expected=0, futex_word=0x555556730c78) at ../sysdeps/nptl/futex-internal.h:320
```

在 `qemu_thread_create` 中添加调试语句:
```plain
call_rcu
IO io0
CPU 0/KVM
CPU 1/KVM
worker
worker
```

分析上面的内容，可以看到:
- 加上 main loop 线程，`qemu_thread_create` 创建的线程是和 `info thread` 中叫做 "qemu-system-x86" 的线程数目完全对应的。
- 两个 vCPU 分别对应一个线程
- 有一些看不懂的 gmain / gdbus / threaded-ml

下面逐个分析一下:

## gmain / gdbus / threaded-ml

通过 `thread ${pid_num}` 和 `backtrace` 可以获取这几个 thread 的内部的执行流程。

比如 threaded-ml 的，gmain 和 gdbus 和这个类似，不列举了。
总之就是这些线程会调用到 poll 系统调用上, 来监听一些事情。
```c
/*
>>> thread 8
[Switching to thread 8 (Thread 0x7ffe51629700 (LWP 1186997))]
#0  0x00007ffff61a6aff in __GI___poll (fds=0x7ffe3c007170, nfds=3, timeout=-1) at ../sysdeps/unix/sysv/linux/poll.c:29
29      ../sysdeps/unix/sysv/linux/poll.c: No such file or directory.
>>> bt
#0  0x00007ffff61a6aff in __GI___poll (fds=0x7ffe3c007170, nfds=3, timeout=-1) at ../sysdeps/unix/sysv/linux/poll.c:29
#1  0x00007ffff6df31d6 in  () at /lib/x86_64-linux-gnu/libpulse.so.0
#2  0x00007ffff6de4841 in pa_mainloop_poll () at /lib/x86_64-linux-gnu/libpulse.so.0
#3  0x00007ffff6de4ec3 in pa_mainloop_iterate () at /lib/x86_64-linux-gnu/libpulse.so.0
#4  0x00007ffff6de4f70 in pa_mainloop_run () at /lib/x86_64-linux-gnu/libpulse.so.0
#5  0x00007ffff6df311d in  () at /lib/x86_64-linux-gnu/libpulse.so.0
#6  0x00007ffff56f272c in  () at /usr/lib/x86_64-linux-gnu/pulseaudio/libpulsecommon-13.99.so
#7  0x00007ffff628c609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#8  0x00007ffff61b3293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```

这几个 thread 不是通过 `qemu_thread_create` 创建的，使用 gdb 在 `clone` 地方打断点，然后逐个 `backtrace` 可以看到所有的 thread 是如何创建的。
下面是 threaded-ml 的创建的过程:
```c
/*
#0  clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:50
#1  0x00007ffff628b2ec in create_thread (pd=pd@entry=0x7ffe48e29700, attr=attr@entry=0x7fffffffcf10, stopped_start=stopped_start@entry=0x7fffffffcf0e, stackaddr=stackad
dr@entry=0x7ffe48e25400, thread_ran=thread_ran@entry=0x7fffffffcf0f) at ../sysdeps/unix/sysv/linux/createthread.c:101
#2  0x00007ffff628ce10 in __pthread_create_2_1 (newthread=<optimized out>, attr=<optimized out>, start_routine=<optimized out>, arg=<optimized out>) at pthread_create.c
:817
#3  0x00007ffff56f3eb4 in pa_thread_new () at /usr/lib/x86_64-linux-gnu/pulseaudio/libpulsecommon-13.99.so
#4  0x00007ffff6dc639b in pa_threaded_mainloop_start () at /lib/x86_64-linux-gnu/libpulse.so.0
#5  0x00007ffe51226e33 in pulse_driver_open () at /usr/lib/x86_64-linux-gnu/libcanberra-0.30/libcanberra-pulse.so
#6  0x00007fffe957219b in  () at /lib/x86_64-linux-gnu/libcanberra.so.0
#7  0x00007fffe9569538 in  () at /lib/x86_64-linux-gnu/libcanberra.so.0
#8  0x00007fffe956a1ef in ca_context_play_full () at /lib/x86_64-linux-gnu/libcanberra.so.0
#9  0x00007ffff7fb5648 in ca_gtk_play_for_widget () at /lib/x86_64-linux-gnu/libcanberra-gtk3.so.0
#10 0x00007ffff7fbcc6e in  () at /usr/lib/x86_64-linux-gnu/gtk-3.0/modules/libcanberra-gtk-module.so
#11 0x00007ffff6fa0f4d in  () at /lib/x86_64-linux-gnu/libgdk-3.so.0
#12 0x00007ffff787a04e in g_main_context_dispatch () at /lib/x86_64-linux-gnu/libglib-2.0.so.0
#13 0x0000555555e6d1a8 in glib_pollfds_poll () at ../util/main-loop.c:232
#14 os_host_main_loop_wait (timeout=<optimized out>) at ../util/main-loop.c:255
#15 main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:531
#16 0x0000555555c190d1 in qemu_main_loop () at ../softmmu/runstate.c:726
#17 0x0000555555940ab2 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:50
```

gmain 和 gdbus 类似，只是从 `early_gtk_display_init` 开始，然后经过层层的在 gtk 库函数的调用

所以，现在可以基本确定一个事情，那就是这几个与众不同的 thread 是 gtk 处理图形界面和音频创建的出来的。
这些东西的处理都是被 glib 库封装好了，之后没有必要关注了。

## worker
总体来说，worker pool 的设计比较简单的，整个 thread-pool.c 也就是只有 300 行左右, 这个主要关联的两个结构体:

```c
struct ThreadPool {
    QemuSemaphore sem;　// 工作线程idle时休眠的信号量

    /* The following variables are protected by lock.  */
    QTAILQ_HEAD(, ThreadPoolElement) request_list;
};

struct ThreadPoolElement {
    ThreadPool *pool;　    // 所属线程池
    ThreadPoolFunc *func;　// 要在线程池中完成的工作
    void *arg;　           // 线程池中完成的工作的参数

    /* Access to this list is protected by lock.  */
    QTAILQ_ENTRY(ThreadPoolElement) reqs; // 通过这个将自己放到 ThreadPool::request_list 上
};
```

- thread_pool_submit_aio : 将任务提交给 thread pool，如果 pool 中没有 idle thread，会调用 spawn_thread 来创建
- worker_thread 和核心执行流程，在 thread_pool_submit_aio 中 qemu_sem_post(ThreadPool::sem) 会让 worker_thread 从这个 lock 上醒过来
然后会从 ThreadPool::request_list 中获取需要执行的函数，最后使用 `qemu_bh_schedule(pool->completion_bh)` 通知这个任务结束了

- 在 worker_thread 中，qemu_sem_timedwait(ThreadPool::sem) 最多只会等待 10s 如果没有任务过来，那么这个 thread 结束。

## call_rcu
RCU 在 Linux 内核中设计的非常的巧妙，当然也非常的复杂和难以掌握。
LWN 提供了[一系列的文章](https://lwn.net/Kernel/Index/#Read-copy-update) 来分析解释内核中 RCU 的设计。
其中 [What is RCU, Fundamentally?](https://lwn.net/Articles/262464/) 中的
Example 1: Maintaining Multiple Versions During Deletion 和 Example 2: Maintaining Multiple Versions During Replacement
用于理解 RCU 的原理算是相当的生动形象了。

虽然原理相同，QEMU 中的 RCU 设计的更加简单和容易理解。

下面的分析使用 RAMList::dirty_memory 作为一个分析的例子:

```c
typedef struct RAMList {
    // ...
    DirtyMemoryBlocks *dirty_memory[DIRTY_MEMORY_NUM];
    // ...
}
```
从 writer 的角度分析，做了两件事情
- 让 RAMList::dirty_memory 存储新的 DirtyMemoryBlocks 地址
- 释放老的 DirtyMemoryBlocks

```c
static void dirty_memory_extend(ram_addr_t old_ram_size,
                                ram_addr_t new_ram_size){
        new_blocks = g_malloc(sizeof(*new_blocks) +
                              sizeof(new_blocks->blocks[0]) * new_num_blocks);
        qatomic_rcu_set(&ram_list.dirty_memory[i], new_blocks);

        g_free_rcu(old_blocks, rcu);
}
```
- 如果一个 reader 从 RAMList::dirty_memory 中获取的就是新的 DirtyMemoryBlocks 地址，之后一切访问正常。
- 如果一个 reader 在更新 RAMList::dirty_memory 之前访问，获取的是旧的的 DirtyMemoryBlocks，现在是不能立刻将其释放掉的。需要等待 reader 都结束了才可以释放。
- 无论上面的哪一个情况，reader 通过 RAMList::dirty_memory 获取的 DirtyMemoryBlocks 总是 atomic 状态的，而不是一部分修改了，一部分没有修改，这是正确性的保证。

所以，现在只有一个问题，什么时候可以回收垃圾。

再看 reader 这一侧，使用 cpu_physical_memory_get_dirty 作为例子:
```c
static inline bool cpu_physical_memory_get_dirty(ram_addr_t start,
                                                 ram_addr_t length,
                                                 unsigned client)
{
    WITH_RCU_READ_LOCK_GUARD() {
      // 访问
    }
    return dirty;
}

```

WITH_RCU_READ_LOCK_GUARD 会展开为:

```txt
- rcu_read_auto_lock
  - rcu_read_lock
    - `rcu_reader->ctr = rcu_gp_ctr->ctr` : 在进入的时候更新当前的

// 中间进行访问

- rcu_read_auto_unlock
  - rcu_read_unlock
    - 如果检测到 rcu_reader::waiting 的话，`qemu_event_set(&rcu_gp_event);`
```

也就是 reader critical region 开始和结束的时候都做出了标记.

| var                  |                                                                                    |
|----------------------|------------------------------------------------------------------------------------|
| rcu_gp_ctr           | 全局变量，用于标记当前的 period                                                    |
| rcu_reader           | 每一个线程的局部变量，当 reader 进入 critical reagion 的时候，会和 rcu_gp_ctr 同步 |
| rcu_call_ready_event | 在 call_rcu1 中用于通知 `call_rcu` thread 有垃圾可以回收了                         |
| rcu_gp_event         | 在 rcu_read_unlock 中用于通知 `call_rcu` thread 有 reader 结束了                   |


- 如果写的过程中，没有读者，无所谓。有问题的是，删除的过程中，还有 reader
  - 如何来记录的?
    - 开始删除之前，synchronize_rcu 将 rcu_gp_ctr ++
    - 那些在 synchronize_rcu 之前的开始读，尚且没有结束的 rcu_gp_ctr 和 rcu_reader 的数值会不相等
      - 也就是 rcu_gp_ongoing 为真
      - 这些 thread 的 rcu_reader::waiting = true, 也会让 wait_for_readers 睡眠下去
      - rcu_read_auto_lock 导致 qemu_event_set

## [^2]
QEMU RCU core has a global counter named 'rcu_gp_ctr' which is used by both readers and updaters.
Every thread has a thread local variable of 'ctr' counter in 'rcu_reader_data' struct.

When the `synchronize_rcu` find that the readers' `ctr` is not the same as the ‘rcu_gp_ctr’
it will set the `rcu_reader_data->waiting` bool variable, and when the `rcu_read_unlock` finds this bool variable
is set it will trigger a event thus notify the `synchronize_rcu` that it leaves the critical section.

> 做法应该是: rcu_read_lock 从 rcu_gp_ctr 从拷贝版本号，当离开的时候，如果发现此时的版本号 和当时拷贝的不同，那么意味着自己之前在使用老的资源，那么需要开始告知

- `qemu_event_set(&rcu_gp_event)` 告知

- rcu_init
  - rcu_init_complete
    - call_rcu_thread : 启动 rcu 回收线程
      - 第一个 while 循环: 需要等待有人调用 call_rcu1 才可以, 然后等待一段时间
      - synchronize_rcu
        - 修改 rcu_gp_ctr, 表示进入到的 period 了
        - wait_for_readers : 流程很清晰
          1. `static ThreadList registry = QLIST_HEAD_INITIALIZER(registry);` : 在 rcu_register_thread 的时候，将 thread local 的 rcu_reader 挂到上面去
          2. 对于 register 上挂载的 rcu_reader 调用 rcu_gp_ongoing 查询 local 的版本和 global 的版本是否存在差别，如果有，那么设置 rcu_reader_data::waiting 为 true, 如果版本相同，那么从 registry 中移除掉
          3. QLIST_EMPTY(&registry) : 这表示所有的 reader 都离开 critical region 了
      - try_dequeue && `node->func(node)` : 从队列中间取出需要执行的函数来, 这些执行函数就是销毁操作了

`rcu_gp_ongoing` is used to check whether the there is a read in critical section.
If it is, the new `rcu_gp_ctr` will not be the same as the `rcu_reader_data->ctr` and will set `rcu_reader_data->waiting` to be true.
If `registry` is empty it means all readers has leaves the critical section and this means no old reader hold the old version pointer
and the RCU thread can call the callback which insert to the RCU queue.

#### 分析一手 call_rcu
```c
void call_rcu1(struct rcu_head *node, void (*func)(struct rcu_head *node))
{
    node->func = func;
    enqueue(node);
    qatomic_inc(&rcu_call_count);
    qemu_event_set(&rcu_call_ready_event);
}
```

#### qatomic_rcu_read 和 qatomic_rcu_set
qatomic_rcu_read and qatomic_rcu_set replace `rcu_dereference` and
`rcu_assign_pointer`.  They take a _pointer_ to the variable being accessed.[^1]

`rcu_dereference()` should be used at read-side, protected by `rcu_read_lock()` or similar.

```c
address_space_set_flatview
    /* Writes are protected by the BQL.  */
    qatomic_rcu_set(&as->current_map, new_view);

void flatview_unref(FlatView *view)
    call_rcu(view, flatview_destroy, rcu);
```

- address_space_set_flatview 进行 qatomic_rcu_set 的时候被 BQL 保护，一般进行 qatomic_rcu_set 的时候会被更加细粒度的锁保护，例如在 qemu_set_log 中 QEMU_LOCK_GUARD(&qemu_logfile_mutex);

rcu_gp_ctr 只是在 synchronize_rcu 中间见到更新，从 [^2] 的描述中，应该是 call_rcu 的时候，`qatomic_inc(&rcu_call_count);` 让
call_rcu_thread 从一个 while 循环中间退出，开始执行 synchronize_rcu，call_rcu_thread 的这个 while 循环执行的比较复杂，结合注释，应该是为了多等待几个 writer

应该是这样的，qatomic_rcu_set 和 qatomic_rcu_read 其中很重要的一个事情是封装 membarrier 的工作，而 RCU 机制的作用在于，
reader 获取了指针 p 之后，之后通过 p 进行各种操作可以保证 p 指向的空间没有被释放。如果重新 qatomic_rcu_read, 那么可能获取到了新的值。
```c
    rcu_read_lock();
    p = qatomic_rcu_read(&foo);
    /* do something with p. */
    rcu_read_unlock();
```

## [^1]
In QEMU, when a lock is used, this will often be the "iothread mutex", also known as the "big QEMU lock" (BQL).

## [ ] 分析一下在当前项目中使用到的 RCU
```plain
➜  src git:(xqm) ✗ ag rcu
qemu/memory_ldst.c.inc
35:    RCU_READ_LOCK();
65:    RCU_READ_UNLOCK();
103:    RCU_READ_LOCK();
133:    RCU_READ_UNLOCK();
169:    RCU_READ_LOCK();
188:    RCU_READ_UNLOCK();
205:    RCU_READ_LOCK();
235:    RCU_READ_UNLOCK();
274:    RCU_READ_LOCK();
296:    RCU_READ_UNLOCK();
311:    RCU_READ_LOCK();
340:    RCU_READ_UNLOCK();
374:    RCU_READ_LOCK();
392:    RCU_READ_UNLOCK();
407:    RCU_READ_LOCK();
436:    RCU_READ_UNLOCK();
471:    RCU_READ_LOCK();
500:    RCU_READ_UNLOCK();
528:#undef RCU_READ_LOCK
529:#undef RCU_READ_UNLOCK

tcg/cputlb.c
764: * Called from TCG-generated code, which is under an RCU read-side

tcg/cpu-exec.c
8:#include "../../include/qemu/rcu.h"
487:  rcu_read_lock();
545:  rcu_read_unlock();

tcg/translate-all.c
525:        void **p = atomic_rcu_read(lp);
544:    pd = atomic_rcu_read(lp);
```

## [x] QTAILQ_INSERT_TAIL 和 QTAILQ_INSERT_TAIL_RCU 版本差异是什么
回答，几乎没有任何的区别啊

对比这两个，只是在写的时候是 atomic 的
```c
#define QTAILQ_INSERT_TAIL(head, elm, field) do {                       \
        (elm)->field.tqe_next = NULL;                                   \
        (elm)->field.tqe_circ.tql_prev = (head)->tqh_circ.tql_prev;     \
        (head)->tqh_circ.tql_prev->tql_next = (elm);                    \
        (head)->tqh_circ.tql_prev = &(elm)->field.tqe_circ;             \
} while (/*CONSTCOND*/0)

#define QTAILQ_INSERT_TAIL_RCU(head, elm, field) do {                   \
    (elm)->field.tqe_next = NULL;                                       \
    (elm)->field.tqe_circ.tql_prev = (head)->tqh_circ.tql_prev;         \
    qatomic_rcu_set(&(head)->tqh_circ.tql_prev->tql_next, (elm));       \
    (head)->tqh_circ.tql_prev = &(elm)->field.tqe_circ;                 \
} while (/*CONSTCOND*/0)
```

- [ ] 算了，分析一屁，以后再说了

顺便分析一下，QTAILQ 的实现方式
```c
typedef struct QTailQLink {
    void *tql_next;
    struct QTailQLink *tql_prev;
} QTailQLink;

#define QTAILQ_ENTRY(type)                                              \
union {                                                                 \
        struct type *tqe_next;        /* next element */                \
        QTailQLink tqe_circ;          /* link for circular backwards list */ \
}

struct CPUState {
    // ...
    QTAILQ_ENTRY(CPUState) node;

    // ...
```

[^1]: https://github.com/qemu/qemu/blob/master/docs/devel/rcu.txt
[^2]: https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2021/03/14/qemu-rcu
[^3]: https://stackoverflow.com/questions/39251287/rcu-dereference-vs-rcu-dereference-protected

## coroutine
在 QEMU 中 coroutine 的实现原理和其他的 coroutine 没有区别，其具体实现接口可以参考 https://www.cnblogs.com/VincentXu/p/3350389.html

Stefan Hajnoczi 说 QEMU 中需要 coroutine 是为了避免 callback hell[^2]

## QEMUBH

## Event Loop

[^1]: https://github.com/chiehmin/gdbus_test
[^2]: http://blog.vmsplice.net/2014/01/coroutines-in-qemu-basics.html
[^8]: https://stackoverflow.com/questions/21926549/get-thread-name-in-gdb
[^9]: https://stackoverflow.com/questions/8944236/gdb-how-to-get-thread-name-displayed
