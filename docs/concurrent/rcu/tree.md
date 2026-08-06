# Tree RCU

Fedora 44，kernel 7.1.3-201.fc44
32 核，2 层 rcu_node 树（根 + 2 个叶子，每叶 16 CPU）。

## 三个核心数据结构

"全局等待所有 CPU"分摊成"逐层合并"：

```
rcu_state（全局一个）
   |
   +-- rcu_node[]（树，本题 32 CPU -> 根 + 2 个叶子，每叶 16 CPU）
   |     |  qsmask:     哪些"孩子"（叶子节点或 CPU）还没过 QS
   |     |  qsmaskinit: 哪些孩子在线
   |     |  grpmask:    本节点在父节点 qsmask 里的位
   |     |  grplo/grphi:本节点覆盖的 CPU 区间
   |     +-- rcu_data（每 CPU 一个）
   |           |  gp_seq:       本 CPU 感知到的 GP 号
   |           |  cpu_no_qs:    本 CPU 是否还需要报 QS
   |           |  core_needs_qs:本 CPU 是否被当前 GP 等待
   |           |  grpmask:      本 CPU 在叶子 qsmask 里的位
   |           +-- cblist:      回调队列（RCU_NEXT_TAIL -> RCU_WAIT_TAIL -> RCU_DONE_TAIL）
```

从 trace 里可以直观看到这棵树（`rcu_quiescent_state_report` 的 level/grplo/grphi 字段）：

```
170313125 2aa5>0 1 0 15 0    <- level=1，覆盖 CPU 0-15 的叶子：qsmask 2aa5 全清
170313125 1>2   0 0 31 0     <- level=0，根（0-31）：把"叶子1"的位清掉，剩 qsmask=2
```

## 二、GP 状态机：`rcu_gp_kthread`（kernel/rcu/tree.c:2271）

```
for (;;) {
    // (1) 等"开始新 GP"的请求
    swait_event_idle_exclusive(gp_wq, gp_flags & RCU_GP_FLAG_INIT);   <- reqwait
    rcu_gp_init();                                                     <- start
    // (2) FQS 循环：等各 CPU 报 QS，必要时强制扫描
    rcu_gp_fqs_loop();                                                 <- fqswait/fqsstart/fqsend
    // (3) 收尾
    rcu_gp_cleanup();                                                  <- end
}
```

状态：`WAIT_GPS -> DONE_GPS -> INIT(rcu_gp_init) -> WAIT_FQS -> DOING_FQS -> CLEANUP -> CLEANED`，每步都有 `trace_rcu_grace_period` 对应，trace 里全能看到。

**`rcu_gp_init()`（tree.c:1804）** 干的活：取根锁、`rcu_seq_start(&rcu_state.gp_seq)` 推进 GP 号、`trace("start")`、逐层设置所有在线 CPU 的 `qsmask = qsmaskinit`（从这一刻起，每个 CPU 都必须过 QS）。

## 三、QS 上报路径：从"本 CPU"一路合并到根

每 CPU 报 QS 的路径（`cpuqs` -> `report_qs_rdp` -> `report_qs_rnp`）：

```c
// tree_plugin.h:295  rcu_qs() —— 调度/时钟路径发现本 CPU 过了 QS
if (__this_cpu_read(rcu_data.cpu_no_qs.b.norm)) {
    trace_rcu_grace_period("rcu_preempt", gp_seq, "cpuqs");   // <- trace 里的 cpuqs
    __this_cpu_write(rcu_data.cpu_no_qs.b.norm, false);
}

// rcu_report_qs_rdp()  tree.c:2443 —— 清本 CPU 在叶子上的位
mask = rdp->grpmask;
if (rnp->qsmask & mask)
    rcu_report_qs_rnp(mask, rnp, rnp->gp_seq, flags);

// rcu_report_qs_rnp()  tree.c:2339 —— 逐层向上，直到根
for (;;) {
    WRITE_ONCE(rnp->qsmask, rnp->qsmask & ~mask);
    trace_rcu_quiescent_state_report(...);      // <- trace 里的 1000>6ba7 这类
    if (rnp->qsmask != 0) return;               // 本层还有人没过，停在本地
    mask = rnp->grpmask;                        // 本层全清，去父节点
    rnp = rnp->parent;
    ...
}
// 到根且根 qsmask 全清：
rcu_report_qs_rsp(flags);   // tree.c:2315 —— 置 RCU_GP_FLAG_FQS 唤醒 GP kthread
```

**这就是树的意义**：叶子节点只在本地合并 16 个 CPU 的位，只有整层全清才向上传播一次，最后根全清才唤醒 kthread —— 避免了全局锁风暴。

## 四、FQS：等不到就主动查（tree.c:2028, 2064）

有的 CPU 在 dyntick-idle（无 tick 睡眠），永远不会主动报 QS。GP kthread 每 `jiffies_till_first_fqs` 醒来，`force_qs_rnp()` 扫描：

```c
// tree.c:834  rcu_implicit_dynticks_qs() —— 发现 CPU 处于 dyntick-idle
trace_rcu_fqs(rcu_state.name, rdp->gp_seq, rdp->cpu, "dti");  // <- trace 里的 dti
// dyntick-idle 本身就是隐式 QS，直接帮它清位
```

trace 里 `rcu_preempt-16 [015]` 打出的 `report_qs_rnp: mask=0xa8aa` 就是 FQS 一次性替一群 dyntick-idle CPU 清位。

## 五、回调加速与执行

**入队**：`call_rcu()` -> `__call_rcu_common` -> `call_rcu_core()`（tree.c:3009）-> `rcutree_enqueue`（发 `rcu_callback` tracepoint）->
若回调太多且无 GP 进行中，`rcu_accelerate_cbs()`（tree.c:1143）把回调**加速**到某个未来 GP（`AccWaitCB`/`AccReadyCB`）
-> `rcu_start_this_gp` -> `rcu_gp_kthread_wake()`。

**GP 结束后**：每 CPU 在时钟中断里 `rcu_sched_clock_irq()` 发现 `rdp->gp_seq != rnp->gp_seq` -> `__note_gp_changes()`
（tree.c:1290-1306，发 `cpuend`/`cpustart`）-> `rcu_advance_cbs()`
把完成 GP 的回调移到 DONE -> `rcu_do_batch()`（tree.c:2540）执行：

```c
for (; rhp; rhp = rcu_cblist_dequeue(&rcl)) {
    trace_rcu_invoke_callback(rcu_state.name, rhp);   // <- trace 里的 invoke
    f(rhp);
}
```

当前实现中的 `cblist` 是 segmented callback list，不同 segment 分别表示：

```text
DONE        GP 已结束，可以执行
WAIT        等待当前 GP
NEXT_READY  可以由下一个 GP 处理
NEXT        尚未确定属于哪个 GP
```

这样大量 callback 可以被同一个 GP 批量覆盖，而不需要为每个 callback 单独启动 GP。

`synchronize_rcu()` 则会把当前任务挂入等待结构，启动或加入一个 GP，然后睡眠等待完成通知。
当前实现还会批处理并发的 `synchronize_rcu()` 调用，使多个调用者共享同一个底层 GP。


## 六、实测：一次 `synchronize_rcu()` 的完整旅程

主动触发 `membarrier(MEMBARRIER_CMD_GLOBAL)` -> `synchronize_rcu()`，
用 ftrace kprobe + RCU tracepoint 全程记录。`rcu_preempt-16` 是 GP kthread。

```
时间戳        TASK        事件（-> 对应源码）
-----------------------------------------------------------------------------
.794679    mbar  cpu21  sync_rcu: synchronize_rcu+0x4/0xe0      <- (1) 进入
.794679    mbar  cpu21  rcu_callback: func=wakeme_after_rcu 11  <- (2) 自己的回调入队
                        （synchronize_rcu 就是 call_rcu(自己, wakeme_after_rcu) 然后睡）
                        （rcu_accelerate_cbs -> AccWaitCB/AccReadyCB -> 唤醒 kthread）
-----------------------------------------------------------------------------
.798579    rcu_preempt   rcu_grace_period: 170319621 end        <- (3) GP 621 结束
.798579    rcu_preempt   rcu_grace_period: 170319625 start      <- (4) GP 625 开始(rcu_gp_init)
.798580    rcu_preempt   gp_init: rcu_gp_init+0x0/0x830         <-     kprobe 佐证
.798580    rcu_preempt   fqs_loop: rcu_gp_fqs_loop+0x0/0x600    <- (5) 进 FQS 阶段
.798580    rcu_preempt   rcu_grace_period: 170319625 fqswait
.803576    rcu_preempt   fqsstart / fqsend                      <- (6) 一轮 FQS 扫描
                        （force_qs_rnp 替 dyntick-idle CPU 清位：dti 事件）
.808580    rcu_preempt   rcu_grace_period: 170319625 end        <- GP 625 结束
.808580    rcu_preempt   rcu_grace_period: 170319629 start      <- GP 629 开始
                        ** 我们的回调被分配到 629 —— 加速的 GP **
.813577    rcu_preempt   fqsstart / fqsend
.818577    rcu_preempt   fqsstart / fqsend                      <- 又两轮 FQS
.818579    rcu_preempt   rcu_grace_period: 170319629 end        <- (7) GP 629 结束！
-----------------------------------------------------------------------------
.819576    idle  cpu21   do_batch: rcu_do_batch+0x0/0x840       <- (8) CPU21 在时钟中断
.819576    idle  cpu21   rcu_batch_start: CBs=5                   (rcu_sched_clock_irq)发现
.819577    idle  cpu21   rcu_invoke_callback: func=wakeme_after_rcu  <- (9) 执行我们的回调
.819578    idle  cpu21   wake_after: wakeme_after_rcu+0x4/0x20   <-     complete(&completion)
.819584    mbar  cpu21   sync_rcu_ret: __do_sys_membarrier <- synchronize_rcu  <- (10) 返回！
```

**总耗时**：794679 -> 819584，约 **24.9ms**，即"入队 -> 等 GP 625 和 629 两个 GP -> 回调执行 -> 唤醒"。

中途还抓到了 QS 逐层合并的实例（另一轮 GP 170313125）：

```
report_qs_rnp: mask=0x1000 gps=0xa26c5a5          <- CPU12 报 QS（bit 0x1000）
quiescent_state_report: 170313125 1000>6ba7 1 0 15 0   <- 叶子(0-15)清掉 0x1000，剩 0x6ba7
report_qs_rnp: mask=0x2aa5 gps=0xa26c5a5          <- FQS 替一群 idle CPU 清位
quiescent_state_report: 170313125 2aa5>0 1 0 15 0      <- 叶子(0-15)全清 -> 向上传播
quiescent_state_report: 170313125 1>2 0 0 31 0         <- 根(0-31)清掉"叶子0"的位，剩 2
```

最后一行 `1>2` 尤其精彩：**根节点的 qsmask 变成 2（另一个叶子 16-31 还没全清），GP 不能结束** —— kthread 继续 `fqswait`，等叶子 16-31 的 CPU 陆续过 QS。这就是第二节讲的"树"在真机上工作的样子。

## 问题

1. qs 是存在版本的吗?
	- 应该是不需要的版本
	- 所有的 cpu 都进入到了 qs 中，然后可以做什么?
		- 让所有的 CPU 都通知下
	- 那么如何区分的当前的，所以是需要有一个 seq number 的
	- 版本是什么时候增加的?
		- 当完成的时候

## 基本流程
### 阶段二：`rcu_gp_init()` 初始化 GP

首先推进全局序号：

```c
rcu_seq_start(&rcu_state.gp_seq);
```

随后 breadth-first 遍历整棵 `rcu_node` 树：

```c
rcu_for_each_node_breadth_first(rnp) {
    rnp->qsmask = rnp->qsmaskinit;
    rnp->gp_seq = rcu_state.gp_seq;
}
```

此时每个 `qsmask` 都被设置为：

```text
这一层中所有需要为当前 GP 报告 QS 的 CPU/子树
```

每 CPU 的 `rcu_data` 在观察到新 GP 后，也会设置：

```c
rdp->cpu_no_qs.b.norm = true;
rdp->core_needs_qs = true;
```

表示这个 CPU 还欠一次 QS。

### 阶段三：CPU 经过 quiescent state

调度 tick 会进入：

```c
rcu_sched_clock_irq()
```

它检查 CPU 是否处于 user/idle，是否需要制造一次 reschedule，以及是否需要调度 RCU core 处理。
NO_HZ/idle CPU 则通过 dynticks/context-tracking 状态判断其是否进入过 extended quiescent state。

要注意：

```text
检测到 QS
```

和：

```text
把 QS 报告进 rcu_node tree
```

通常是两个步骤。先在本 CPU 上记录：

```c
rdp->cpu_no_qs.b.norm = false;
```

之后 RCU core 再调用：

```c
rcu_report_qs_rdp(rdp);
```


### 阶段四：从 CPU 向 leaf 报告

`rcu_report_qs_rdp()` 找到当前 CPU 的 leaf：

```c
rnp = rdp->mynode;
mask = rdp->grpmask;
```

然后清除对应 bit。

概念上相当于：

```c
leaf->qsmask &= ~cpu_bit;
```

如果 leaf 里还有其他 CPU 没完成：

```text
leaf->qsmask != 0
```

到这里就结束，不会继续访问上层节点。

### 阶段五：最后一个 CPU 向父节点传播

当 leaf 的 `qsmask` 变成零，并且没有 preempted reader 阻塞 GP 时，调用：

```c
rcu_report_qs_rnp()
```

向上走：

```c
mask = rnp->grpmask;
rnp = rnp->parent;
```

然后清除父节点中代表当前子树的 bit：

```c
parent->qsmask &= ~child_bit;
```

如果父节点也变成零，就继续向祖先传播：

```text
CPU
  ↓
leaf node
  ↓
middle node
  ↓
root node
```

当前源码中的 `rcu_report_qs_rnp()` 就是一个向上遍历循环；只有当前节点 `qsmask == 0`，并且不存在阻塞当前 GP 的 reader，才继续到 parent。

---

### 阶段六：root 清零，GP 完成

当 root 满足：

```text
root->qsmask == 0
并且没有阻塞当前 GP 的 preempted reader
```

就意味着整棵树已经完成汇总。

随后唤醒 `rcu_gp_kthread`，退出 `rcu_gp_fqs_loop()`，进入：

```c
rcu_gp_cleanup();
```

如果有 CPU 长时间不报告，`rcu_gp_fqs_loop()` 会周期性执行 force-quiescent-state：

* 检查 CPU 是否已经进入 dynticks idle；
* 检查 CPU 是否 offline；
* 请求某些 CPU reschedule；
* 最终可能触发 RCU stall warning。

---

### 阶段七：推进 callbacks

`rcu_gp_cleanup()`：

1. 标记全局 GP 结束；
2. 把新的 `gp_seq` 传播到所有 `rcu_node`；
3. 让每 CPU callback list 向前推进；
4. 唤醒 `synchronize_rcu()` 等待者；
5. 如有更多 callback，启动下一个 GP。

已经进入 `DONE` segment 的 callbacks 最终由：

```c
rcu_do_batch()
```

批量执行。普通情况下可能由 RCU softirq 或每 CPU RCU kthread 处理；配置 `RCU_NOCB_CPU` 时，也可以把 callback 执行 offload 给专门的 nocb kthread。

---

## 6. PREEMPT_RCU 为什么更复杂

这是理解 Tree RCU 最容易出错的地方。

### 抢占式 RCU

`CONFIG_PREEMPT_RCU` 中，reader 可以在读侧临界区被抢占：

```c
rcu_read_lock();

p = rcu_dereference(ptr);

/* 这里可能被抢占 */

use(p);

rcu_read_unlock();
```

这时不能因为该 CPU 调度出去了，就认为 reader 完成了。reader 是一个 task，它之后甚至可能迁移到其他 CPU 恢复执行。

因此 PREEMPT_RCU 在 `task_struct` 中保存：

```c
int rcu_read_lock_nesting;
struct list_head rcu_node_entry;
struct rcu_node *rcu_blocked_node;
union rcu_special rcu_read_unlock_special;
```

`rcu_read_lock()` 增加 task nesting；如果任务在临界区内被抢占，它会被挂到 leaf `rcu_node->blkd_tasks` 上。`gp_tasks` 指向其中第一个阻塞当前 GP 的任务。

于是 GP 的完成条件不只是：

```text
qsmask == 0
```

还必须满足：

```text
没有属于旧 reader 的 blocked task
```

任务恢复并执行最外层 `rcu_read_unlock()` 时，`rcu_read_unlock_special()` 会将它从 blocked-reader 状态中清理，并在必要时继续向树上报告。([GitHub][2])

这也是为什么 `rcu_node` 中既有位图：

```c
qsmask
```

又有任务链表：

```c
blkd_tasks
gp_tasks
```

前者跟踪 CPU，后者跟踪被抢占的 reader task。

---

## 7. `gp_seq` 是干什么的

Tree RCU 不能只用一个布尔变量表示“GP 正在运行”。

因为 CPU、node、callback 都可能稍晚才观察到状态变化，所以每层都有 sequence：

```text
rcu_state.gp_seq    全局真实进度
rcu_node.gp_seq     这个 node 已知的进度
rcu_data.gp_seq     这个 CPU 已知的进度
```

它们可能短暂不同步。

例如：

```text
global:  GP 101 已开始
leaf:    仍认为 GP 100 刚结束
CPU:     尚未运行 RCU core
```

CPU 下次进入 RCU core 时，通过比较 sequence 得知：

```text
旧 GP 已结束
新 GP 已开始
```

随后：

* 推进 callback segments；
* 设置 `cpu_no_qs`；
* 开始寻找新 GP 的 QS。

`gp_seq_needed` 则表示 callback 或等待者最远请求到了哪个未来 GP。

---

## 8. Tree RCU 不只是“数 CPU”

如果只看 `qsmask`，Tree RCU 很像一个分层 completion counter，但它还必须提供内存序保证：

```text
更新者在 GP 前删除对象
        ↓
所有旧 reader 结束
        ↓
更新者在 GP 后释放对象
```

Tree RCU 通过：

* `rcu_node->lock` 的锁序列；
* GP start/end 的 sequence 操作；
* idle/context-tracking 的有序原子操作；
* CPU report 向上穿过整棵树的锁链；

把各 CPU 上的内存访问排序连接起来。

因此 GP 结束不仅表示：

```text
大家都打过勾了
```

还表示：

```text
GP 前后的内存访问形成了 RCU 要求的全局 ordering
```

官方文档把 `rcu_node->lock` 临界区称为 Tree RCU grace-period memory ordering 的主要工作机制。([Linux内核文档][3])

```text
call_rcu() / synchronize_rcu()
              |
              v
       wake rcu_gp_kthread
              |
              v
          rcu_gp_init()
              |
              v
 CPU detects quiescent state
              |
              v
       rcu_report_qs_rdp()
              |
              v
       rcu_report_qs_rnp()
              |
              v
         root qsmask == 0
              |
              v
         rcu_gp_cleanup()
              |
              v
          rcu_do_batch()
```

## 为什么是 FQS（Force Quiescent State）

 1. 名字从哪来

 FQS = Force Quiescent State，直译"强制 quiescent state"。这个名字散落在三处代码里
 ：

 ```c
   kernel/rcu/tree.c:2792   void rcu_force_quiescent_state(void)   // 老 API，名字
 的源头
   kernel/rcu/tree.c:2028   static void rcu_gp_fqs(bool first_time) // GP 里的 FQS
 扫描
   kernel/rcu/tree.c:2015   RCU_GP_FLAG_FQS                         // 触发 FQS 的
 flag
 ```

 2. 为什么"必须强制"——因为有些 CPU 永远不主动报告

 GP 结束的前提：每个在线 CPU 都过了 QS。但有两大类 CPU 永远不会主动来报告：

 - dyntick-idle 的 CPU：深睡眠、调度 tick 都停了。它睡着了，没人叫它不会醒，永远不
   可能"自己报告 QS"。
 - 长时间待在内核里的 CPU（尤其 NO_HZ_FULL 下无 tick 运行）：不调度、不
   cond_resched()、不进用户态，同样永远不会报。

 如果 GP kthread 只会被动等报告，GP 就永远结束不了。所以必须由 kthread 主动出击 ——
 这就是 "Force" 的含义。

 3. "强制"的实质：kthread 主动判定，而不是等 CPU 上报

 FQS 是 kthread 的主动扫描，分两级手段：

 第一级（温柔）：观察 dynticks 计数器，替 CPU 认定 QS

 ```c
   // rcu_watching_snap_save()  tree.c:819 —— 第一轮 FQS：给每个欠 QS 的 CPU 拍快
 照
   rdp->watching_snap = ct_rcu_watching_cpu_acquire(rdp->cpu);
   if (rcu_watching_snap_in_eqs(rdp->watching_snap)) {
       trace_rcu_fqs(..., "dti");   // 已经在 idle 里了，直接清位
       return 1;
   }

   // rcu_watching_snap_recheck()  tree.c:855 —— 后续 FQS：对比快照
   if (rcu_watching_snap_stopped_since(rdp, rdp->watching_snap)) {
       trace_rcu_fqs(..., "dti");   // 快照之后 CPU 进入过 idle = 隐式 QS
       return 1;
   }
 ```

 rcu_watching 计数器是 context tracking 维护的：CPU 每次进出 extended quiescent
 state（idle/用户态）都会翻转。快照发生变化 = 证明该 CPU 进入过 idle = 它不可能还
 在执行 GP 开始前的 RCU 读临界区（idle 代码里没有这种临界区）。于是 kthread 替它把
 qsmask 位清掉：

 ```c
   // force_qs_rnp()  tree.c:2732 —— 只扫描"还欠着 QS"的 CPU
   for_each_leaf_node_cpu_mask(rnp, cpu, rnp->qsmask) {
       ret = f(rdp);
       if (ret > 0) mask |= rdp->grpmask;   // idle/offline：替它上报
   }
   if (mask != 0)
       rcu_report_qs_rnp(mask, rnp, rnp->gp_seq, flags);   // 一次清一批
 ```

 注意 qsmask 这个过滤 —— 已经报过的 CPU 一个都不碰，只处理"顽固分子"。这就是上一轮
 trace 里 rcu_preempt-16 [015] report_qs_rnp: mask=0x2aa5 那一行的来历：FQS 一次替
 一群 dyntick-idle CPU 认定了 QS。

 第二级（粗暴）：kick —— 对既不 idle 也不报告的 CPU

 ```c
   // rcu_watching_snap_recheck() 返回负数的场景，tree.c:918-952
   WRITE_ONCE(rdp->rcu_urgent_qs, true);        // 让它下次 tick/上下文切换时报
   WRITE_ONCE(rdp->rcu_need_heavy_qs, true);
   ...
   ret = -1;                                     // 要求强制调度
   // force_qs_rnp() 里：
   for_each_leaf_node_cpu_mask(rnp, cpu, rsmask)
       resched_cpu(cpu);                         // 直接发 IPI 打醒它
 ```

 代码注释说得很直白（tree.c:931）：

 │ So hit them over the head with the resched_cpu() hammer!

 也就是：先温柔地设标志，让 CPU 在自己的调度时钟里自己报；再不行就直接 IPI 强制它
 立刻走到调度点。

 4. "Force"还有"主动催促"的意思

 rcu_force_quiescent_state() 的调用场景（tree.c:3028，call_rcu_core 里）：

 ```c
   if (unlikely(rcu_segcblist_n_cbs(&rdp->cblist) > rdp->qlen_last_fqs_check +
 qhimark)) {
       ...
       rcu_force_quiescent_state();   // 回调积压太多，等不及了！
   }
 ```

 某个 CPU 回调积压太多、想让当前 GP 早点结束 → 置 RCU_GP_FLAG_FQS → 唤醒 kthread →
 rcu_gp_fqs_loop 里看到 flag 就不等 jiffies_till_first_fqs 到期，立刻 fqsstart 做
 一轮扫描（tree.c:2114）。所以 "force" 也包含了"主动推进 GP"的意思。

 5. 与 trace 的对照

 ```
   rcu_preempt-16  rcu_grace_period: ... fqswait      <- kthread 睡觉，等 CPU 自己
 报或定时器到点
   rcu_preempt-16  rcu_grace_period: ... fqsstart     <- 定时器到期，开始 FQS 扫描
 （force_qs_rnp）
   rcu_preempt-16  rcu_fqs: ... cpu N dti             <- 判定 CPU N 处于
 dyntick-idle，替它认定 QS
   rcu_preempt-16  report_qs_rnp: mask=0x2aa5         <- 一批 idle CPU 的位被强制
 清掉
   rcu_preempt-16  rcu_grace_period: ... fqsend       <- 本轮 FQS 结束，回到
 fqswait
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
