# kernel 各个模块的锁的设计

## 为什么 scsi_host_eh_past_deadline 成功之后，需要调用 scsi_device_put

```c
			if (scsi_host_eh_past_deadline(shost)) {
				SCSI_LOG_ERROR_RECOVERY(3,
					sdev_printk(KERN_INFO, sdev,
						    "%s: skip START_UNIT, past eh deadline\n",
						    current->comm));
				scsi_device_put(sdev);
```

## irq 的 lock 设计

1. synchronize_irq

```c
/**
 *	synchronize_irq - wait for pending IRQ handlers (on other CPUs)
 *	@irq: interrupt number to wait for
 *
 *	This function waits for any pending IRQ handlers for this interrupt
 *	to complete before returning. If you use this function while
 *	holding a resource the IRQ handler may need you will deadlock.
 *
 *	Can only be called from preemptible code as it might sleep when
 *	an interrupt thread is associated to @irq.
 *
 *	It optionally makes sure (when the irq chip supports that method)
 *	that the interrupt is not pending in any CPU and waiting for
 *	service.
 */
void synchronize_irq(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (desc)
		__synchronize_irq(desc);
}
```
如果一直来中断，那这个还可以删掉吗? 还是说 spurious 中断其实根本就不在乎的
只要当前正在处理的中断可以解决就可以了。

2.
- handle_irq_event_percpu
  - note_interrupt
    - report_bad_irq
      - __report_bad_irq

有一个注释:

```c
	/*
	 * We need to take desc->lock here. note_interrupt() is called
	 * w/o desc->lock held, but IRQ_PROGRESS set. We might race
	 * with something else removing an action. It's ok to take
	 * desc->lock here. See synchronize_irq().
	 */
	raw_spin_lock_irqsave(&desc->lock, flags);
	for_each_action_of_desc(desc, action) {
		printk(KERN_ERR "[<%p>] %ps", action->handler, action->handler);
		if (action->thread_fn)
			printk(KERN_CONT " threaded [<%p>] %ps",
					action->thread_fn, action->thread_fn);
		printk(KERN_CONT "\n");
	}
	raw_spin_unlock_irqrestore(&desc->lock, flags);
```
也就是，通过 IRQ_PROGRESS 这个 flag 就可以保证 desc 不会被释放。


3. irq_domain_free_irqs 的时候，如果当时正好有中断，如何?

### 的确都是在考虑的范围之类的
```txt
@[
        __synchronize_irq+1
        irq_domain_free_irqs+144
        msi_domain_free_locked.part.0+397
        msi_domain_free_irqs_all_locked+107
        pci_free_msi_irqs+18
        pci_disable_msix+76
        pci_free_irq_vectors+19
        nvme_setup_io_queues+509
        nvme_probe.cold+353
        local_pci_probe+63
        pci_device_probe+216
        really_probe+201
        __driver_probe_device+120
        driver_probe_device+31
        __device_attach_driver+137
        bus_for_each_drv+151
        __device_attach+177
        pci_bus_add_device+88
        pci_bus_add_devices+48
        enable_slot+854
        acpiphp_check_bridge.part.0+335
        acpiphp_hotplug_notify+378
        acpi_device_hotplug+262
        acpi_hotplug_work_fn+30
        process_one_work+501
        worker_thread+444
        kthread+254
        ret_from_fork+49
        ret_from_fork_asm+26
]: 1
@[
        __synchronize_irq+1
        free_irq+297
        pci_free_irq+28
        nvme_setup_io_queues+1836
        nvme_probe.cold+353
        local_pci_probe+63
        pci_device_probe+216
        really_probe+201
        __driver_probe_device+120
        driver_probe_device+31
        __device_attach_driver+137
        bus_for_each_drv+151
        __device_attach+177
        pci_bus_add_device+88
        pci_bus_add_devices+48
        enable_slot+854
        acpiphp_check_bridge.part.0+335
        acpiphp_hotplug_notify+378
        acpi_device_hotplug+262
        acpi_hotplug_work_fn+30
        process_one_work+501
        worker_thread+444
        kthread+254
        ret_from_fork+49
        ret_from_fork_asm+26
]: 1
@[
        __synchronize_irq+1
        nvme_poll_irqdisable+69
        nvme_dev_disable+224
        nvme_remove+111
        pci_device_remove+63
        device_release_driver_internal+422
        pci_stop_bus_device+109
        pci_stop_and_remove_bus_device+18
        disable_slot+101
        acpiphp_disable_and_eject_slot+20
        acpiphp_hotplug_notify+317
        acpi_device_hotplug+262
        acpi_hotplug_work_fn+30
        process_one_work+501
        worker_thread+444
        kthread+254
        ret_from_fork+49
        ret_from_fork_asm+26
]: 1
@[
        __synchronize_irq+1
        free_irq+297
        pci_free_irq+28
        nvme_dev_disable+293
        nvme_remove+111
        pci_device_remove+63
        device_release_driver_internal+422
        pci_stop_bus_device+109
        pci_stop_and_remove_bus_device+18
        disable_slot+101
        acpiphp_disable_and_eject_slot+20
        acpiphp_hotplug_notify+317
        acpi_device_hotplug+262
        acpi_hotplug_work_fn+30
        process_one_work+501
        worker_thread+444
        kthread+254
        ret_from_fork+49
        ret_from_fork_asm+26
]: 1
@[
        __synchronize_irq+1
        free_irq+297
        pci_free_irq+28
        nvme_dev_disable+277
        nvme_remove+111
        pci_device_remove+63
        device_release_driver_internal+422
        pci_stop_bus_device+109
        pci_stop_and_remove_bus_device+18
        disable_slot+101
        acpiphp_disable_and_eject_slot+20
        acpiphp_hotplug_notify+317
        acpi_device_hotplug+262
        acpi_hotplug_work_fn+30
        process_one_work+501
        worker_thread+444
        kthread+254
        ret_from_fork+49
        ret_from_fork_asm+26
]: 64
@[
        __synchronize_irq+1
        irq_domain_free_irqs+144
        msi_domain_free_locked.part.0+397
        msi_domain_free_irqs_all_locked+107
        pci_free_msi_irqs+18
        pci_disable_msix+76
        pci_free_irq_vectors+19
        nvme_dev_disable+305
        nvme_remove+111
        pci_device_remove+63
        device_release_driver_internal+422
        pci_stop_bus_device+109
        pci_stop_and_remove_bus_device+18
        disable_slot+101
        acpiphp_disable_and_eject_slot+20
        acpiphp_hotplug_notify+317
        acpi_device_hotplug+262
        acpi_hotplug_work_fn+30
        process_one_work+501
        worker_thread+444
        kthread+254
        ret_from_fork+49
        ret_from_fork_asm+26
]: 65
```

## 热插拔

拔盘之后，系统都是咋运行的?

## virtio queue 该如何上锁?

中断中回去修改 queue ，而且提交的时候也需要修改 queue ，所以
需要锁来控制

## 网络

在 __dev_queue_xmit 的第一个 rcu_read_lock_bh 下添加 lock ，那么可以看到

```txt
[   71.272691] 4 locks held by NetworkManager/698:
[   71.272733]  #0: ffff0000c6444848 (sk_lock-AF_INET6){+.+.}-{0:0}, at: rawv6_sendmsg+0x664/0x14b0
[   71.273094]  #1: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: ip6_send_skb+0x34/0x230
[   71.273133]  #2: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: ip6_finish_output2+0x118/0xc58
[   71.273171]  #3: ffff800081558858 (rcu_read_lock_bh){....}-{1:3}, at: __dev_queue_xmit+0x6c/0x1340
[   73.327334] 4 locks held by sshd/1103:
[   73.327371]  #0: ffff0000c3240e48 (sk_lock-AF_INET){+.+.}-{0:0}, at: tcp_sendmsg+0x30/0x70
[   73.327464]  #1: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: __ip_queue_xmit+0x8/0x858
[   73.327522]  #2: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: ip_finish_output2+0xfc/0x978
[   73.327561]  #3: ffff800081558858 (rcu_read_lock_bh){....}-{1:3}, at: __dev_queue_xmit+0x6c/0x1340
[  167.304430] 4 locks held by mount.nfs/1462:
[  167.304501]  #0: ffff0000c2a90248 (sk_lock-AF_INET-RPC){+.+.}-{0:0}, at: tcp_sock_set_cork+0x28/0x78
[  167.304591]  #1: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: __ip_queue_xmit+0x8/0x858
[  167.304650]  #2: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: ip_finish_output2+0xfc/0x978
[  167.304729]  #3: ffff800081558858 (rcu_read_lock_bh){....}-{1:3}, at: __dev_queue_xmit+0x6c/0x1340
```

大致调用路径应该是这个样子的:
```txt
@[
    __dev_queue_xmit+0
    __ip_finish_output+176
    ip_finish_output+60
    ip_output+120
    __ip_queue_xmit+432
    ip_queue_xmit+28
    __tcp_transmit_skb+1184
    tcp_write_xmit+1152
    tcp_push_one+68
    tcp_sendmsg_locked+1384
    tcp_sendmsg+64
    inet_sendmsg+76
    sock_write_iter+188
    vfs_write+876
    ksys_write+244
    __arm64_sys_write+36
    invoke_syscall.constprop.0+88
    do_el0_svc+72
    el0_svc+88
    el0t_64_sync_handler+268
    el0t_64_sync+408
]: 262183
```

唯一上的锁，言简意赅，就是一个 sock 只能同时被一个 thread 使用
```c
int tcp_sendmsg(struct sock *sk, struct msghdr *msg, size_t size)
{
        int ret;

        lock_sock(sk);
        ret = tcp_sendmsg_locked(sk, msg, size);
        release_sock(sk);

        return ret;
}
```
### 有时候感觉都是在滥用 rcu lock

```txt
[   24.163867] 6 locks held by swapper/3/0:
[   24.163960]  #0: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: netif_receive_skb_list_internal+0xec/0x3f0
[   24.164036]  #1: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: ip_local_deliver_finish+0x60/0x1d0
[   24.164102]  #2: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: tcp_rcv_state_process+0x150/0x1128
[   24.164163]  #3: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: tcp_v4_send_synack+0xe8/0x3d0
[   24.164224]  #4: ffff800081558830 (rcu_read_lock){....}-{1:3}, at: ip_finish_output2+0xfc/0x978
[   24.164284]  #5: ffff800081558858 (rcu_read_lock_bh){....}-{1:3}, at: __dev_queue_xmit+0x6c/0x1340
```
真的有必要在 softirq 中嵌套这么多层的 rcu_read_lock 吗?


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
