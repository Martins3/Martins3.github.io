## rcu_read_lock_bh
<!-- 9c0d67bf-550b-4688-b910-0641ef6858d5 -->

参考 Documentation/RCU/whatisRCU.rst ，基本 API 为:
```txt
bh::

	Critical sections	Grace period		Barrier

	rcu_read_lock_bh	[Same as RCU]		[Same as RCU]
	rcu_read_unlock_bh
	[local_bh_disable]
	[and friends]
	rcu_dereference_bh
	rcu_dereference_bh_check
	rcu_dereference_bh_protected
	rcu_read_lock_bh_held
```



以下是我的推测为什么有 rcu-bh ，但是没有 rcu-irq 的版本:

1. 需要 rcu_read_lock_bh 保护，意味着 rcu_read_lock 的 critical region 中，就一个 grace period 就出现，这合理吗。
也就是，软中断就是会触发 grace period 的结束，
2. 所以，基于相同的思考，那就是硬中断不会推动 grace period 的结束
(这两个理解都是错误的，不是 softirq 会推动 grace period ，而是 softirq / hardirq 都是
在 rcu_read_lock 持有的时候运行的，那么这个时候，有的代码，希望可以屏蔽掉 softirq ，
动机就是这么简单。就是很多时候，有需要屏蔽 softirq ，又需要 rcu lock
)

3. 也是会受到 preemption 是否打开的情况，如果 preemption 没有打开，bh 还有意义吗？
	- rcu 本来就可能被打断，然后在打断中，进行结构体的释放
		- 问题的关键在于，释放操作是 rcu subsystem 管理的，rcu subsystem 来负责等没有人用的时候来释放
		而不是 softirq 中自己来释放。

## 例子

### vhost
```diff
commit b0c057ca7e835b36c6050c7627634b664796c1d6
Author: Michael S. Tsirkin <mst@redhat.com>
Date:   Thu Feb 13 11:45:11 2014 +0200

    vhost: fix a theoretical race in device cleanup

    vhost_zerocopy_callback accesses VQ right after it drops a ubuf
    reference.  In theory, this could race with device removal which waits
    on the ubuf kref, and crash on use after free.

    Do all accesses within rcu read side critical section, and synchronize
    on release.

    Since callbacks are always invoked from bh, synchronize_rcu_bh seems
    enough and will help release complete a bit faster.

    Signed-off-by: Michael S. Tsirkin <mst@redhat.com>
    Acked-by: Jason Wang <jasowang@redhat.com>
    Signed-off-by: David S. Miller <davem@davemloft.net>

diff --git a/drivers/vhost/net.c b/drivers/vhost/net.c
index 41be4de37e81..a0fa5de210cf 100644
--- a/drivers/vhost/net.c
+++ b/drivers/vhost/net.c
@@ -308,6 +308,8 @@ static void vhost_zerocopy_callback(struct ubuf_info *ubuf, bool success)
 	struct vhost_virtqueue *vq = ubufs->vq;
 	int cnt;

+	rcu_read_lock_bh();
+
 	/* set len to mark this desc buffers done DMA */
 	vq->heads[ubuf->desc].len = success ?
 		VHOST_DMA_DONE_LEN : VHOST_DMA_FAILED_LEN;
@@ -322,6 +324,8 @@ static void vhost_zerocopy_callback(struct ubuf_info *ubuf, bool success)
 	 */
 	if (cnt <= 1 || !(cnt % 16))
 		vhost_poll_queue(&vq->poll);
+
+	rcu_read_unlock_bh();
 }

 /* Expects to be always run from workqueue - which acts as
@@ -799,6 +803,8 @@ static int vhost_net_release(struct inode *inode, struct file *f)
 		fput(tx_sock->file);
 	if (rx_sock)
 		fput(rx_sock->file);
+	/* Make sure no callbacks are outstanding */
+	synchronize_rcu_bh();
 	/* We do an extra flush before freeing memory,
 	 * since jobs can re-queue themselves. */
 	vhost_net_flush(n);
```

### __dev_queue_xmit
最经典的使用应该是在 __dev_queue_xmit 中，
docs/trace/code/ftrace-function.sh 可以看到 `__dev_queue_xmit` 就是在软中断执行的

```c
int __dev_queue_xmit(struct sk_buff *skb, struct net_device *sb_dev)
{
	struct net_device *dev = skb->dev;
	struct netdev_queue *txq = NULL;
	struct Qdisc *q;
	int rc = -ENOMEM;
	bool again = false;

	skb_reset_mac_header(skb);
	skb_assert_len(skb);

	if (unlikely(skb_shinfo(skb)->tx_flags &
		     (SKBTX_SCHED_TSTAMP | SKBTX_BPF)))
		__skb_tstamp_tx(skb, NULL, NULL, skb->sk, SCM_TSTAMP_SCHED);

	/* Disable soft irqs for various locks below. Also
	 * stops preemption for RCU.
	 */
	rcu_read_lock_bh();
```

```txt
@[
        __dev_queue_xmit+0
        br_forward_finish+96
        br_nf_hook_thresh+280
        br_nf_forward_finish+372
        br_nf_forward_arp+300
        br_nf_forward+512
        nf_hook_slow+80
        __br_forward+160
        deliver_clone+76
        maybe_deliver+168
        br_flood+164
        br_handle_frame_finish+412
        br_handle_frame+708
        __netif_receive_skb_core.constprop.0+744
        __netif_receive_skb_list_core+280
        netif_receive_skb_list_internal+536
        napi_complete_done+136
        hns3_nic_common_poll+304
        __napi_poll+64
        net_rx_action+368
        handle_softirqs+300
        __do_softirq+28
        ____do_softirq+24
        call_on_irq_stack+36
        do_softirq_own_stack+36
        __irq_exit_rcu+316
        irq_exit_rcu+24
        el1_interrupt+72
        el1h_64_irq_handler+24
        el1h_64_irq+128
        default_idle_call+56
        cpuidle_idle_call+380
        do_idle+244
        cpu_startup_entry+60
        rest_init+196
        start_kernel+1104
        __primary_switched+136
]: 134
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

那么在进程上下文下：
softirq 仍然可能在本 CPU 上抢占执行，softirq 代码可能触发：
qdisc 切换
qdisc 的 RCU 回收推进

这会破坏你对并发关系的假设。

## 网络的用户很喜欢在中断上下文中执行?
<!-- d94ae018-006e-4cf7-9dae-94d097f10f23 -->

(2026-04-07 我认为下一步的调查在于，网络在 softirq 中到底都做了什么工作，这个可以让 codex 做，应该是比较接近了)

rg rcu_read_lock_bh 基本上全部都是网络的组件调用，如果把 bpf xdp 也算进去，
那么 kernel/padata.c 就是唯一的一个例外了。

这个问题非常经典，也就是，如果速度够快，那么可以放到 hardirq 中，如果事情多，
那么就需要放到 softirq 中的。我初步感觉，网络的 softirq 中会完成特别多的工作，
其逻辑复杂，所以需要 rcu_read_lock_bh 。

首先，存储不是不用 softirq ，而是 nvme 基本不用，而 sata 会用。
```txt
@[
        folio_wake_bit+1
        folio_end_writeback_no_dropbehind+101
        folio_end_writeback+22
        iomap_finish_ioend_buffered+303
        clone_endio+147
        blk_mq_end_request_batch+288
        nvme_irq+122
        __handle_irq_event_percpu+85
        handle_irq_event+56
        handle_edge_irq+197
        __common_interrupt+76
        common_interrupt+128
        asm_common_interrupt+38
        cpuidle_enter_state+204
        cpuidle_enter+49
        cpuidle_idle_call+245
        do_idle+119
        cpu_startup_entry+41
        start_secondary+294
        common_startup_64+318
]: 9
```

sata 的基本都是会用:
```txt
@[
        io_complete_rw+5
        blkdev_bio_end_io_async+78
        blk_update_request+415
        scsi_end_request+39
        scsi_io_completion+83
        blk_done_softirq+74
        handle_softirqs+241
        __irq_exit_rcu+194
        common_interrupt+133
        asm_common_interrupt+38
        cpuidle_enter_state+211
        cpuidle_enter+45
        cpuidle_idle_call+241
        do_idle+120
        cpu_startup_entry+41
        start_secondary+296
        common_startup_64+318
]: 301
```


## 再仔细看看文档吧

总结一下，目前知道的
1. 这个问题 chatgpt 不靠谱
2. 需要看文档
3. softirq 中显然也不会释放，释放是 rcu 的工作

🧀  rg bh
rcu.rst
57:  "rcu_read_lock_bh", "rcu_read_unlock_bh", "srcu_read_lock",

whatisRCU.rst
```txt
d.	Do you need to treat NMI handlers, hardirq handlers,
	and code segments with preemption disabled (whether
	via preempt_disable(), local_irq_save(), local_bh_disable(),
	or some other mechanism) as if they were explicit RCU readers?
	If so, RCU-sched readers are the only choice that will work
	for you, but since about v4.20 you use can use the vanilla RCU
	update primitives.

e.	Do you need RCU grace periods to complete even in the face of
	softirq monopolization of one or more of the CPUs?  For example,
	is your code subject to network-based denial-of-service attacks?
	If so, you should disable softirq across your readers, for
	example, by using rcu_read_lock_bh().  Since about v4.20 you
	use can use the vanilla RCU update primitives.
```

lockdep.rst
18:     rcu_read_lock_bh_held() for RCU-bh.
20:     rcu_read_lock_any_held() for any of normal RCU, RCU-bh, and RCU-sched.
34:     rcu_dereference_bh(p):
35:             Check for RCU-bh read-side critical section.
44:     rcu_dereference_bh_check(p, c):
46:             rcu_read_lock_bh_held().  This is useful in code that
47:             is invoked by both RCU-bh readers and updaters.

stallwarn.rst
459:   disabled, perhaps via local_bh_disable().  It is of course possible

UP.rst
126:    elsewhere using an _bh variant of the spinlock primitive.
129:    like spin_lock_bh() to acquire the lock.  Please note that

checklist.rst
67:     pointer must be covered by rcu_read_lock(), rcu_read_lock_bh(),
241:    and re-enables softirq, for example, rcu_read_lock_bh() and
242:    rcu_read_unlock_bh(), or (3) any pair of primitives that disables
274:    invocation happens entirely within a single local_bh_disable()
351:    such as rcu_read_lock_bh() and rcu_read_unlock_bh(), in which
353:    order to keep lockdep happy, in this case, rcu_dereference_bh().
375:    with softirq disabled, e.g., via spin_lock_bh().  Failing to

Design/Requirements/Requirements.rst
1285:be legal, including within preempt-disable code, local_bh_disable()
1291:protection of local_bh_disable(). In both the Linux kernel and in
2489:The RCU-bh flavor of RCU has since been expressed in terms of the other
2495:The softirq-disable (AKA “bottom-half”, hence the “_bh” abbreviations)
2496:flavor of RCU, or *RCU-bh*, was developed by Dipankar Sarma to provide a
2505:The solution was the creation of RCU-bh, which does
2506:local_bh_disable() across its read-side critical sections, and which
2509:offline. This means that RCU-bh grace periods can complete even when
2511:algorithms based on RCU-bh to withstand network-based denial-of-service
2514:Because rcu_read_lock_bh() and rcu_read_unlock_bh() disable and
2516:during the RCU-bh read-side critical section will be deferred. In this
2517:case, rcu_read_unlock_bh() will invoke softirq processing, which can
2519:overhead should be associated with the code following the RCU-bh
2520:read-side critical section rather than rcu_read_unlock_bh(), but the
2523:RCU-bh read-side critical section executes during a time of heavy
2527:rcu_read_unlock_bh(). This can of course make it appear at first
2528:glance as if rcu_read_unlock_bh() was executing very slowly.
2530:The `RCU-bh
2532:includes rcu_read_lock_bh(), rcu_read_unlock_bh(), rcu_dereference_bh(),
2533:rcu_dereference_bh_check(), and rcu_read_lock_bh_held(). However, the
2534:old RCU-bh update-side APIs are now gone, replaced by synchronize_rcu(),
2536:anything that disables bottom halves also marks an RCU-bh read-side
2537:critical section, including local_bh_disable() and local_bh_enable(),
2565:latency and overhead entailed. Just as with rcu_read_unlock_bh(),

RTFP.txt
2462:@mastersthesis{AbhinavDuggal2010Masters
2463:,author="Abhinav Duggal"
2469:   http://www.filesystems.org/docs/abhinav-thesis/abhinav_thesis.pdf
2618:,author = {Seyster, Justin and Radhakrishnan, Prabakar and Katoch, Samriti and Duggal, Abhinav and Stoller, Scott D. and Zadok, Erez}

Design/Expedited-Grace-Periods/Expedited-Grace-Periods.rst
14:third RCU-bh flavor having been implemented in terms of the other two.

Design/Data-Structures/Data-Structures.rst
1184:Turner, Abhishek Srivastava, Matt Kowalczyk, and Serge Hallyn for

## 在 softirq 无法被抢占，那么 rcu 的实现会有不同吗?

```txt
[   24.163867] 6 locks held by swapper/3/0:
[   24.163960]  #0: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: netif_receive_skb_list_internal+0xec/0x3f0
[   24.164036]  #1: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: ip_local_deliver_finish+0x60/0x1d0
[   24.164102]  #2: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: tcp_rcv_state_process+0x150/0x1128
[   24.164163]  #3: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: tcp_v4_send_synack+0xe8/0x3d0
[   24.164224]  #4: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: ip_finish_output2+0xfc/0x978
[   24.164284]  #5: ffff800081558858 (rcu_read_lock_bh){....}-{1:3}, at: __dev_queue_xmit+0x6c/0x1340
```
softirq 结束的时候 gp 才可以结束吗?


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
