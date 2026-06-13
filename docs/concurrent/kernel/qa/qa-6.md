## 为什么 backing-dev 中的这个 look up 需要 bh 的屏蔽


```c
struct backing_dev_info *bdi_get_by_id(u64 id)
{
	struct backing_dev_info *bdi = NULL;
	struct rb_node **p;

	spin_lock_bh(&bdi_lock);
	p = bdi_lookup_rb_node(id, NULL);
	if (*p) {
		bdi = rb_entry(*p, struct backing_dev_info, rb_node);
		bdi_get(bdi);
	}
	spin_unlock_bh(&bdi_lock);

	return bdi;
}
```

才发现，访问 bdi_tree 全部都是都有 bh 的保护:

```c
static struct rb_root bdi_tree = RB_ROOT;
```


这些代码真的会在 softirq 中执行吗?

我的感觉不会，但是，其实看下面的两个调用路线，都非常长，
如果我们想要改过来，也是没有办法的。

### bdi_lock 可以拆分吗?

- bdi_get_by_id 中使用了 spin_lock_bh ，但是实际上 bdi_get_by_id 被调用的可能性不低哦

有这个调用链:
- balance_dirty_pages
  - mem_cgroup_flush_foreign
    - cgroup_writeback_by_id

居然在:
```txt
show_stack+0x34/0x98 (C)
dump_stack_lvl+0x70/0x98
dump_stack+0x18/0x24
bdi_register_va+0x48/0x2b0
bdi_register+0x64/0x98
add_disk_fwnode+0x3e8/0x4f0
device_add_disk+0x1c/0x30
virtblk_probe+0x54c/0x988 [virtio_blk]
virtio_dev_probe+0x1a4/0x258
really_probe+0xb8/0x3c8
__driver_probe_device+0x84/0x160
driver_probe_device+0x44/0x130
__device_attach_driver+0xc4/0x168
bus_for_each_drv+0x90/0xf8
__device_attach+0xa8/0x1c8
device_initial_probe+0x1c/0x30
bus_probe_device+0xb4/0xc0
device_add+0x60c/0x830
register_virtio_device+0x21c/0x260
virtio_pci_probe+0xd0/0x198 [virtio_pci]
local_pci_probe+0x48/0xc0
pci_device_probe+0xd4/0x270
really_probe+0xb8/0x3c8
__driver_probe_device+0x84/0x160
driver_probe_device+0x44/0x130
__device_attach_driver+0xc4/0x168
bus_for_each_drv+0x90/0xf8
__device_attach+0xa8/0x1c8
device_attach+0x1c/0x30
pci_bus_add_device+0x90/0x148
pci_bus_add_devices+0x40/0x98
pciehp_configure_device+0xc0/0x188
pciehp_handle_presence_or_link_change+0x188/0x428
pciehp_ist+0x178/0x2c8
irq_thread_fn+0x34/0xc0
irq_thread+0x1a0/0x300
kthread+0x134/0x208
ret_from_fork+0x10/0x20
```

经典问题，如果在 softirq thread 中，
现在 in_softirq() 输出是什么？

答案是 0 ，也就是这个就是用来配置为是不是在中断的

所以，在 ARM 环境中完全没必要使用这个 bh 啊

### 附注 x86 中结果

```txt
Workqueue: kacpi_hotplug acpi_hotplug_work_fn
Call Trace:
 <TASK>
 dump_stack_lvl+0x5d/0x80
 bdi_register_va+0x35/0x270
 bdi_register+0x66/0x80
 add_disk_fwnode+0x3b7/0x480
 virtblk_probe+0x631/0xab0 [virtio_blk]
 virtio_dev_probe+0x1d4/0x2a0
 ? __pfx___device_attach_driver+0x10/0x10
 really_probe+0xc9/0x350
 ? pm_runtime_barrier+0x54/0x90
 __driver_probe_device+0x78/0x110
 driver_probe_device+0x1f/0xa0
 __device_attach_driver+0x89/0x110
 bus_for_each_drv+0x97/0xf0
 __device_attach+0xb1/0x1b0
 bus_probe_device+0x98/0xb0
 device_add+0x662/0x880
 ? lockdep_init_map_type+0x5c/0x220
 ? __pfx_vp_set_status+0x10/0x10 [virtio_pci]
 ? __pfx___device_attach_driver+0x10/0x10
 register_virtio_device+0x13f/0x160
 virtio_pci_probe+0xe2/0x180 [virtio_pci]
 local_pci_probe+0x3f/0x80
 pci_device_probe+0xd8/0x250
 ? sysfs_do_create_link_sd+0x6e/0xe0
 ? __pfx___device_attach_driver+0x10/0x10
 really_probe+0xc9/0x350
 ? pm_runtime_barrier+0x54/0x90
 __driver_probe_device+0x78/0x110
 driver_probe_device+0x1f/0xa0
 __device_attach_driver+0x89/0x110
 bus_for_each_drv+0x97/0xf0
 __device_attach+0xb1/0x1b0
 pci_bus_add_device+0x58/0x80
 pci_bus_add_devices+0x30/0x70
 enable_slot+0x356/0x490
 ? pci_device_is_present+0x54/0x80
 acpiphp_check_bridge.part.0+0x14f/0x180
 acpiphp_hotplug_notify+0x17a/0x270
 ? __pfx_acpiphp_hotplug_notify+0x10/0x10
 acpi_device_hotplug+0x106/0x440
 acpi_hotplug_work_fn+0x1e/0x30
 process_one_work+0x1f5/0x570
 worker_thread+0x1bc/0x3b0
 ? __pfx_worker_thread+0x10/0x10
 kthread+0xfe/0x220
 ? lock_release+0xc6/0x290
 ? __pfx_kthread+0x10/0x10
 ? __pfx_kthread+0x10/0x10
 ret_from_fork+0x31/0x50
 ? __pfx_kthread+0x10/0x10
 ret_from_fork_asm+0x1a/0x30
 </TASK>

--> 0 // 显然，在 workqueue 中呢
```

有时间可以看看为什么 x86 和 arm 会有不同。

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
