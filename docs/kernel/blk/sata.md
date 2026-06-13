# sata
## 文档
- https://www.kernel.org/doc/html/latest/driver-api/libata.html

## 基本测试

观察不同机器上 `ls /sys/block -la` 结果

```txt
 nvme0n1 -> ../devices/pci0000:00/0000:00:1a.0/0000:03:00.0/nvme/nvme0/nvme0n1
 nvme1n1 -> ../devices/pci0000:00/0000:00:06.0/0000:02:00.0/nvme/nvme1/nvme1n1
 sda -> ../devices/pci0000:00/0000:00:17.0/ata7/host6/target6:0:0/6:0:0:0/block/sda
 sdb -> ../devices/pci0000:00/0000:00:17.0/ata8/host7/target7:0:0/7:0:0:0/block/sdb
 sdc -> ../devices/pci0000:00/0000:00:14.0/usb1/1-7/1-7:1.0/host8/target8:0:0/8:0:0:0/block/sdc
```

```txt
sda -> ../devices/pci0000:00/0000:00:01.6/0000:04:00.0/host0/target0:2:8/0:2:8:0/block/sda
sdb -> ../devices/pci0000:00/0000:00:01.6/0000:04:00.0/host0/target0:2:9/0:2:9:0/block/sdb
sdc -> ../devices/pci0000:00/0000:00:01.6/0000:04:00.0/host0/target0:2:10/0:2:10:0/block/sdc
sdd -> ../devices/pci0000:00/0000:00:01.6/0000:04:00.0/host0/target0:2:11/0:2:11:0/block/sdd
sde -> ../devices/pci0000:00/0000:00:01.2/0000:03:00.0/ata5/host6/target6:0:0/6:0:0:0/block/sde
sr0 -> ../devices/pci0000:20/0000:20:07.1/0000:24:00.3/usb3/3-1/3-1.1/3-1.1:1.0/host1/target1:0:0/1:0:0:0/block/sr0
sr1 -> ../devices/pci0000:20/0000:20:07.1/0000:24:00.3/usb3/3-1/3-1.1/3-1.1:1.0/host1/target1:0:0/1:0:0:1/block/sr1
```

- sata 接到 sata controller 上，usb 接入到 usb controller 上，scsi 盘接入到 hba 存储上
- 一个 sata 可以提供两个设备出来。

将错误处理分为两个层次，一种是注册 `eh_strategy_handler`, 目前只有 ata_scsi_error ，
但是看上去去实现真的还是相当的复杂啊 : drivers/ata/libata-eh.c 中

> scmds enter EH via scsi_eh_scmd_add(), which does the following.
>
>  1. Links scmd->eh_entry to shost->eh_cmd_q
>
>  2. Sets SHOST_RECOVERY bit in shost->shost_state
>
>  3. Increments shost->host_failed
>
>  4. Wakes up SCSI EH thread if shost->host_busy == shost->host_failed
>
> As can be seen above, once any scmd is added to shost->eh_cmd_q,
> SHOST_RECOVERY shost_state bit is turned on.  This prevents any new
> scmd to be issued from blk queue to the host; eventually, all scmds on
> the host either complete normally, fail and get added to eh_cmd_q, or
> time out and get added to shost->eh_cmd_q.

> [!NOTE]
> 参考 Deepseeek ，有待验证

- SATA 硬盘可以通过不同的模式工作（如 IDE 兼容模式、AHCI 模式 或 RAID 模式）。
- 启用 AHCI 模式 后，SATA 控制器才能发挥 NCQ 等高级特性。
- AHCI 是一种 协议标准，规定了操作系统如何通过驱动程序与 SATA 控制器通信。

## include/linux/ata.h 中定义了关于 ATA 的命令

```c
enum {
	/* various global constants */
	// ...
	ATA_CMD_READ		= 0xC8,
	ATA_CMD_READ_EXT	= 0x25,
	ATA_CMD_READ_QUEUED	= 0x26,
	ATA_CMD_READ_STREAM_EXT	= 0x2B,
	ATA_CMD_READ_STREAM_DMA_EXT = 0x2A,
	ATA_CMD_WRITE		= 0xCA,
	ATA_CMD_WRITE_EXT	= 0x35,
	ATA_CMD_WRITE_QUEUED	= 0x36,
	ATA_CMD_WRITE_STREAM_EXT = 0x3B,
	ATA_CMD_WRITE_STREAM_DMA_EXT = 0x3A,
	ATA_CMD_WRITE_FUA_EXT	= 0x3D,
	ATA_CMD_WRITE_QUEUED_FUA_EXT = 0x3E,
	// ...
```

```txt
                 0      libata:ata_sff_flush_pio_task
                 0      libata:atapi_send_cdb
                 0      libata:atapi_pio_transfer_data
                 0      libata:ata_sff_pio_transfer_data
                 0      libata:ata_sff_port_intr
                 0      libata:ata_sff_hsm_command_complete
                 0      libata:ata_sff_hsm_state
                 0      libata:ata_port_thaw
                 0      libata:ata_port_freeze
                 0      libata:ata_std_sched_eh
                 0      libata:ata_slave_postreset
                 0      libata:ata_link_postreset
                 0      libata:ata_link_softreset_end
                 0      libata:ata_slave_hardreset_end
                 0      libata:ata_link_hardreset_end
                 0      libata:ata_link_softreset_begin
                 0      libata:ata_slave_hardreset_begin
                 0      libata:ata_link_hardreset_begin
                 0      libata:ata_eh_done
                 0      libata:ata_eh_about_to_do
                 0      libata:ata_eh_link_autopsy_qc
                 0      libata:ata_eh_link_autopsy
                 0      libata:ata_bmdma_status
                 0      libata:ata_bmdma_stop
                 0      libata:ata_bmdma_start
                 0      libata:ata_bmdma_setup
                 0      libata:ata_exec_command
                 0      libata:ata_tf_load
             3,223      libata:ata_qc_complete_done
                 0      libata:ata_qc_complete_failed
                 0      libata:ata_qc_complete_internal
             3,239      libata:ata_qc_issue
             3,239      libata:ata_qc_prep
```

```txt
@[
        ata_qc_issue+553
        __ata_scsi_queuecmd+240
        ata_scsi_queuecmd+67
        scsi_dispatch_cmd+138
        scsi_queue_rq+790
        blk_mq_dispatch_rq_list+294
        __blk_mq_do_dispatch_sched+716
        __blk_mq_sched_dispatch_requests+337
        blk_mq_sched_dispatch_requests+45
        blk_mq_run_hw_queue+622
        blk_mq_dispatch_list+373
        blk_mq_flush_plug_list+73
        __blk_flush_plug+242
        __submit_bio+458
        submit_bio_noacct_nocheck+243
        ext4_read_bh+89
        ext4_bread+92
        ext4_init_orphan_info+373
        __ext4_fill_super+3895
        ext4_fill_super+276
        get_tree_bdev_flags+323
        vfs_get_tree+41
        vfs_cmd_create+87
        __do_sys_fsconfig+1206
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 512
```

在早期 Linux（2.4 及更早）中，IDE/ATA 硬盘和 SCSI 硬盘是完全分开的（IDE 硬盘叫 `/dev/hda`，SCSI 硬盘叫 `/dev/sda`）。
但为了减少代码重复并利用 SCSI 子系统成熟的高级功能（如错误处理、热插拔、排队机制），Linux 引入了 **libata** 层。

* **SCSI 中间层 (SCSI Mid-layer):** 负责通用的存储逻辑，它只懂 SCSI 协议。
* **libata:** 它是一个“翻译官”。它向 SCSI 层注册为一个“SCSI 低层驱动 (LLD)”，但实际上它控制的是 ATA 硬件。

发生翻译的地方在: ata_scsi_queuecmd

## ATA over ethernet
<!-- 2f170112-12fe-4a22-9156-84da8bf7eae3 -->

AOE 协议最初由 Coraid 公司开发，它允许将 SATA、PATA 等 ATA
存储设备通过标准以太网进行远程访问，而无需使用更复杂的协议如 iSCSI 或光纤通道。这在某些存储区域网络 (SAN)
环境中很有用。

该模块注册了主设备号 152 (AOE_MAJOR)，并支持最多 16 个分区
(AOE_PARTITIONS)。它还包含重传机制、拥塞控制和设备发现等功能。


### 基本使用
源码位置: drivers/block/aoe/
看上去写的相当简单

- https://wpollock.com/AUnixNet/NASandSAN.htm
再比如，怎么感觉 ata 和 sas 很像，但是也不是一个东西啊:

- https://en.wikipedia.org/wiki/ATA_over_Ethernet

参考
- https://wiki.archlinux.org/title/ATA_over_Ethernet 操作指南
不过 fedora 中按照的是 sudo dnf install aoetools

server : https://github.com/OpenAoE/vblade 似乎也是相当简单的程序

```txt
# 1. 确保 aoe 驱动已加载
sudo modprobe aoe

# 2. 触发设备发现（等效于 aoe-discover）
echo 1 | sudo tee /dev/etherd/discover

# 3. 查看已发现的设备
ls /dev/etherd/                  # 列出 AoE 块设备（如 eX.Y）
aoe-stat                         # 如已安装 aoetools，显示更友好格式
```
```txt
 ls -la /dev/etherd
c-w--w---- 152,3 root  4 Feb 12:07 discover
cr--r----- 152,2 root  4 Feb 12:07 err
c-w--w---- 152,6 root  4 Feb 12:07 flush
c-w--w---- 152,4 root  4 Feb 12:07 interfaces
c-w--w---- 152,5 root  4 Feb 12:07 revalidate
```

sudo aoe-interfaces ens5
sudo aoe-discover
sudo  aoe-stat

```txt
sudo mkfs.ext4 /dev/etherd/e1.1
mkdir /mnt/e1.1
mount /dev/etherd/e1.1 /mnt/e1.1
```

initiator 端的:
```txt
- blk_mq_dispatch_list
   - 13.10% blk_mq_run_hw_queue
      - 10.65% blk_mq_sched_dispatch_requests
         - __blk_mq_sched_dispatch_requests
            - 5.44% blk_mq_dispatch_rq_list
               - aoeblk_queue_rq
                    5.21% _raw_spin_unlock_irq
            - 2.87% dd_dispatch_request
               - 0.83% _raw_spin_lock
                    0.75% lock_acquire
                 0.58% lock_is_held_type
                 0.55% __dd_dispatch_request
            - 1.90% __blk_mq_alloc_driver_tag
               - sbitmap_get
                  - sbitmap_find_bit
                       1.52% _raw_spin_unlock_irqrestore
```

target ，也就是 vblade server :
```txt
- 88.58% doaoe
   - 67.31% __GI___libc_write
      - 66.53% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 65.92% ksys_write
               - 65.84% vfs_write
                  - 65.65% sock_write_iter
                     - packet_sendmsg
                        - 53.56% __dev_queue_xmit
                           - 49.81% sch_direct_xmit
                              - 43.22% dev_hard_start_xmit
                                 - 42.83% start_xmit
                                    - 22.91% virtqueue_add_outbuf
                                       - vring_map_one_sg
                                          - 20.52% dma_map_phys
```

## AHCI 是什么
<!-- 7af3816d-e574-47be-908f-0699aa92b351 -->

**AHCI** 的全称是 **Advanced Host Controller Interface**（高级主机控制器接口）。

AHCI 是**操作系统（软件）和 SATA 硬盘（硬件）之间进行对话的一种标准协议**。

在 AHCI 出现之前，硬盘使用的是古老的 **IDE (PATA)** 模式。IDE 有很多限制，限制了 SATA 硬盘性能的发挥。AHCI 的出现解锁了 SATA 的两大核心功能：

- NCQ (Native Command Queuing，原生指令队列) —— **最核心的区别**
	* **IDE 模式**：像单线程。CPU 发一个读请求，硬盘读完，CPU 再发下一个。如果有两个请求分别在磁盘的最内圈和最外圈，磁头就要来回疯狂摆动。
	* **AHCI 模式**：像多线程/异步。CPU 一口气扔给硬盘 32 个请求，硬盘内部自己根据数据在磁盘上的物理位置进行**重新排序**，顺路把数据都读了。
- 热插拔 (Hot Plug)
	* 允许你在不关机的情况下插拔硬盘（像 USB 一样）。IDE 模式是不支持这个的。

检查了一下设备，可以发现这个 SATA 设备的底层就是 ahci 了:
```txt
🧀  lspci -k | grep -i -A 3 "sata"
00:17.0 SATA controller: Intel Corporation Alder Lake-S PCH SATA Controller [AHCI Mode] (rev 11)
        DeviceName: Onboard - SATA
        Subsystem: ASUSTeK Computer Inc. Device 8694
        Kernel driver in use: ahci
        Kernel modules: ahci
```

```txt
🧀  cat /sys/class/scsi_host/host*/proc_name
ahci
ahci
ahci
ahci
ahci
ahci
ahci
ahci
```

ahci_qc_issue 应该就是最底层了:

```c
unsigned int ahci_qc_issue(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	void __iomem *port_mmio = ahci_port_base(ap);
	struct ahci_port_priv *pp = ap->private_data;

	/* Keep track of the currently active link.  It will be used
	 * in completion path to determine whether NCQ phase is in
	 * progress.
	 */
	pp->active_link = qc->dev->link;

	if (ata_is_ncq(qc->tf.protocol))
		writel(1 << qc->hw_tag, port_mmio + PORT_SCR_ACT);

	if (pp->fbs_enabled && pp->fbs_last_dev != qc->dev->link->pmp) {
		u32 fbs = readl(port_mmio + PORT_FBS);
		fbs &= ~(PORT_FBS_DEV_MASK | PORT_FBS_DEC);
		fbs |= qc->dev->link->pmp << PORT_FBS_DEV_OFFSET;
		writel(fbs, port_mmio + PORT_FBS);
		pp->fbs_last_dev = qc->dev->link->pmp;
	}

	writel(1 << qc->hw_tag, port_mmio + PORT_CMD_ISSUE);

	ahci_sw_activity(qc->dev->link);

	return 0;
}
```

## 实验: 虚拟机中调试一下

libata-transport.c:ata_tport_add 中就是 sysfs 路径 中添加 ata7 的原因了:
```txt
lrwxrwxrwx - root 13 Feb 17:41 sda -> ../devices/pci0000:00/0000:00:17.0/ata7/host6/target6:0:0/6:0:0:0/block/sda
```

```txt
[    1.670155]  ata_tport_add.cold+0x13/0xf8 [libata]
[    1.670163]  ata_host_register+0x74/0x140 [libata]
[    1.670171]  ata_host_activate+0x10e/0x170 [libata]
[    1.670179]  ahci_init_one+0x8ef/0xcd0 [ahci]
[    1.670188]  local_pci_probe+0x48/0x90
[    1.670190]  pci_device_probe+0xdd/0x250
[    1.670192]  ? sysfs_do_create_link_sd+0x72/0xe0
[    1.670196]  really_probe+0xd4/0x3a0
[    1.670198]  ? __pfx___driver_attach+0x10/0x10
[    1.670199]  __driver_probe_device+0x7d/0x130
[    1.670201]  driver_probe_device+0x24/0xb0
[    1.670203]  __driver_attach+0xc7/0x200
[    1.670205]  bus_for_each_dev+0x95/0xf0
[    1.670209]  bus_add_driver+0x128/0x210
[    1.670211]  driver_register+0x77/0xe0
[    1.670212]  ? __pfx_ahci_pci_driver_init+0x10/0x10 [ahci]
[    1.670215]  do_one_initcall+0x76/0x2a0
[    1.670219]  do_init_module+0x68/0x240
[    1.670222]  init_module_from_file+0xdb/0x100
[    1.670229]  idempotent_init_module+0x134/0x350
[    1.670238]  __x64_sys_finit_module+0x77/0xf0
[    1.670239]  ? syscall_trace_enter+0x77/0x1f0
[    1.670242]  do_syscall_64+0x75/0xf80
[    1.670244]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

## libata vs libsas
drivers/scsi/libsas/

https://www.kernel.org/doc/html/latest/scsi/libsas.html

libsas 就是 sas 的一个通用库

## scsi 的 transport 层次
<!-- 30d3271d-5332-4b73-a200-e0ff9bb1885f -->

路径结构分解（以 `sdb` 为例）
```
../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host2/port-2:0/expander-2:0/port-2:0:0/end_device-2:0:0/target2:0:0/2:0:0:0/block/sdb
```

| 路径段 | 含义 |
|--------|------|
| `pci0000:00/0000:00:16.0/0000:0b:00.0` | PCI 拓扑路径，定位到 SAS HBA（Host Bus Adapter）或 RAID controller。`0000:0b:00.0` 是设备的 BDF（Bus:Device.Function）地址。 |
| `host2` | SCSI 子系统中的 host 编号，对应 `/sys/class/scsi_host/host2`。 |
| `expander-2:0` | SAS expander（扩展器），用于级联多个设备。格式为 `expander-<host>:<channel>`。 |
| `port-2:0:x` | Expander 上的物理端口编号（x 从 0 开始）。例如 `port-2:0:0` 是第 0 号端口。 |
| `end_device-2:0:x` | 连接到该端口的终端设备（硬盘）。 |
| `target2:0:x` | SCSI target，格式为 `target<host>:<channel>:<id>`。此处 `<id>` 与端口号一致。 |
| `2:0:x:0` | SCSI 设备地址：`<host>:<channel>:<id>:<lun>`，LUN 通常为 0。 |
| `block/sdb` | 最终映射到的块设备。 |

```txt
[$ ls -l /sys/class/sas_end_device/
total 0
lrwxrwxrwx 1 root root 0 Feb 11 17:25 end_device-1:0:0 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:0/end_device-1:0:0/sas_end_device/end_device-1:0:0
lrwxrwxrwx 1 root root 0 Feb 11 17:25 end_device-1:0:1 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:1/end_device-1:0:1/sas_end_device/end_device-1:0:1
lrwxrwxrwx 1 root root 0 Feb 11 17:25 end_device-1:0:10 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:10/end_device-1:0:10/sas_end_device/end_device-1:0:10
lrwxrwxrwx 1 root root 0 Feb 11 17:25 end_device-1:0:2 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:2/end_device-1:0:2/sas_end_device/end_device-1:0:2
lrwxrwxrwx 1 root root 0 Feb 11 17:25 end_device-1:0:3 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:3/end_device-1:0:3/sas_end_device/end_device-1:0:3
lrwxrwxrwx 1 root root 0 Feb 11 17:25 end_device-1:0:4 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:4/end_device-1:0:4/sas_end_device/end_device-1:0:4
lrwxrwxrwx 1 root root 0 Feb 11 17:25 end_device-1:0:5 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:5/end_device-1:0:5/sas_end_device/end_device-1:0:5
lrwxrwxrwx 1 root root 0 Feb 11 17:25 end_device-1:0:6 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:6/end_device-1:0:6/sas_end_device/end_device-1:0:6
lrwxrwxrwx 1 root root 0 Feb 11 17:25 end_device-1:0:7 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:7/end_device-1:0:7/sas_end_device/end_device-1:0:7
lrwxrwxrwx 1 root root 0 Feb 11 17:25 end_device-1:0:8 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:8/end_device-1:0:8/sas_end_device/end_device-1:0:8
lrwxrwxrwx 1 root root 0 Feb 11 17:25 end_device-1:0:9 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:9/end_device-1:0:9/sas_end_device/end_device-1:0:9

$ ls -l /sys/class/sas_expander/
total 0
lrwxrwxrwx 1 root root 0 Feb 11 17:25 expander-1:0 -> ../../devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/sas_expander/expander-1:0
```

```txt
PCI 总线层 (硬件总线)
    ↓
SCSI 主机层 (HBA 抽象)
    ↓
SAS Transport 层 (SAS 拓扑)
    ├─ Port（物理连接）
    ├─ Expander（拓扑扩展）
    └─ End Device（终端设备）
    ↓
SCSI 逻辑层
    ├─ Target（逻辑目标）
    └─ Device（LUN）
    ↓
块设备层 (用户访问接口)
    └─ Block Device (sdX)
```

SAS Transport Layer

| 文件 | 功能 | 关键函数 |
|-----|------|---------|
| `drivers/scsi/scsi_transport_sas.c` | SAS transport 核心（1991 行）| `sas_port_alloc()`, `sas_expander_alloc()`, `sas_end_device_alloc()`, `sas_rphy_add()` |
| `drivers/scsi/libsas/sas_discover.c` | SAS 设备发现 | `sas_discover_domain()`, `sas_discover_expander()` |
| `drivers/scsi/libsas/sas_port.c` | SAS 端口管理 | `sas_form_port()`, `sas_deform_port()` |
| `drivers/scsi/libsas/sas_phy.c` | SAS PHY 管理 | `sas_register_phys()` |
| `drivers/scsi/libsas/sas_expander.c` | SAS 扩展器管理 | `sas_ex_discover_devices()`, `sas_ex_join_wide_port()` |

**设备命名代码位置**：
- SAS Port（HBA）：`drivers/scsi/scsi_transport_sas.c:928` (`dev_set_name(&port->dev, "port-%d:%d", ...)`)
- SAS Port（Expander）：`drivers/scsi/scsi_transport_sas.c:924` (`dev_set_name(&port->dev, "port-%d:%d:%d", ...)`)
- SAS Expander：`drivers/scsi/scsi_transport_sas.c:1523` (`dev_set_name(&rdev->rphy.dev, "expander-%d:%d", ...)`)
- SAS End Device（Expander 后）：`drivers/scsi/scsi_transport_sas.c:1480` (`dev_set_name(&rdev->rphy.dev, "end_device-%d:%d:%d", ...)`)
- SAS End Device（HBA 直连）：`drivers/scsi/scsi_transport_sas.c:1485` (`dev_set_name(&rdev->rphy.dev, "end_device-%d:%d", ...)`)

哦，原来是在 drivers/scsi/scsi_transport_sas.c 就是完成这些工作，
似乎理解了，和 iscsi 的非常类似了
就是在 drivers/scsi/scsi_transport_iscsi.c 中的 iscsi_add_session 是类似的

此外，还存在一些属性:
```txt
$ pwd
/sys/devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host1/port-1:0/expander-1:0/port-1:0:10/end_device-1:0:10/target1:0:10/1:0:10:0

$ cat sas_address
0x500056b36ad37cfd
$ cat sas_device_handle
0x0014
$ cat sas_ncq_prio_enable
0
```

scsi_transport_sas.c 与 mpt3sas_transport.c 的关系:
```txt
┌─────────────────────────────────────────────────────────────────────┐
│                        用户空间 (sysfs)                             │
│    /sys/class/sas_phy/  /sys/class/sas_port/  /sys/class/sas_device/│
└─────────────────────────────────────────────────────────────────────┘
                                    ▲
                                    │ 读写属性
┌───────────────────────────────────┼─────────────────────────────────┐
│                                   │                                 │
│   ┌───────────────────────────────┴───────────────────────────────┐ │
│   │              SCSI 核心层 (scsi_transport_sas.c)               │ │
│   │    • 通用 SAS 传输类实现                                      │ │
│   │    • 定义 sas_phy/sas_port/sas_rphy 结构                      │ │
│   │    • 提供 alloc/add/delete API                                │ │
│   │    • 管理 sysfs 属性                                          │ │
│   └───────────────────────────────┬───────────────────────────────┘ │
│                                   │ 调用核心 API                    │
│                                   ▼                                 │
│   ┌───────────────────────────────────────────────────────────────┐ │
│   │           MPT3SAS 驱动层 (mpt3sas_transport.c)                │ │
│   │    • 响应硬件事件 (PHY 状态变化、设备插拔)                    │ │
│   │    • 调用 sas_port_alloc/sas_rphy_add 注册拓扑                │ │
│   │    • 维护 MPT3SAS 私有数据结构 (_sas_port, _sas_node)         │ │
│   └───────────────────────────────┬───────────────────────────────┘ │
│                                   │ 硬件操作                        │
│                                   ▼                                 │
│   ┌───────────────────────────────────────────────────────────────┐ │
│   │              MPT3SAS 硬件抽象层 (mpt3sas_base.c)              │ │
│   │    • 发送 MPT 命令到固件                                      │ │
│   │    • 处理中断和完成队列                                       │ │
│   └───────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                           ┌─────────────────┐
                           │  MPT3SAS 硬件   │
                           └─────────────────┘
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```txt
 SAS 协议的正式规范由 INCITS T10 委员会 制定，类似于 NVMe 规范由 NVM Express 组织制定。

  SAS 协议标准文档

   版本    标准编号          名称                                                  状态
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   SAS-1   INCITS 376-2003   Information Technology - Serial Attached SCSI (SAS)   已过时
   SAS-2   INCITS 478-2011   Information Technology - Serial Attached SCSI (SAS)   已过时
   SAS-3   INCITS 519-2014   Information Technology - Serial Attached SCSI (SAS)   主流
   SAS-4   INCITS 534-2019   Information Technology - Serial Attached SCSI (SAS)   最新

  相关子协议标准

   标准   全称                       描述
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   SSP    SCSI Serial Protocol       SCSI 命令传输协议（磁盘 I/O）
   SMP    SCSI Management Protocol   SAS 拓扑管理协议（发现、配置）
   STP    SATA Tunneling Protocol    SATA 设备隧道传输协议
   SPL    SCSI Parallel Interface    物理层和链路层规范
```

到此为止了，不用在这个 legacy 设备上纠结太久了。

## lsscsi 命令的含义

```txt
lsscsi

[0:0:0:0]    disk    ATA      SATADOM-SL 3IE3  25i   /dev/sda
[4:0:0:0]    disk    ATA      INTEL SSDSC2BA40 0160  /dev/sdb
[5:0:0:0]    disk    ATA      INTEL SSDSC2BA40 0160  /dev/sdc
[6:0:0:0]    disk    ATA      ST2000NX0423     NB33  /dev/sdd
[7:0:0:0]    disk    ATA      ST2000NX0423     NB33  /dev/sde
[8:0:0:0]    disk    ATA      ST2000NX0423     NB33  /dev/sdf
[9:0:0:0]    disk    ATA      ST2000NX0423     NB33  /dev/sdg
```
忽然意识到，其实还可能存在 tape 的。

```txt
🧀  lsscsi
[0:0:0:0]    disk    Linux    scsi_debug       0191  /dev/sda
[1:0:1:0]    disk    QEMU     QEMU HARDDISK    2.5+  /dev/sdb
[1:0:20:1]   cd/dvd  QEMU     QEMU CD-ROM      2.5+  /dev/sr0
```

不过，我需要说的是这个 ATA 是一个误导

即便是 megaraid 下的盘，也是 ATA 的:
```txt
lsscsi
[0:1:124:0]  enclosu BROADCOM VirtualSES       03    -
[0:2:10:0]   disk    ATA      ST16000NM001G-2K SN03  /dev/sda
[0:2:13:0]   disk    ATA      ST16000NM001G-2K SN03  /dev/sdb
[0:2:14:0]   disk    ATA      ST16000NM001G-2K SN03  /dev/sdc
[0:2:15:0]   disk    ATA      ST16000NM001G-2K SN03  /dev/sdd
[5:0:0:0]    disk    ATA      ASMT106x_ V0Safe 05    /dev/sde
[N:0:0:1]    disk    INTEL SSDPE2KE016T8__1                     /dev/nvme0n1
```

```txt
lsscsi -g  # 显示 sg 设备
[0:0:0:0]    disk    VMware   Virtual disk     2.0   /dev/sda   /dev/sg0
[1:0:0:0]    disk    ATA      MZ7KM1T9HMJP0D3  GD53  /dev/sdb   /dev/sg2
[1:0:1:0]    disk    ATA      MZ7KM1T9HMJP0D3  GD53  /dev/sdc   /dev/sg3
[1:0:2:0]    disk    SEAGATE  ST8000NM0185     PT51  /dev/sdd   /dev/sg4
[1:0:3:0]    disk    SEAGATE  ST8000NM0185     PT51  /dev/sde   /dev/sg5
[1:0:4:0]    disk    ATA      ST8000NM0205-2FF PB53  /dev/sdf   /dev/sg6
[1:0:5:0]    disk    SEAGATE  ST8000NM0185     PT51  /dev/sdg   /dev/sg7
[1:0:6:0]    disk    SEAGATE  ST8000NM0185     PT51  /dev/sdh   /dev/sg8
[1:0:7:0]    disk    ATA      ST16000NM001G    LBC5  /dev/sdi   /dev/sg9
[1:0:8:0]    disk    ATA      ST16000NM001G    LBC5  /dev/sdj   /dev/sg10
[1:0:9:0]    disk    ATA      ST16000NM001G    LBC5  /dev/sdk   /dev/sg11
[1:0:10:0]   enclosu DP       BP13G+EXP        3.31  -          /dev/sg12
[4:0:0:0]    cd/dvd  NECVMWar VMware SATA CD00 1.00  /dev/sr0   /dev/sg1
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
