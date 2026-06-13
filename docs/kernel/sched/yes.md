##  CLONE_THREAD 和 CLONE_VM
<!-- 56363356-341b-4a03-9e65-768b09e6cc8e -->

Man clone(2) 中的 CLONE_THREAD 可以看到:

CLONE_THREAD (since Linux 2.4.0)
```txt
    Thread groups were a feature added in Linux 2.4 to support the POSIX threads notion of a set of threads that share a single PID.  Internally, this shared PID is the so-called thread group identifier (TGID) for the thread group.  Since Linux 2.4, calls to getpid(2) return the TGID of the caller.

    The threads within a group can be distinguished by their (system-wide) unique thread IDs (TID).  A new thread's TID is available as the function result returned to the caller, and a thread can obtain its own TID using gettid(2).

    When a clone call is made without specifying CLONE_THREAD, then the resulting thread is placed in a new thread group whose TGID is the same as the thread's TID.  This thread is the leader of the new thread group.

    Since  Linux  2.5.35,  the  flags  mask  must  also  include   CLONE_SIGHAND   if CLONE_THREAD  is  specified (and note that, since Linux 2.6.0, CLONE_SIGHAND also requires CLONE_VM to be included).
```

1. CLONE_THREAD 描述是否为一个 thread group ，一个 thread group 中来处理信号以及 exec 等机制
2. CLONE_VM 是处理是否共享地址空间
3. CLONE_THREAD 会自动选择 CLONE_VM
  - "CLONE_THREAD must take CLONE_SIGHAND and CLONE_VM with it"
3. 是可以 CLONE_VM 但是不用 CLONE_THREAD
  - 这里的典型例子就是内核线程的构建: `kernel_thread`
  - 另外的测试参考 /home/martins3/data/vn/code/src/c/sched/CLONE_VM.c

没有那么复杂，也无需通过 /proc 这么间接的方法理解，看下 man clone(2)

通过 msharefs 来实现多个 process 共享地址空间，现在的系统调用实现约束太大了
需要让共享的 process 都是从同一个 parent clone 出来的:
https://mp.weixin.qq.com/s/OavFbBFanLrLiHQI3aAGow

配套分析代码: docs/kernel/sched/code/CLONE_VM.c
## SCHED_NORMAL 还是 SCHED_OTHER
<!-- 3596757f-3263-4993-8a65-5dca2615cb48 -->

总是言之，SCHED_NORMAL 就是 SCHED_OTHER
```c
/*
 * Scheduling policies
 */
#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
/* SCHED_ISO: reserved but not implemented yet */
#define SCHED_IDLE		5
#define SCHED_DEADLINE		6
#define SCHED_EXT		7
#define SCHED_MARTINS3		8
```

为什么被翻译为 SCHED_OTHER ，这是 strace 给出的结果

```txt
sched_getscheduler(13921)               = 0 (SCHED_OTHER)
```

在 glibc-2.40-36-dev/include/bits/sched.h 中定义的
```c
/* Scheduling algorithms.  */
#define SCHED_OTHER		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#ifdef __USE_GNU
# define SCHED_BATCH		3
# define SCHED_ISO		4
# define SCHED_IDLE		5
# define SCHED_DEADLINE		6

# define SCHED_RESET_ON_FORK	0x40000000
#endif
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
