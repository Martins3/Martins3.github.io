# kcsan
## data_race

这里为什么需要
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
	spin_lock_irqsave(&ds_queue->split_queue_lock, flags);
	if (!list_empty(&folio->_deferred_list)) {
		ds_queue->split_queue_len--;
		list_del_init(&folio->_deferred_list);
	}
	spin_unlock_irqrestore(&ds_queue->split_queue_lock, flags);
}
```

```c
	else if (data_race(sis->flags & SWP_SYNCHRONOUS_IO))
```

```txt
History:        #0
Commit:         7b7aca6d7c0f9b2d9400bfc57cb2b23cfbd5134d
Author:         Pei Li <peili.dev@gmail.com>
Committer:      Andrew Morton <akpm@linux-foundation.org>
Author Date:    Fri 12 Jul 2024 12:32:30 AM CST
Committer Date: Thu 18 Jul 2024 12:05:18 PM CST
```

## 如何理解 do_swap_page 中的这个

```c
		if (data_race(si->flags & SWP_SYNCHRONOUS_IO) &&
		    __swap_count(entry) == 1) {
```

是不是全局变量的访问，要么添加 READ_ONCE / WRITE_ONCE ，
要么添加 date_race 才可以?

## 调试
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
