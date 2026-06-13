# 存储的热插拔
https://pve.proxmox.com/wiki/Hotplug_(qemu_disk,nic,cpu,memory)

https://serverfault.com/questions/1017754/hot-swapping-physical-disks-passed-through-to-a-qemu-vm

https://gist.github.com/devimc/e9fd533e52b08387f1df65df8b19e038

https://forum.proxmox.com/threads/how-to-remove-add-a-disk-on-a-running-kvm-virtual-machine.9273/

https://wiki.ubuntu.com/QemuDiskHotplug

## 看看 megaraid 的 hotplug 机制

当 megaraid 的上拔掉盘，那么
- megasas_remove_scsi_device

## pcie 热插拔调查一下，sata 的热插拔也测试一下
TODO 就对于 del_gendisk 打点


## virtio-blk 的热插拔通知机制是通过 acpihp 的
<!-- 08ee1004-273b-4487-9b38-ced87d2d1462 -->

我想这个结论有点草率了

```txt
[   20.504051] CPU: 2 PID: 11 Comm: kworker/u24:0 Tainted: G        W          6.6.0-15859-g89cdf9d55601-dirty #712
[   20.504442] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[   20.504687] Workqueue: kacpi_hotplug acpi_hotplug_work_fn
[   20.504897] Call Trace:
[   20.505001]  <TASK>
[   20.505083]  dump_stack_lvl+0x64/0x80
[   20.505231]  bdev_mark_dead+0x40/0xa0
[   20.505369]  blk_report_disk_dead+0x82/0xe0
[   20.505526]  del_gendisk+0x37e/0x390
[   20.505662]  virtblk_remove+0x2a/0x80
[   20.505800]  virtio_dev_remove+0x3e/0xa0
[   20.505948]  device_release_driver_internal+0x19f/0x200
[   20.506141]  bus_remove_device+0xc4/0x100
[   20.506295]  device_del+0x15c/0x4a0
[   20.506428]  device_unregister+0x12/0x30
[   20.506575]  unregister_virtio_device+0x15/0x30
[   20.506744]  virtio_pci_remove+0x3f/0x80
[   20.506894]  pci_device_remove+0x37/0xa0
[   20.507045]  device_release_driver_internal+0x19f/0x200
[   20.507240]  pci_stop_bus_device+0x6c/0x90
[   20.507392]  pci_stop_and_remove_bus_device+0x12/0x20
[   20.507578]  disable_slot+0x49/0x90
[   20.507710]  acpiphp_disable_and_eject_slot+0x14/0x80
[   20.507902]  acpiphp_hotplug_notify+0x199/0x220
[   20.508073]  ? __pfx_acpiphp_hotplug_notify+0x10/0x10
[   20.508265]  acpi_device_hotplug+0xc5/0x530
[   20.508424]  ? acpi_device_hotplug+0x9/0x530
[   20.508587]  acpi_hotplug_work_fn+0x1e/0x30
[   20.508747]  process_one_work+0x138/0x2f0
[   20.508900]  worker_thread+0x2f5/0x420
[   20.509044]  ? __pfx_worker_thread+0x10/0x10
[   20.509210]  kthread+0xe3/0x110
[   20.509331]  ? __pfx_kthread+0x10/0x10
[   20.509475]  ret_from_fork+0x31/0x50
[   20.509612]  ? __pfx_kthread+0x10/0x10
[   20.509754]  ret_from_fork_asm+0x1b/0x30
[   20.509904]  </TASK>
```
blk_report_disk_dead 中，对于所有的 partition 调用 bdev_mark_dead, 所以会出现多次刷新，以上只是部分的
代码:
```c
	xa_for_each(&disk->part_tbl, idx, bdev) {
		if (!kobject_get_unless_zero(&bdev->bd_device.kobj))
			continue;
		rcu_read_unlock();

		bdev_mark_dead(bdev, surprise);

		put_device(&bdev->bd_device);
		rcu_read_lock();
	}
```

同样在 qemu 中测试，可以发现对于 nvme 也是
```txt
@[
        device_add+5
        wakeup_source_device_create+180
        wakeup_source_sysfs_add+18
        wakeup_source_register+283
        acpi_add_pm_notifier+164
        pci_acpi_setup+165
        acpi_device_notify+270
        device_add+239
        pci_device_add+603
        pci_scan_single_device+201
        pci_scan_slot+97
        acpiphp_hotplug_notify+359
        acpi_device_hotplug+265
        acpi_hotplug_work_fn+30
        process_one_work+504
        worker_thread+462
        kthread+271
        ret_from_fork+542
        ret_from_fork_asm+26
]: 1
```

```txt
sudo bpftrace -e 'kprobe:pciehp_* { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
```
```txt
@[
        pciehp_is_native+5
        enable_slot+82
        acpiphp_check_bridge.part.0+351
        acpiphp_hotplug_notify+378
        acpi_device_hotplug+265
        acpi_hotplug_work_fn+30
        process_one_work+504
        worker_thread+462
        kthread+271
        ret_from_fork+542
        ret_from_fork_asm+26
]: 1
@[
        pciehp_is_native+5
        pci_bridge_d3_possible+115
        pci_bridge_d3_update+98
        pci_bus_add_device+63
        pci_bus_add_devices+48
        enable_slot+262
        acpiphp_check_bridge.part.0+351
        acpiphp_hotplug_notify+378
        acpi_device_hotplug+265
        acpi_hotplug_work_fn+30
        process_one_work+504
        worker_thread+462
        kthread+271
        ret_from_fork+542
        ret_from_fork_asm+26
]: 1
```
简单看了一下 pciehp_is_native 的实现，
也就是，pciehp 和 acpihp 他们之间的确有千丝万缕的联系的，似乎只是虚拟机中就是用的 acpihp 而已

> [!NOTE]
> 参考神奇海螺的意见，有待验证

使用 q35 芯片组。
预先定义 pcie-root-port 作为插槽。
使用 device_add 时，通过 bus= 参数将设备挂载到那个 Root Port 上。

```txt
qemu-system-x86_64 \
  -enable-kvm \
  -m 2G \
  -machine q35 \
  -cpu host \
  -drive file=ubuntu.qcow2,format=qcow2,if=virtio \
  \
  # 关键步骤：添加一个 PCIe Root Port
  # id: 给这个端口起个名字，后面热插拔要用
  # bus: 接在主板根总线 pcie.0 上
  # chassis/slot: 物理拓扑标识
  -device pcie-root-port,id=root_port_1,bus=pcie.0,chassis=1,slot=0 \
  \
  # 启用 QEMU 监控台，方便输入热插拔指令
  -monitor stdio
```

```txt
static inline bool hotplug_is_native(struct pci_dev *bridge)
{
	return (bridge->is_pciehp && pciehp_is_native(bridge)) ||
	       shpchp_is_native(bridge);
}
```

## 如果 mount read-only 了，然后又将所有的 disk 删除，这合理吗?
出现了错误，所有的日志没有了，通过 bisection 检查下这个错误吧

不要立刻刷，而是延迟刷

### 测试下，使用 drive_del 是否还可以使用

4.19 内核
```txt
# 对于 md 操作，是可以读写的
➜  ~ echo a > /mnt/a.md
zsh: read-only file system: /mnt/a.md
➜  ~ cat /mnt/a.md
affafdasfafafafa

# 对于非 md 操作，首先触发触发问题，之后还是可以读到内容的:
➜  ~ echo a > mnt/a
zsh: Input/output error: mnt/a
➜  ~ cat mnt/a.md
affafdasfafafafa
```

主线的内核，默认不会刷掉 cache ，但是一个写操作，之后似乎 cache 就被清理了
```txt
# 对于 md 操作，出现问题之后就无法维持生活了
➜  ~ echo a > /mnt/a.md
zsh: read-only file system: /mnt/a.md
➜  ~ cat /mnt/a.md
cat: mnt/a.md: Input/output error

# 对于非 md 操作，也是不可以
➜  ~ echo a > mnt/a
zsh: Input/output error: mnt/a
➜  ~ cat mnt/a.md
cat: mnt/a.md: Input/output error
```
在主线上，root 也会因为 drive 删除而整个文件系统无法正常使用。

几个参数:
1. 内核版本
2. md / disk

操作流程:
1. echo a > /mnt/a
2. drive_del
3. cat /mnt/a
4. echo a > /mnt/b
5. cat /mnt/a

总结: 主线内核，drive 掉了，那么文件系统只读触发，cache 被刷掉。

但是也不是完全如此，这不是一个必然 cache 刷掉的。

## 那些 inflight io 都是如何返回的？

首先在物理中，这些 mmio 写被丢失了，类似的，virtio 的 kick queue 没有效果，

大致的路径应该是这样的:

- 这些内存本来就是 guest 分配的，然后就是让所有的 io 都释放。(清空 tag_set)
- block 住所有提交的 io ，并且立刻返回错误
- 等待正在处理的中断处理函数都返回。(释放 irq)
- 各种结构体的释放，/dev/sda 之类的问题被 unlink

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
