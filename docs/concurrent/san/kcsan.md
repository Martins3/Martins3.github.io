# kcsan

KCSAN（Kernel Concurrency Sanitizer）是 Linux 内核里的动态数据竞争检测器。可以把它理解成：

> 编译器给普通内存访问插桩；运行时随机挑一些访问“停下来守株待兔”，观察其他 CPU 或线程是否同时访问了同一块内存。

它主要检测 data race，而不是所有广义上的并发逻辑错误。

## 什么算 data race

两个访问满足以下条件时构成 data race：

- 来自不同执行上下文；
- 访问的地址范围重叠；
- 至少一个是写；
- 至少一个是普通访问（plain access）；
- 两者之间缺少足够的同步或顺序保证。

例如：

```c
static int counter;

/* CPU 0 */
counter++;

/* CPU 1 */
counter++;
```

`counter++` 实际上是读、修改、写。两个 CPU 并发执行时可能丢失更新，也构成 data race。

## 基本实现方法
### 对于所有的 load / store 插桩

启用 `CONFIG_KCSAN` 后，编译器使用类似 ThreadSanitizer 的插桩方式，在普通 load/store 前插入调用。

大致可以把：

```c
value = *ptr;
```

理解为被转换成：

```c
__tsan_read4(ptr);
value = *ptr;
```

写操作类似：

```c
__tsan_write4(ptr);
*ptr = value;
```

KCSAN 在 `kernel/kcsan/core.c` 中实现了 `__tsan_readN()`、`__tsan_writeN()` 等接口，然后统一进入 `check_access()`。

内联汇编等编译器看不到的访问不会自动插桩，这也是 KCSAN 可能漏报的来源之一。

### 软件 watchpoint

KCSAN 不使用 CPU 的硬件断点，而是维护一个很小的软件 watchpoint 表。

watchpoint 表里登记“现在有人正在观察地址 X”，然后让所有经过编译器插桩的内存访问主动查询这张表。

这其实很容易理解，如果在访存过程中，当前正在写的地址，其他的变量正在访问，那么
就是说明当前的位置是被共享的访问的。

两个 CPU 都访问 `x`：

```c
/* CPU 0 */
x = 1;

/* CPU 1 */
value = x;
```

编译器插桩后，可以近似理解成：

```c
/* CPU 0 */
kcsan_check_write(&x, sizeof(x));
x = 1;

/* CPU 1 */
kcsan_check_read(&x, sizeof(x));
value = x;
```

假设 CPU 0 被 KCSAN 随机选中：

```text
CPU 0                              CPU 1

kcsan_check_write(&x)
  安装 watchpoint:
  { addr=&x, size=4, write=true }
  读取 x 的旧值
  原地延迟 80 us
                                     kcsan_check_read(&x)
                                       查询 watchpoint 表
                                       发现 &x 正被监视
                                       read 与 write 冲突
                                       消费 watchpoint
                                       保存 CPU 1 调用栈
                                     value = x
  醒来
  再次读取 x
  发现 watchpoint 已被消费
  组合 CPU 0、CPU 1 的调用栈
  输出 data-race 报告
x = 1
```

这里不要求两条机器指令在同一纳秒执行。

CPU 0 被暂停在“即将访问 `x`”的位置，CPU 1 在这个窗口内也到达了对 `x` 的访问点。因为同步机制没有阻止两边同时到达，KCSAN 就认为观察到了并发冲突。

也就是:
一次普通访问会执行 KCSAN 的 hook 大致走下面的路径：
```text
访问内存
   |
   +-- 查找是否存在重叠的 watchpoint
   |      |
   |      +-- 有，并且至少一方是写
   |             -> 消费 watchpoint
   |             -> 记录当前线程的调用栈
   |             -> 报告 data race
   |
   +-- 没有 watchpoint
          |
          +-- 大多数时候直接返回
          |
          +-- 被随机采样到
                 -> 安装 watchpoint
                 -> 读取旧值
                 -> 延迟几十微秒
                 -> 读取新值
                 -> 判断是否被其他执行上下文访问
```

这里的关键点是：KCSAN 不维护完整的 lockset 或 happens-before 图，而是通过真实执行中的“访问重叠窗口”捕获竞争。

## 普通访问和 marked access

KCSAN 区分：

- 普通 C 访问；
- `READ_ONCE()` 和 `WRITE_ONCE()`；
- `atomic_*`；
- 显式 KCSAN atomic region；
- 使用 `data_race()` 标注的有意竞争。

其中一个重要设计是：

- 普通访问可以安装 watchpoint，也会检查已有 watchpoint；
- marked/atomic 访问只检查已有 watchpoint，不主动安装 watchpoint。

因此：

```text
plain  vs plain   -> 可能报告
plain  vs marked  -> 可能报告
marked vs marked  -> 不会报告
```

这对应 LKMM 中 data race 要求“至少有一方是 plain access”的定义。

`data_race(expr)` 的含义不是“让这段代码变得线程安全”，而是：

## 内存屏障检测

普通 KCSAN 主要发现访问真正重叠的情况。

启用 `CONFIG_KCSAN_WEAK_MEMORY` 后，它还可以有限地模拟访问重排序，用来发现一部分缺少内存屏障的问题。例如：

```c
int data;
int ready;

/* CPU 0 */
data = 42;
WRITE_ONCE(ready, 1);

/* CPU 1 */
while (!READ_ONCE(ready))
	;
printk("%d\n", data);
```

正确写法通常需要 release/acquire：

```c
/* CPU 0 */
data = 42;
smp_store_release(&ready, 1);

/* CPU 1 */
while (!smp_load_acquire(&ready))
	;
printk("%d\n", data);
```

KCSAN 可以把某个 plain access 暂时保留为“待重排序访问”，在后续访问发生时继续检查，遇到相应 barrier 后再终止模拟。

但它只模拟 LKMM 的一部分，主要模拟 buffering 或 delayed access，不能完整模拟预取以及所有弱内存行为。因此不能用 KCSAN 代替 LKMM 分析或 litmus test。


## 案例分析
### folio_undo_large_rmappable() 为什么需要 data_race 标记

```c
void folio_undo_large_rmappable(struct folio *folio)
{
	struct deferred_split *ds_queue;
	unsigned long flags;

	if (folio_order(folio) <= 1)
		return;

	/*
	 * At this point, there is no one trying to add the folio to
	 * deferred_list. If folio is not in deferred_list, it's safe
	 * to check without acquiring the split_queue_lock.
	 */
	if (data_race(list_empty(&folio->_deferred_list)))
		return;

	ds_queue = get_deferred_split_queue(folio);

    /* 真正修改链表之前加锁 */
	spin_lock_irqsave(&ds_queue->split_queue_lock, flags);
	if (!list_empty(&folio->_deferred_list)) {
		ds_queue->split_queue_len--;
		list_del_init(&folio->_deferred_list);
	}
	spin_unlock_irqrestore(&ds_queue->split_queue_lock, flags);
}
```

这里是一个典型的“无锁快速检查，锁内最终确认”：

 无锁检查观察到的结果              后续行为            为什么安全
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 空                                直接返回            以后没人再添加，因此不会漏删
────────────────────────────────  ──────────────────  ──────────────────────────────────
 非空                              获取锁              在锁内重新检查并删除
────────────────────────────────  ──────────────────  ──────────────────────────────────
 检查时正被其他线程删除，看到空    返回                对方已经完成删除
────────────────────────────────  ──────────────────  ──────────────────────────────────
 检查时正被删除，但看到非空        获取锁并重新检查    锁内会看到最新状态，不会重复删除

类似，假如一个变量上有一个 flag 描述是否释放资源:

### sis->flags

```c
void __swap_writepage(struct folio *folio, struct swap_iocb **swap_plug)
{
	struct swap_info_struct *sis = __swap_entry_to_info(folio->swap);

	VM_BUG_ON_FOLIO(!folio_test_swapcache(folio), folio);
	/*
	 * ->flags can be updated non-atomically,
	 * but that will never affect SWP_FS_OPS, so the data_race
	 * is safe.
	 */
	if (data_race(sis->flags & SWP_FS_OPS))
		swap_writepage_fs(folio, swap_plug);
	/*
	 * ->flags can be updated non-atomically,
	 * but that will never affect SWP_SYNCHRONOUS_IO, so the data_race
	 * is safe.
	 */
	else if (data_race(sis->flags & SWP_SYNCHRONOUS_IO))
		swap_writepage_bdev_sync(folio, sis);
	else
		swap_writepage_bdev_async(folio, sis);
}
```

commit 7b7aca6d7c0f ("mm: ignore data-race in __swap_writepage")

KCSAN 按重叠的内存地址判断冲突，不理解“两个线程关心不同 bit”，所以会报告 data race。

do_swap_page

```c
		if (data_race(si->flags & SWP_SYNCHRONOUS_IO) &&
		    __swap_count(entry) == 1) {
```

### KCSAN 的报错太多了
只有一打开，就有很多类似的错误
想要解析这个东西并不容易

```txt
==================================================================
BUG: KCSAN: data-race in virtqueue_get_buf_ctx_split+0x63/0x220

race at unknown origin, with read to 0xffff88810cab3242 of 2 bytes by interrupt on cpu 2:
 virtqueue_get_buf_ctx_split+0x63/0x220
 virtqueue_get_buf_ctx+0x41/0x50
 virtnet_rq_get_buf+0x5c/0xa0 [virtio_net]
 virtnet_poll+0xd22/0xf40 [virtio_net]
 __napi_poll+0x5f/0x280
 net_rx_action+0x311/0x670
 handle_softirqs+0xe3/0x2a0
 irq_exit_rcu+0x9a/0xc0
 common_interrupt+0x85/0xa0
 asm_common_interrupt+0x26/0x40
 pv_native_safe_halt+0xf/0x20
 default_idle+0x13/0x20
 default_idle_call+0x29/0xf0
 do_idle+0x1cd/0x230
 cpu_startup_entry+0x29/0x30
 start_secondary+0x114/0x140
 common_startup_64+0x13e/0x148

value changed: 0x1474 -> 0x1476

Reported by Kernel Concurrency Sanitizer on:
CPU: 2 UID: 0 PID: 0 Comm: swapper/2 Not tainted 6.15.4 #14 PREEMPT(voluntary)
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.16.3-32-g9029a010ec41 04/01/2014
```

```txt
BUG: KCSAN: data-race in vring_interrupt+0xdc/0x160

race at unknown origin, with read to 0xffff888111a03242 of 2 bytes by interrupt on cpu 4:
 vring_interrupt+0xdc/0x160
 __handle_irq_event_percpu+0x7e/0x240
 handle_irq_event+0x81/0x100
 handle_edge_irq+0x136/0x430
 __common_interrupt+0x3e/0xa0
 common_interrupt+0x80/0xa0
 asm_common_interrupt+0x26/0x40
 pv_native_safe_halt+0xf/0x20
 default_idle+0x13/0x20
 default_idle_call+0x29/0xf0
 do_idle+0x1cd/0x230
 cpu_startup_entry+0x29/0x30
 start_secondary+0x114/0x140
 common_startup_64+0x13e/0x148

value changed: 0xd4e5 -> 0xd4e6

Reported by Kernel Concurrency Sanitizer on:
CPU: 4 UID: 0 PID: 0 Comm: swapper/4 Not tainted 6.15.6 #15 PREEMPT(voluntary)
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.16.3-32-g9029a010ec41 04/01/2014
```

```txt
BUG: KCSAN: data-race in _find_next_bit+0x37/0xb0

race at unknown origin, with read to 0xffffffff83242880 of 8 bytes by interrupt on cpu 0:
 _find_next_bit+0x37/0xb0
 _nohz_idle_balance.isra.0+0x113/0x380
 handle_softirqs+0xe3/0x2a0
 irq_exit_rcu+0x9a/0xc0
 sysvec_call_function_single+0x71/0x90
 asm_sysvec_call_function_single+0x1a/0x20
 pv_native_safe_halt+0xf/0x20
 default_idle+0x13/0x20
 default_idle_call+0x29/0xf0
 do_idle+0x1cd/0x230
 cpu_startup_entry+0x29/0x30
 rest_init+0x106/0x110
 start_kernel+0x936/0x940
 x86_64_start_reservations+0x24/0x30
 x86_64_start_kernel+0x8b/0x90
 common_startup_64+0x13e/0x148

value changed: 0x00000000000000f7 -> 0x00000000000000ff

Reported by Kernel Concurrency Sanitizer on:
CPU: 0 UID: 0 PID: 0 Comm: swapper/0 Not tainted 6.15.4 #14 PREEMPT(voluntary)
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.16.3-32-g9029a010ec41 04/01/2014
```

也就是:
```text
80  BUG: KCSAN: data-race in _find_next_bit+0x42/0xd0          # _nohz_idle_balance
51  BUG: KCSAN: data-race in virtqueue_get_buf_ctx_split+0x6b/0x2a0
44  BUG: KCSAN: data-race in virtqueue_kick_prepare_split+0x9e/0xf0
41  BUG: KCSAN: data-race in vring_interrupt+0x131/0x280
37  BUG: KCSAN: data-race in virtqueue_poll+0xc8/0xf0
```

源码层面也能确认：

- `virtqueue_poll_split()` 仍然 plain 读 `vq->split.vring.used->idx`,
  写方是 QEMU 设备的 DMA(未插桩),所以永远是 `race at unknown origin`
  + `value changed`。`virtqueue_get_buf_ctx_split` / `vring_interrupt` /
  `virtqueue_poll` / `virtqueue_kick_prepare_split` 报的都是同一个地址。
- `_nohz_idle_balance()` 仍然用 `for_each_cpu_wrap()` plain 读
  `nohz.idle_cpus_mask`,写方是其他 CPU 的 `cpumask_set_cpu`(marked)。
- upstream 只修过 virtio_ring 的 `event_triggered` 这个 race (commit 2e2f925fe737, 6.15 合入),和上面这些无关。

这类报告属于已知的良性 race(读到的只是性能 hint / 统计信息),
upstream 没有动力逐个加 `data_race()` 标注。

#### 修复办法
```diff
diff --git a/drivers/virtio/virtio_ring.c b/drivers/virtio/virtio_ring.c
index fbca7ce1c6bf..ac7af80b124b 100644
--- a/drivers/virtio/virtio_ring.c
+++ b/drivers/virtio/virtio_ring.c
@@ -810,10 +810,10 @@ static bool virtqueue_kick_prepare_split(struct vring_virtqueue *vq)

 	if (vq->event) {
 		needs_kick = vring_need_event(virtio16_to_cpu(vq->vq.vdev,
-					vring_avail_event(&vq->split.vring)),
+					data_race(vring_avail_event(&vq->split.vring))),
 					      new, old);
 	} else {
-		needs_kick = !(vq->split.vring.used->flags &
+		needs_kick = !(data_race(vq->split.vring.used->flags) &
 					cpu_to_virtio16(vq->vq.vdev,
 						VRING_USED_F_NO_NOTIFY));
 	}
@@ -897,8 +897,10 @@ static void detach_buf_split(struct vring_virtqueue *vq, unsigned int head,
 static bool virtqueue_poll_split(const struct vring_virtqueue *vq,
 				 unsigned int last_used_idx)
 {
+	/* The used idx is written by the device via DMA, which KCSAN cannot
+	 * instrument; tag the read as intentionally racy. */
 	return (u16)last_used_idx != virtio16_to_cpu(vq->vq.vdev,
-			vq->split.vring.used->idx);
+			data_race(vq->split.vring.used->idx));
 }

 static bool more_used_split(const struct vring_virtqueue *vq)
diff --git a/kernel/sched/fair.c b/kernel/sched/fair.c
index 3ebec186f982..6505c9b7130a 100644
--- a/kernel/sched/fair.c
+++ b/kernel/sched/fair.c
@@ -37,6 +37,7 @@
 #include <linux/sched/cputime.h>
 #include <linux/sched/isolation.h>
 #include <linux/sched/nohz.h>
+#include <linux/kcsan-checks.h>
 #include <linux/sched/prio.h>

 #include <linux/cpuidle.h>
@@ -13026,7 +13027,12 @@ static void _nohz_idle_balance(struct rq *this_rq, unsigned int flags)
 	/*
 	 * Start with the next CPU after this_cpu so we will end with this_cpu and let a
 	 * chance for other idle cpu to pull load.
+	 *
+	 * nohz.idle_cpus_mask is updated concurrently by other CPUs without
+	 * locks; the iteration below intentionally reads it locklessly, so
+	 * silence KCSAN for the whole loop (find_next_bit() does plain reads).
 	 */
+	kcsan_disable_current();
 	for_each_cpu_wrap(balance_cpu,  nohz.idle_cpus_mask, this_cpu+1) {
 		if (!idle_cpu(balance_cpu))
 			continue;
@@ -13083,6 +13089,8 @@ static void _nohz_idle_balance(struct rq *this_rq, unsigned int flags)
 			   now + msecs_to_jiffies(LOAD_AVG_PERIOD));

 abort:
+	/* Both the normal fall-through and the goto above end up here. */
+	kcsan_enable_current();
 	/* There is still blocked load, enable periodic update */
 	if (has_blocked_load)
 		WRITE_ONCE(nohz.has_blocked_load, 1);
```

如果要把 virtio_ring.c 那两处发到上游，格式上可以参考已合入的 2e2f925fe737(event_triggered 那个），逻辑完全同类；
fair.c 那个整循环关 KCSAN 的做法比较粗暴，上游大概率不会收，更可能被要求改 _find_next_bit 的读取方式。


此外，系统还有:
 KCSAN 报告：8 个签名，每个 1~2 次

我们标注的 virtio 和 _nohz_idle_balance 系列保持 0 报告。剩下的都是其它子系统的零星报告：

- maple tree ×2(mas_topiary_replace / mas_wr_store_entry vs mtree_range_walk)——写方是 munmap 分裂 VMA，读方是缺页路径 lock_vma_under_rcu 的无锁 VMA 查找。这是 maple tree 的有意设计：RCU 侧允许看到中间态，走完之后再校验，属于已知良性 race。
- folio/LRU ×3(folio_end_read vs lru_add / lru_gen_add_folio / folio_batch_move_lru)——block 层 endio 和 mm LRU 操作并发改 folio flags， 经典已知噪音。
- _find_first_zero_bit（调用方 __schedule)——和 nohz 那个同族，plain 读并发更新的 cpumask,25 分钟只出现 1 次。
- ps2、tty、filemap/mempolicy 各 1 次——都是挂了多年的已知良性报告（ps2 和 tty 那两个 syzbot 上常年开着）。

## 实现细节

### 基本内容
1. 相关源码
    - `Documentation/dev-tools/kcsan.rst`
    - `kernel/kcsan/core.c`
    - `kernel/kcsan/encoding.h`
    - `kernel/kcsan/report.c`
    - `include/linux/kcsan-checks.h`
    - `lib/Kconfig.kcsan`

2. 设置和消费 watchpoint 的主要逻辑位于 `kernel/kcsan/core.c`
中的 `kcsan_setup_watchpoint()` 和 `kcsan_found_watchpoint()`。

```text
CONFIG_KCSAN_UDELAY_TASK
CONFIG_KCSAN_UDELAY_INTERRUPT
CONFIG_KCSAN_SKIP_WATCH
```

调小 `kcsan.skip_watch`、调大 `kcsan.udelay_task` 可以提高发现率，但也会让系统更慢，并对原有时序产生更大的扰动。

一个 watchpoint 在一个 `long` 中编码：
- 内存地址；
- 访问大小；
- 是读还是写。

编码方式见 `kernel/kcsan/encoding.h`。

### watchpoint 存储内容

每个 KCSAN watchpoint 是一个编码后的 `long`，主要包含：

```c
struct conceptual_watchpoint {
	unsigned long address;
	size_t size;
	bool is_write;
};
```

实际实现没有真的使用这个结构，而是把这些字段压进一个 `long`，这样就能通过原子操作快速安装和消费：

```text
| write 标志 | size | address 的部分位 |
```

实现位于 `kernel/kcsan/encoding.h`。

因为访问可能不是单字节，所以匹配的是地址范围：

```text
watchpoint: [0x1000, 0x1007]
access:                 [0x1004, 0x100b]
```

两者范围重叠，因此匹配。

如果两边都是读，即使地址重叠也不冲突：

```text
read  + read  = 不冲突
read  + write = 冲突
write + write = 冲突
```

### “安装”和“消费”

安装 watchpoint 可以近似理解为：

```c
slot = hash(address);

if (slot 是空的)
	atomic_cmpxchg(slot, EMPTY, encoded_watchpoint);
```

消费 watchpoint 可以近似理解为：

```c
if (slot 仍然是刚才看到的 watchpoint)
	atomic_cmpxchg(slot, encoded_watchpoint, CONSUMED);
```

消费动作确保：

- 只有一个冲突访问成为报告中的“另一方”；
- 安装方能够知道有人命中了它；
- 多个 CPU 同时命中时不会破坏表项状态；
- 表项不会在报告尚未完成时立即被其他地址复用。

最后由安装方清空表项，供后续采样复用。

## TODO
KCSAN 的优点包括：

- 不需要大量 shadow memory；
- fast path 不需要共享锁；
- 能覆盖内核中的大量普通访问；
- 能发现设备或未插桩代码导致的值变化；
- 已知双方时，报告可以直接给出竞争双方的调用栈；
- 适合长时间压力测试和 fuzzing。

它的局限包括：

- 使用采样检测，理论上存在大量 false negative；
- 竞争窗口没有被采样到就不会报告；
- 未插桩访问可能只能显示 `unknown origin`；
- 只能理解一部分 LKMM 内存顺序；
- 发现不了完全由 atomic 操作组成的高层逻辑 race；
- 会明显改变系统时序和性能；
- 报告的是 data race，不一定意味着一定会立即产生用户可见故障。

KCSAN 对执行分析是“不健全的”，也就是允许漏报；但对于真正观察到的访问，设计目标是避免误报。

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
