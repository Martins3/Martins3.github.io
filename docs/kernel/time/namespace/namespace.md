# time namespace

- https://lpc.events/event/2/contributions/202/attachments/25/28/LPC2018__Time_Namespace_4.pdf
- https://lwn.net/Articles/766089/
- https://unix.stackexchange.com/questions/646318/how-are-time-namespaces-supposed-to-be-used

Linux **time namespace（时间命名空间）**可以把它理解成：

> 给容器里的进程提供一个“经过平移的系统时间轴”，主要虚拟化 **系统启动后经过了多久**，而不是虚拟化日期/北京时间这种 wall clock。

它和 PID namespace、network namespace 类似，但管理的是部分 clock。

### 1. 它解决什么问题？

最典型的是 **容器迁移 / checkpoint-restore**。

假设容器在机器 A 上运行了 1000 秒：

```text
Host A:
CLOCK_MONOTONIC = 100000 s
Container       = 1000 s
```

把容器 CRIU checkpoint，然后迁移到机器 B：

```text
Host B:
CLOCK_MONOTONIC = 500000 s
```

如果没有 time namespace，容器恢复后突然看到：

```text
1000 秒 -> 500000 秒
```

很多依赖 monotonic time 的逻辑都会出问题，例如：

```text
timeout
lease expiration
heartbeat
timer
缓存 TTL
进程 uptime
```

time namespace 可以给容器设置：

```text
offset = -499000 s
```

于是：

```text
container CLOCK_MONOTONIC
    = host CLOCK_MONOTONIC + offset
    = 500000 - 499000
    = 1000 s
```

对容器来说，时间线就连续了。

---

### 2. 它主要 namespace 哪些时间？

核心是：

* `CLOCK_MONOTONIC`（连同 `CLOCK_MONOTONIC_COARSE`、`CLOCK_MONOTONIC_RAW`）
* `CLOCK_BOOTTIME`（连同 `CLOCK_BOOTTIME_ALARM`）

也就是说主要虚拟化的是：

```text
系统运行了多久
```

而不是：

```text
CLOCK_REALTIME
2026-09-17 13:00:00
```

所以 time namespace **不是让每个容器拥有自己的日期**。

这点非常重要。

可以简单记成：

```text
time namespace ≈ virtual uptime
```

两个容易漏掉、实测确认过的点：

* `CLOCK_MONOTONIC_RAW` **也会被加上 monotonic offset**（代码里是显式的：
  `timens_setup_vdso_clock_data()` 写 `offset[CLOCK_MONOTONIC_RAW] = monotonic`，
  `posix_get_monotonic_raw()` 跟着 `timens_add_monotonic()`），
  vDSO 和 syscall 两条路径结果一致。
* 副作用是 `/proc/uptime`、`/proc/stat` 的 `btime`、`/proc/PID/stat` 第 22 列
  starttime 也跟着变，但它们是 **BOOTTIME** 语义：只改 monotonic offset 时
  这几个文件完全不变，只改 boottime offset 时全变。

例如：

```c
clock_gettime(CLOCK_MONOTONIC, ...)
clock_gettime(CLOCK_BOOTTIME, ...)
```

在不同 time namespace 中可以得到不同结果。

---

### 3. 内核怎么实现？

它并没有真的为每个 namespace 维护一个独立运行的 hardware clock。

本质非常简单：

```text
namespace_time = host_time + namespace_offset
```

内核里的 time namespace 大致维护类似：

```c
struct time_namespace {
        ...
        struct timens_offsets offsets;
};

struct timens_offsets {
        struct timespec64 monotonic;
        struct timespec64 boottime;
};
```

所以 namespace 本身主要保存：

```text
monotonic offset
boottime offset
```

比如：

```text
host CLOCK_MONOTONIC = 10000
offset               = -9000
--------------------------------
container time        = 1000
```

这比真的维护 N 套 clock 简单很多。

需要注意的是，offset 是**相对 initial time namespace（host）的绝对值**，
不是相对父 namespace 的增量：写进去的值直接覆盖 `ns->offsets.monotonic`，
范围校验也是拿 host 时间来做的。所以嵌套时可以直接造出"时间倒退"的
namespace —— L1 是 `+3600`，在 L1 里建 L2 并写 `monotonic 1000`，
L2 看到的是 host+1000，比 L1 小 2600s。

---

### 4. task 怎么知道自己属于哪个 time namespace？

和其他 namespace 类似，通过：

```c
task_struct
   |
   +-- nsproxy
          |
          +-- time_ns               <- 自己在的
          +-- time_ns_for_children  <- 孩子将进入的
```

逻辑上类似：

```c
current->nsproxy->time_ns
```

进程调用：

```c
clock_gettime(CLOCK_MONOTONIC, &ts);
```

走 syscall 时大致经历：

```text
hardware/kernel time
        ↓
ktime_get()
        ↓
host monotonic time
        ↓
找到 current 的 time namespace
        ↓
+ namespace offset
        ↓
返回给 userspace
```

即：

```text
                kernel global clock
                       |
                       v
                 ktime_get()
                       |
           +-----------+----------+
           |                      |
       namespace A             namespace B
       offset = 0              offset = -1000
           |                      |
           v                      v
        5000 sec               4000 sec
```

（更常见的路径其实是 §6 的 vDSO，压根不陷入内核。）

`unshare(CLONE_NEWTIME)` 只改 `time_ns_for_children`，调用者自己还在旧 ns，
要等 `fork()`（`timens_on_fork()`）或 `exec()`（`exec_task_namespaces()`）
之后才真正进去。所以 `/proc/self/timens_offsets` 读的其实是
`time_ns_for_children`，在这个窗口里它和 `ns/time` 不是一回事。

---

### 5. 那 timer 怎么办？

这里比 `clock_gettime()` 稍微有意思。

例如容器说：

```c
clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...);
```

要求：

```text
在 namespace time = 2000 时唤醒
```

但是内核的 timer subsystem 最终需要操作 host clock。

因此需要反向转换：

```text
namespace deadline
        ↓
减去 namespace offset
        ↓
host deadline
        ↓
hrtimer
```

也就是：

```text
namespace_time = host_time + offset
```

反过来：

```text
host_time = namespace_time - offset
```

所以不需要：

```text
每个 namespace 一套 hrtimer 时钟
```

依然可以共享内核底层的 timer infrastructure。

**只有绝对时间才换算**，相对时间（`nanosleep`、相对的 `timerfd_settime`、
`FUTEX_WAIT`）原样不动。换算入口是 `timens_ktime_to_host()`，调用它的地方有
`posix-timers.c` 的 `common_timer_set()` / `common_nsleep_timens()`、
`futex/syscalls.c` 的 `futex_init_timeout()`（非 `FUTEX_CLOCK_REALTIME`）、
`alarmtimer.c`。函数里还有一句 clamp：`tim < offset` 说明这个绝对时间在
ns 视角里早就过期了，直接当 0。

---

### 6. vDSO 怎么办？

这是 time namespace 实现里一个比较值得看的点。

正常：

```c
clock_gettime()
```

大量情况下根本不会陷入内核，而是：

```text
userspace
   ↓
vDSO
   ↓
直接读 kernel 暴露的 time data
```

如果只有 syscall 路径支持 time namespace，而 vDSO 不支持，那么：

```c
clock_gettime()
```

就可能绕过 namespace。

因此 kernel 的 **vDSO timekeeping 数据也专门支持 time namespace**：
每个 timens 单独一张 vvar 页，用户态的 `[vvar]` 布局整个换掉：

```text
普通任务    [vvar]+0  = 真 vvar        [vvar]+4K = SIGBUS
timens 任务 [vvar]+0  = timens 页      [vvar]+4K = 真 vvar
```

timens 页的特征是 `seq` 恒为奇数、`clock_mode == VDSO_CLOCKMODE_TIMENS`，
`basetime[]` 的位置实际是 `offset[]`。这样 `vdso_read_begin_timens()` 会把
代码逼进 timens 分支：先去 `[vvar]+4K` 读真时间，再加 `offset[]`。
页的互换在 `lib/vdso/datastore.c` 的 `vvar_fault()` 里做，
`timens_commit()` → `vdso_join_timens()` 会 zap 掉 `[vvar]` VMA 让下次缺页重建。

逻辑仍然是：

```text
host clock data
     +
namespace offset
```

因此 `clock_gettime()` 仍然可以保持非常快（实测 17.3ns → 17.6ns/call，
而真 syscall 是 287ns/call）。

---

### 7. 用户态如何配置？

主要接口是：

```text
/proc/<pid>/timens_offsets
```

例如可以看到类似：

```text
monotonic  0 0
boottime   0 0
```

创建 namespace 使用：

```c
CLONE_NEWTIME
```

例如：

```c
unshare(CLONE_NEWTIME);
```

不过 time namespace 有个和其他 namespace 有点不一样的设计：

```text
time_ns
time_ns_for_children
```

创建新的 time namespace 时，当前进程通常先保持在原 namespace，而：

```text
后续 child
```

进入新的 namespace。

这样父进程可以先：

```text
创建 namespace
      ↓
配置 timens_offsets
      ↓
fork/clone child
      ↓
child 使用新时间
```

这是一个很实用的设计。

几个使用限制：

* `unshare(CLONE_NEWTIME)` 需要 `CAP_SYS_ADMIN`；没有 root 时可以用
  `unshare -Ur` 先拿到 userns 的 capability。
* 写 offsets 需要 **target userns 里的 `CAP_SYS_TIME`**，而且只能在
  **第一个进程 join 这个 ns 之前**（`frozen_offsets == false`）。
  之后再写返回 `EACCES(13)`；capability 不够则是 `EPERM(1)`，两个别混。
* `setns` 进已有 timens 门槛更高：`timens_install()` 要求 target userns
  **和** 调用者自己的 userns 里都有 `CAP_SYS_ADMIN`。

---

### 8. 为什么不 namespace `CLOCK_REALTIME`？

这是理解设计目标的关键。

time namespace 主要解决：

> container 的 uptime / monotonic timeline continuity。

如果 namespace：

```text
CLOCK_REALTIME
```

会牵涉到非常多全局语义：

```text
RTC
NTP
settimeofday()
time synchronization
filesystem timestamp
network protocol timestamp
```

复杂度会高很多。

而 container migration 真正需要解决的通常是：

```text
CLOCK_MONOTONIC
CLOCK_BOOTTIME
```

因为应用程序 timeout/timer 通常应该基于 monotonic clock。

---

### 9. 一张图理解

可以把 Linux 的设计理解成：

```text
              Physical / Kernel time
                     |
                     |
               global clock
                     |
         +-----------+-----------+
         |                       |
      Host NS                Container NS
                              offset=-10000
         |                       |
         v                       v
   monotonic=12000          monotonic=2000
                                  |
                                  |
                         application sees
                              uptime=2000
```

实际上只有一套底层 clock：

```text
真实 clock + offset
```

而不是：

```text
每个 container 自己跑一块虚拟钟
```

### 最值得记住的一句话

Linux time namespace 本质上是：

> **给 `CLOCK_MONOTONIC/CLOCK_BOOTTIME` 加 namespace-specific offset，从而虚拟化容器看到的 uptime；主要用于 checkpoint/restore 和 container migration。**

如果要对照源码，直接看这几个地方：

```text
include/linux/time_namespace.h     timens_add_* / timens_ktime_to_host
kernel/time/namespace.c            clone/copy/install、proc_timens_*
kernel/time/namespace_vdso.c       vvar 页、freeze、zap VMA
lib/vdso/datastore.c               vvar_fault() 的页互换
lib/vdso/gettimeofday.c            do_hres_timens() / do_coarse_timens()
kernel/time/posix-timers.c         各 clock 的加 offset、绝对 timer 换算
kernel/futex/syscalls.c            futex 的绝对时间换算
kernel/nsproxy.c                   timens_on_fork() / exec_task_namespaces()
```

（`kernel/time/hrtimer.c`、`kernel/time/vsyscall.c` 里没有 timens 相关代码，
换算在进 hrtimer 之前就做完了。）

本目录的 `timens-*.c` 是验证上面这些结论用的实验程序。

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
