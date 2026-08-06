# task rcu

function tracer 会自动的打开  CONFIG_TASKS_RUDE_RCU

```txt
config FUNCTION_TRACER
	bool "Kernel Function Tracer"
	depends on HAVE_FUNCTION_TRACER
	select KALLSYMS
	select GENERIC_TRACER
	select CONTEXT_SWITCH_TRACER
	select GLOB
	select TASKS_RCU if PREEMPTION
	select TASKS_RUDE_RCU
	help
	  Enable the kernel to trace every kernel function. This is done
	  by using a compiler feature to insert a small, 5-byte No-Operation
	  instruction at the beginning of every kernel function, which NOP
	  sequence is then dynamically patched into a tracer call when
	  tracing is enabled by the administrator. If it's runtime disabled
	  (the bootup default), then the overhead of the instructions is very
	  small and not measurable even in micro-benchmarks (at least on
	  x86, but may have impact on other architectures).
```

https://docs.kernel.org/RCU/Design/Requirements/Requirements.html#tasks-rcu

trace 机制和 preemption 通知打开的时候，tracepoint 里面的 trampolines 的内存的释放。

所以，我们观察到，这一组函数的调用者都是 call_rcu_tasks() 之类的

## 为什么 task rcu 引入一堆的 thread 来支持

这些线程可以粗分成四类:

- 普通 Tree RCU 的宽限期线程
- expedited RCU 的加急宽限期线程
- Tasks RCU 各个 flavor 的宽限期线程
- RCU 相关 workqueue 的 rescuer kworker

在一台开启 `PREEMPT_RCU`、`TASKS_RCU`、`TASKS_TRACE_RCU` 之类配置的机器上，常见线程名可以这样理解:

- `rcu_preempt`
  普通 Tree RCU 的主 grace-period kthread。它负责推进普通的 RCU 宽限期，确认旧 reader 都过去之后，相关 callback 才能安全执行。
- `rcu_exp_gp_kthread_worker`
  expedited RCU 的工作线程。有人走 `synchronize_rcu_expedited()` 这类“加急”等待路径时，它负责推进更激进的 expedited grace period。
- `rcu_exp_par_gp_kthread_worker/N`
  expedited RCU 的并行辅助线程。大机器上，为了并行做 expedited GP 的一部分扫描和等待工作，会看到多个 `/N` worker。
- `rcu_tasks_kthread`
  普通 Tasks RCU 的 grace-period 线程。它等待的是 task 级别的安全点，而不是普通 RCU 的 CPU quiescent state。
- `rcu_tasks_rude_kthread`
  Tasks Rude RCU 的 grace-period 线程。这个 flavor 更粗暴，会通过更强硬的跨 CPU 同步方式逼系统尽快过安全点，代价也更大。
- `rcu_tasks_trace_kthread`
  Tasks Trace RCU 的 grace-period 线程。它主要服务 tracing/BPF/faultable probe 这类场景，保护的是“task 是否还可能停在旧 tracing 路径里”。
- `kworker/R-rcu_gp`
  这个不是独立 flavor 的主线程，而是 `rcu_gp` workqueue 的 rescuer kworker。这里的 `R-` 表示 rescuer，通常出现在 `WQ_MEM_RECLAIM` workqueue 上，用来在内存压力或普通 worker 不方便运行时兜底执行 `rcu_gp` 队列里的工作。

可以用名字快速猜它的职责:

- 带 `exp` 的，通常和 expedited grace period 有关
- 带 `tasks` 的，通常和 Tasks RCU 有关
- 带 `trace` 的，通常和 tracing/BPF 的 task-level reader 有关
- `kworker/R-...` 通常不是某个子系统自己专门造的新主线程，而是某个 workqueue 的 rescuer

## 详细的讲解一下 rcu tasks 如用于解决 trace 问题的

可以把 **RCU Tasks** 理解成：

> **普通 RCU 等“旧 reader 消失”；RCU Tasks 等“旧 task 不可能还停留在某段旧代码里”。**

它主要不是为了保护普通数据结构，而是为了 **ftrace/BPF/kprobe 等动态修改代码、trampoline** 这种场景。
Linux 文档也是从“如何安全释放旧 trampoline”这个问题引出 Tasks RCU 的。

### 1. trampoline 引入了一个奇怪的问题

考虑 ftrace/BPF 修改函数入口：

```text
foo:
    ...
    call trampoline_A
    ...
```

现在我们想把 `trampoline_A` 替换成 `trampoline_B`：

```text
old:

    code ---> trampoline_A

new:

    code ---> trampoline_B
```

修改入口本身可能很容易。

真正麻烦的是：

> **什么时候可以 free trampoline_A？**

例如 CPU0：

```text
CPU0

trampoline_A:
    instruction 1
    instruction 2
    instruction 3
         ^
         |
     此时被抢占
```

CPU0 上的 task `T` 被抢占了。

注意它保存的 RIP 是：

```text
T->saved_rip = trampoline_A + offset
```

这时候 CPU1：

```text
CPU1:

把入口修改成 trampoline_B

free(trampoline_A);
```

那就炸了。

等 CPU0 上的 `T` 再调度回来：

```text
resume T
   |
   v
trampoline_A + offset
```

而这块代码已经被释放。

### 3. 为什么 `synchronize_rcu()` 不够？

你可能马上想到：

```c
patch_to_B();

synchronize_rcu();

free(trampoline_A);
```

问题是这里：

```text
trampoline_A
```

里面没有：

```c
rcu_read_lock();
...
rcu_read_unlock();
```

而且也不容易添加进去，因为无法保证 rcu_read_lock 和 rcu_read_unlock 是 trampoline 的第一条指令和最后一条指令

所以普通 RCU 根本不知道：

```text
task T 正停在 trampoline_A 里面
```

### 4. Tasks RCU 的核心思路

Tasks RCU 干脆换一个观察对象：

```text
普通 RCU：

        看 RCU reader
             ↓
rcu_read_lock() ----- rcu_read_unlock()


Tasks RCU：

        看整个 task
             ↓
   一个 quiescent state
             |
             | 任意 kernel execution
             |
   下一个 quiescent state
```

也就是说，Tasks RCU 可以近似认为：

> **一个 task 从上一个 Tasks-RCU quiescent state 到下一个 quiescent state 之间，都处于一个隐式的 read-side critical section。**

因此它没有普通 Tasks RCU 的：

```c
rcu_read_lock_tasks();
rcu_read_unlock_tasks();
```

这也是官方 API 表里面 RCU-Tasks 的 read-side critical section 写成 `N/A` 的原因。([Linux Kernel Archives][3])

主要 API 就是：

```c
synchronize_rcu_tasks();

call_rcu_tasks(...);

rcu_barrier_tasks();
```

### 5. Tasks RCU 的 quiescent state 计算

最重要的是 voluntary context switch

也就是 task **主动走到 scheduler**：

```text
Task T

kernel code
    |
    |
    v
schedule()
    |
    +-------- Tasks-RCU QS
```

另外，进入 userspace 也是一个很强的证据：

```text
kernel
 |
 | return to userspace
 v
userspace
```

因为如果 task 已经跑回 userspace：

> 它显然已经不可能还停留在刚才那个 kernel trampoline 里面。

Linux 因此将 voluntary scheduling、userspace execution 等作为 Tasks RCU 判断 task 已离开旧执行区间的重要依据。

### 6. 但是被抢占不算

这个非常重要。

假设：

```text
Task A:

        trampoline_old
              |
              | instruction
              X  <---- preempt here
```

然后：

```text
CPU0:

A
|
| preempt
v
B
|
C
|
D
```

虽然 CPU 已经 context switch 了很多次，但：

```text
A->RIP
```

仍然指向：

```text
trampoline_old
```

因此：

> **involuntary context switch 不能证明这个 task 离开了旧代码。**

Linux 文档对此明确强调：**involuntary context switch 不是 Tasks-RCU quiescent state**。原因正是 task 可能被抢占在 trampoline 中，恢复后还要继续执行它。

这其实就是 Tasks RCU 最值得记住的一点。

### 7. 所以 `synchronize_rcu_tasks()` 在等什么？

可以先用一个高度简化的模型理解：

调用：

```c
synchronize_rcu_tasks();
```

时，假设系统里有：

```text
T1
T2
T3
T4
```

RCU Tasks 大致认为：

```text
              synchronize_rcu_tasks()
                        |
                        v

T1 ------old execution------ QS
T2 --old execution------------- QS
T3 ------------ QS
T4 ----old execution--------------------- QS
                                           |
                                           v
                                      GP complete
```

必须确认调用开始时相关的 task 都到达了一个能够证明：

```text
我已经不可能还停留在之前那段 kernel code 中
```

的状态。

然后：

```c
synchronize_rcu_tasks();
```

才能返回。

### 8. 回到 trampoline 的例子

现在整个事情就非常漂亮了：

```c
remove_trampoline(old);

synchronize_rcu_tasks();

free(old);
```

假设某个 task：

```text
                  remove old
                      |
                      v

Task A:
    trampoline_old
         |
         X preempt
         .
         .        synchronize_rcu_tasks()
         .
         v
    resume
         |
         v
    trampoline_old remaining code
         |
         v
    normal kernel code
         |
         v
      schedule()       <-- QS
```

Tasks RCU 不会因为：

```text
X preempt
```

就放过 A。

它会一直等。

直到 A 恢复：

```text
resume
```

把 old trampoline 跑完，随后最终到达安全点：

```text
schedule()
```

于是可以推导：

```text
A 不可能再执行 old trampoline
```

此时才能安全：

```c
free(old);
```

### 9. 为什么名字叫 **Tasks** RCU？

因为 grace period 的 tracking 单位不同。

普通 Tree RCU 很大程度是在追踪：

```text
CPU
+
被抢占的显式 RCU reader
```

而 Tasks RCU 的思想是：

```text
for each relevant task:
        你有没有证明自己已经越过旧执行状态？
```

所以可以粗略记：

```text
RCU
    grace period over RCU readers

Tasks RCU
    grace period over tasks
```

这不是说：

> “Tasks RCU 用来保护 `task_struct`。”

这是非常容易产生的误解。

**不是。**

`task_struct` / task list 本身也大量使用普通 RCU：

```c
rcu_read_lock();
for_each_process(p) {
        ...
}
rcu_read_unlock();
```

这里的 `"Tasks"` 指的是 **grace-period 的观察对象是 task 的执行状态**。

### 11. 再和普通 RCU 对照一下

最推荐记住这个表：

|                | 普通 RCU                   | Tasks RCU                   |
| -------------- | ------------------------ | --------------------------- |
| 主要保护对象         | data lifetime            | **code/execution lifetime** |
| reader 边界      | `rcu_read_lock/unlock()` | **隐式**                      |
| GP 等谁          | old RCU readers          | **old tasks execution**     |
| preempt reader | 显式 reader 会被 RCU 跟踪      | **仅仅被 preempt 不代表 QS**      |
| 典型用途           | 链表、hash、对象生命周期           | **ftrace/BPF/trampoline**   |
| 等待 API         | `synchronize_rcu()`      | `synchronize_rcu_tasks()`   |

当然，“data vs code”不是 API 的严格定义，但作为**心智模型非常准确**。

### 12. 还有 Tasks Rude 和 Tasks Trace

现在内核实际上有三兄弟：

```text
                    RCU Tasks
                       |
        +--------------+--------------+
        |                             |
   Tasks Rude                    Tasks Trace
```

##### RCU Tasks

也经常称 classic Tasks RCU：

```c
synchronize_rcu_tasks();
```

特点：

```text
reader 没有显式标记
依赖 task 到达安全执行状态
```

典型：

```text
trampoline/code lifetime
```

---

##### RCU Tasks Rude

```c
synchronize_rcu_tasks_rude();
```

它解决更极端的问题：

> 连普通 RCU 不观察的 CPU 状态，例如某些 preemption-disabled / idle 相关执行区域，我也想确认它过去了。

它会比较“粗暴”地让各 CPU 执行工作，因此叫 **Rude**。

官方文档描述它会迫使各 online CPU 调度 work，因此可能打扰 `nohz_full`/实时 workload。

可以粗略理解：

```text
Tasks RCU:
    “等 task 自己证明安全”

Tasks Rude:
    “我主动去各 CPU 敲门：
     你们都给我证明一下现在安全”
```

##### RCU Tasks Trace

API：

```c
rcu_read_lock_trace();

...

rcu_read_unlock_trace();

synchronize_rcu_tasks_trace();
```

和 Classic Tasks RCU 不一样：

```text
Tasks Trace 有显式 reader
```

它是专门针对 tracing 场景设计的，能够覆盖更加复杂的 execution context。
当前内核文档列出的 API 包括 `rcu_read_lock_trace()`、`rcu_read_unlock_trace()` 和 `synchronize_rcu_tasks_trace()`。

### 13. 总结

普通 RCU：

```text
我不知道 reader 在干什么。

但只要：

        rcu_read_lock()
              |
              |
              v
        rcu_read_unlock()

全部过去，

旧对象就没人用了。
```

Tasks RCU：

```text
我甚至无法给 reader 加 lock/unlock。

所以我观察 task：

        old code
           |
           |
           v
    voluntary schedule / userspace
           |
           v

一旦所有旧 task 都跨过这种边界，

它们就不可能还停在 old code 中。

于是 old code 可以释放。
```

所以一句话概括：

> **Tasks RCU 是一种用“task 的执行进展”来定义 grace period 的 RCU，核心目的是在没有显式 read-side marker 的情况下，确认所有旧执行流已经离开某段旧代码。**

而它最关键的细节就是：

```text
task 被抢占
    ≠
Tasks-RCU quiescent state
```

因为被抢占的 task **完全可能保存着指向旧 trampoline 的 RIP**。这也是理解 Tasks RCU 的钥匙。([Linux Kernel Archives][4])

## 补充说明

### 如何区分是自动 schedule 还是 preempt 的?

都会 context switch ，但是

主动阻塞/主动调度
```txt
schedule()
  -> __schedule_loop(SM_NONE)
       -> __schedule(SM_NONE)
```

内核抢占
```txt
preempt_schedule()
  -> preempt_schedule_common()
       -> __schedule(SM_PREEMPT)
```

### 进入到用户态后，如何通知 QS

场景 1：中断发生时任务本来就在用户态

```
  userspace ── 时间片到 / 被唤醒抢占
      │ timer interrupt (interrupt lands in user mode)
      v
  irqentry_exit(): user_mode(regs) 为真
      └─ irqentry_exit_to_user_mode()
          └─ exit_to_user_mode_loop()      kernel/entry/common.c:40-47
              └─ TIF_NEED_RESCHED → schedule()   ← 普通 schedule()，SM_NONE！
```

场景 2：中断发生时任务在内核态（比如 trampoline 里）

```
  kernel (trampoline 中间) ── 被抢占
      │ timer interrupt (interrupt lands in kernel mode)
      v
  irqentry_exit(): user_mode(regs) 为假
      └─ irqentry_exit_to_kernel_mode()
          └─ raw_irqentry_exit_cond_resched()   kernel/entry/common.c:132-145
              └─ preempt_schedule_irq()  →  __schedule(SM_PREEMPT)
```

可以继续切换，然后执行其他程序，加入 A 被打断了，最后切换到用户态，就可以通知 GP

```
  synchronize_rcu_tasks() 调用者
     │ wait_for_completion(&rs->completion)        睡觉…
     │
     ├─ call_rcu_tasks(&rs->head, wakeme_after_rcu) ← 排进回调队列，唤醒 GP kthread
     │
     ▼
  GP kthread: 建 holdout 列表（快照每个任务 nvcsw）
     │
     ▼
  A 跑完内核代码，进入用户态
     │ 路径1/2/3 之一 ──► current->rcu_tasks_holdout = false   （只写一个位）
     ▼
  GP kthread 睡醒，check_holdout_task() 看到标记 ──► 摘除 A
     │
     ▼ holdout 列表空
  GP 完成 ──► rcu_tasks_invoke_cbs() ──► wakeme_after_rcu() ──► complete()
     │
     ▼
  synchronize_rcu_tasks() 返回
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
