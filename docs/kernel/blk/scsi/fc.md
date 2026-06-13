# fc
## 杂七杂八东西
- http://events17.linuxfoundation.org/sites/events/files/slides/kvmforum17-npiv.pdf
	- qemu 模拟 fibre channel ，不过没进
- https://docs.kernel.org/scsi/scsi_fc_transport.html : 关键内容，介绍了 rport 和 vport

看这里可以尝试 rdma ，所以 over frabic 也是可以的吧
https://github.com/manishrma/nvme-qemu

https://www.broadcom.com/support/fibre-channel-networking/san-standards/fc-standards
应该的确是 T11 定义的

## rport 和 target 的关系
<!-- 355283aa-494a-46a7-a98c-a8a36f978ba9 -->

https://docs.google.com/document/d/1DcmFfzNOtGIpPdxJc70zTZRmjYVxJjWcEybB-jDhDIQ/edit?tab=t.0#heading=h.4bvfd7d3p63v

include/scsi/scsi_transport_fc.h
```c
/*
 * FC SCSI Target Attributes
 *
 * The SCSI Target is considered an extension of a remote port (as
 * a remote port can be more than a SCSI Target). Within the scsi
 * subsystem, we leave the Target as a separate entity. Doing so
 * provides backward compatibility with prior FC transport api's,
 * and lets remote ports be handled entirely within the FC transport
 * and independently from the scsi subsystem. The drawback is that
 * some data will be duplicated.
 */

struct fc_starget_attrs {	/* aka fc_target_attrs */
	/* Dynamic Attributes */
	u64 node_name;
	u64 port_name;
	u32 port_id;
};
```

┌────────────┬─────────────────────────────────────────┬──────────────────────────────────────┐
│    特性    │                fc_rport                 │       fc_target (scsi_target)        │
├────────────┼─────────────────────────────────────────┼──────────────────────────────────────┤
│ 层次       │ FC 传输层                               │ SCSI 子系统                          │
├────────────┼─────────────────────────────────────────┼──────────────────────────────────────┤
│ 创建时机   │ FC 链路发现远程端口时                   │ rport 扫描发现 FCP Target 时         │
├────────────┼─────────────────────────────────────────┼──────────────────────────────────────┤
│ 父子关系   │ scsi_host 的子设备                      │ fc_rport 的子设备                    │
├────────────┼─────────────────────────────────────────┼──────────────────────────────────────┤
│ 角色       │ 可以是 Target、Initiator、IP 等         │ 专门表示 SCSI Target                 │
├────────────┼─────────────────────────────────────────┼──────────────────────────────────────┤
│ 标识符     │ WWNN, WWPN, Port ID                     │ SCSI Target ID (0, 1, 2...)          │
├────────────┼─────────────────────────────────────────┼──────────────────────────────────────┤
│ Sysfs 路径 │ /sys/class/fc_remote_ports/rport-H:C:R/ │ /sys/class/fc_transport/targetH:C:T/ │
├────────────┼─────────────────────────────────────────┼──────────────────────────────────────┤
│ 生命周期   │ 由 FC 传输层管理                        │ 由 SCSI 子系统管理                   │
├────────────┼─────────────────────────────────────────┼──────────────────────────────────────┤
│ 数据重复   │ 包含完整 FC 属性                        │ 复制部分 FC 属性（WWN, Port ID）     │
└────────────┴─────────────────────────────────────────┴──────────────────────────────────────┘

- fc_rport: 代表"FC 网络上有一个端口"
- fc_target: 代表"这个端口是一个 SCSI 存储设备"

一个 fc_rport 可能：
- 不创建 fc_target（如果角色不是 FCP_TARGET）
- 创建一个 fc_target（如果角色包含 FCP_TARGET）
- 同时具有多种角色（如既是 Target 又支持 NVME）


似乎是有道理的，在 rport 下就是有 target 的
```txt
$ pwd
/sys/devices/pci0000:c9/0000:c9:04.0/0000:ca:00.0/host4/rport-4:0-7

$ ls -la
total 0
drwxr-xr-x  6 root root    0 Feb  2 18:12 .
drwxr-xr-x 31 root root    0 Feb  2 18:12 ..
drwxr-xr-x  3 root root    0 Feb  2 18:12 bsg
drwxr-xr-x  3 root root    0 Feb  2 18:12 fc_remote_ports
drwxr-xr-x  2 root root    0 Feb  3 08:34 power
drwxr-xr-x  6 root root    0 Feb  2 18:12 target4:0:0
-rw-r--r--  1 root root 4096 Feb  2 18:12 uevent
```

## syfs 和 fc 相关的目录基本观察:
- fc
	- fc_udev_device -> ../../devices/virtual/fc/fc_udev_device
- fc_vports : 空的
- fc_host
- fc_remote_ports
- fc_transport : 其实和 target 正好对应的

```txt
/sys/class/fc_host/hostN：主机 FC HBA 卡信息（对应在 /sys/class/scsi_host/ 存在相应的 hostN）
	host15  host4
	lrwxrwxrwx  1 root root 0 Feb  2 18:13 host15 -> ../../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.1/host15/fc_host/host15
	lrwxrwxrwx  1 root root 0 Feb  2 18:13 host4 -> ../../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.0/host4/fc_host/host4
	port_id：端口 id
	node_name：节点名称
	port_name：端口名称
	port_state：端口状态
	speed：端口速率
/sys/class/fc_transport/targetH:B:T/：已分配的存储链路信息
	target15:0:0  target15:0:6  target4:0:0  target4:0:6
	node_name：目标节点名称
	port_id：目标端口 id
	port_name：目标端口名称
/sys/class/fc_remote_ports/rport-H:B-R/
	rport-15:0-0   rport-15:0-14  rport-15:0-2   rport-15:0-4  rport-4:0-0   rport-4:0-14  rport-4:0-2   rport-4:0-3  rport-4:0-9
	rport-15:0-1   rport-15:0-15  rport-15:0-20  rport-15:0-5  rport-4:0-1   rport-4:0-15  rport-4:0-20  rport-4:0-4
	rport-15:0-10  rport-15:0-16  rport-15:0-21  rport-15:0-6  rport-4:0-10  rport-4:0-16  rport-4:0-21  rport-4:0-5
	rport-15:0-11  rport-15:0-17  rport-15:0-22  rport-15:0-7  rport-4:0-11  rport-4:0-17  rport-4:0-22  rport-4:0-6
	rport-15:0-12  rport-15:0-18  rport-15:0-23  rport-15:0-8  rport-4:0-12  rport-4:0-18  rport-4:0-23  rport-4:0-7
	rport-15:0-13  rport-15:0-19  rport-15:0-3   rport-15:0-9  rport-4:0-13  rport-4:0-19  rport-4:0-24  rport-4:0-8
	node_name：节点名称
	port_id：远端端口 id
	port_name：远端端口名称
	port_state：远端端口状态
```

```txt
[rport-15:0-0]$ grep . *
grep: device: Is a directory
dev_loss_tmo:30
fast_io_fail_tmo:off
maxframe_size:2112 bytes
node_name:0x206400083149d5d1
port_id:0xfffffe
port_name:0x200100083149d5d0
port_state:Online
grep: power: Is a directory
roles:Fabric Port
scsi_target_id:-1
grep: subsystem: Is a directory
supported_classes:Class 2, Class 3
```

```txt
ca:00.1 Fibre Channel [0c04]: Emulex Corporation LPe35000/LPe36000 Series 32Gb/64Gb Fibre Channel Adapter [10df:f400]
```

```txt
ls -la /sys/class/fc_transport
total 0
drwxr-xr-x  2 root root 0 Feb  2 18:12 .
drwxr-xr-x 76 root root 0 Feb  2 18:12 ..
lrwxrwxrwx  1 root root 0 Feb  2 18:12 target15:0:0 -> ../../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.1/host15/rport-15:0-7/target15:0:0/fc_transport/target15:0:0
lrwxrwxrwx  1 root root 0 Feb  2 18:12 target15:0:6 -> ../../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.1/host15/rport-15:0-20/target15:0:6/fc_transport/target15:0:6
lrwxrwxrwx  1 root root 0 Feb  2 18:12 target4:0:0 -> ../../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.0/host4/rport-4:0-7/target4:0:0/fc_transport/target4:0:0
lrwxrwxrwx  1 root root 0 Feb  2 18:13 target4:0:6 -> ../../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.0/host4/rport-4:0-22/target4:0:6/fc_transport/target4:0:6
```

```txt
[root@csfs-elf-130-84 20:31:30 var]$ ls -la /sys/class/fc_host/
total 0
drwxr-xr-x  2 root root 0 Feb  2 18:12 .
drwxr-xr-x 76 root root 0 Feb  2 18:12 ..
lrwxrwxrwx  1 root root 0 Feb  2 18:13 host15 -> ../../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.1/host15/fc_host/host15
lrwxrwxrwx  1 root root 0 Feb  2 18:13 host4 -> ../../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.0/host4/fc_host/host4
```
```txt
[root@csfs-elf-130-84 20:37:56 var]$ ls -la /sys/class/fc_remote_ports
total 0
drwxr-xr-x  2 root root 0 Feb  2 18:12 .
drwxr-xr-x 76 root root 0 Feb  2 18:12 ..
lrwxrwxrwx  1 root root 0 Feb  2 18:13 rport-15:0-0 -> ../../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.1/host15/rport-15:0-0/fc_remote_ports/rport-15:0-0
lrwxrwxrwx  1 root root 0 Feb  2 18:13 rport-15:0-1 -> ../../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.1/host15/rport-15:0-1/fc_remote_ports/rport-15:0-1
lrwxrwxrwx  1 root root 0 Feb  2 18:13 rport-15:0-10 -> ../../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.1/host15/rport-15:0-10/fc_remote_ports/rport-15:0-10
...
```

```txt
$ ls -la /sys/block
total 0
drwxr-xr-x  2 root root 0 Feb  2 18:12 .
dr-xr-xr-x 13 root root 0 Feb  2 18:12 ..
lrwxrwxrwx  1 root root 0 Feb  2 18:22 dm-0 -> ../devices/virtual/block/dm-0
lrwxrwxrwx  1 root root 0 Feb  2 18:22 dm-1 -> ../devices/virtual/block/dm-1
lrwxrwxrwx  1 root root 0 Feb  2 18:22 dm-2 -> ../devices/virtual/block/dm-2
lrwxrwxrwx  1 root root 0 Feb  2 18:13 loop0 -> ../devices/virtual/block/loop0
lrwxrwxrwx  1 root root 0 Feb  2 18:12 nvme0n1 -> ../devices/pci0000:30/0000:30:02.0/0000:31:00.0/nvme/nvme0/nvme0n1
lrwxrwxrwx  1 root root 0 Feb  2 18:12 nvme1n1 -> ../devices/pci0000:30/0000:30:03.0/0000:32:00.0/nvme/nvme1/nvme1n1
lrwxrwxrwx  1 root root 0 Feb  2 18:12 nvme2n1 -> ../devices/pci0000:64/0000:64:02.0/0000:65:00.0/nvme/nvme2/nvme2n1
lrwxrwxrwx  1 root root 0 Feb  2 18:12 nvme3n1 -> ../devices/pci0000:64/0000:64:03.0/0000:66:00.0/nvme/nvme3/nvme3n1
lrwxrwxrwx  1 root root 0 Feb  2 18:12 sda -> ../devices/pci0000:00/0000:00:1d.0/0000:05:00.0/ata11/host12/target12:0:0/12:0:0:0/block/sda
lrwxrwxrwx  1 root root 0 Feb  2 18:13 sdb -> ../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.1/host15/rport-15:0-7/target15:0:0/15:0:0:1/block/sdb
lrwxrwxrwx  1 root root 0 Feb  2 18:13 sdc -> ../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.1/host15/rport-15:0-7/target15:0:0/15:0:0:2/block/sdc
lrwxrwxrwx  1 root root 0 Feb  2 18:13 sdd -> ../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.0/host4/rport-4:0-7/target4:0:0/4:0:0:1/block/sdd
lrwxrwxrwx  1 root root 0 Feb  2 18:13 sde -> ../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.0/host4/rport-4:0-7/target4:0:0/4:0:0:2/block/sde
lrwxrwxrwx  1 root root 0 Feb  2 18:13 sdf -> ../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.1/host15/rport-15:0-20/target15:0:6/15:0:6:1/block/sdf
lrwxrwxrwx  1 root root 0 Feb  2 18:13 sdg -> ../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.1/host15/rport-15:0-20/target15:0:6/15:0:6:2/block/sdg
lrwxrwxrwx  1 root root 0 Feb  2 18:13 sdh -> ../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.0/host4/rport-4:0-22/target4:0:6/4:0:6:1/block/sdh
lrwxrwxrwx  1 root root 0 Feb  2 18:13 sdi -> ../devices/pci0000:c9/0000:c9:04.0/0000:ca:00.0/host4/rport-4:0-22/target4:0:6/4:0:6:2/block/sdi
lrwxrwxrwx  1 root root 0 Feb  2 18:13 sdj -> ../devices/platform/host16/session1/target16:0:0/16:0:0:0/block/sdj
lrwxrwxrwx  1 root root 0 Feb  2 18:13 sdk -> ../devices/platform/host17/session2/target17:0:0/17:0:0:0/block/sdk
lrwxrwxrwx  1 root root 0 Feb  2 18:13 sdl -> ../devices/platform/host18/session3/target18:0:0/18:0:0:0/block/sdl
$ lspci -s 0000:ca:00.1
ca:00.1 Fibre Channel: Emulex Corporation LPe35000/LPe36000 Series 32Gb/64Gb Fibre Channel Adapter
```

```txt
 lsblk
NAME                                MAJ:MIN RM   SIZE RO TYPE  MOUNTPOINT
loop0                                 7:0    0    20G  0 loop  /var/lib/zookeeper
sda                                   8:0    0 223.5G  0 disk
├─sda1                                8:1    0     5G  0 part  /boot
└─sda2                                8:2    0   217G  0 part  /
sdb                                   8:16   0    10G  0 disk
└─3600a098038314b7334245854726b6548 253:0    0    10G  0 mpath
sdc                                   8:32   0     5G  0 disk
└─3600a098038314b7334245854726b6565 253:1    0     5G  0 mpath
sdd                                   8:48   0    10G  0 disk
└─3600a098038314b7334245854726b6548 253:0    0    10G  0 mpath
sde                                   8:64   0     5G  0 disk
└─3600a098038314b7334245854726b6565 253:1    0     5G  0 mpath
sdf                                   8:80   0    10G  0 disk
└─3600a098038314b7334245854726b6548 253:0    0    10G  0 mpath
sdg                                   8:96   0     5G  0 disk
└─3600a098038314b7334245854726b6565 253:1    0     5G  0 mpath
sdh                                   8:112  0    10G  0 disk
└─3600a098038314b7334245854726b6548 253:0    0    10G  0 mpath
sdi                                   8:128  0     5G  0 disk
└─3600a098038314b7334245854726b6565 253:1    0     5G  0 mpath
sdj                                   8:144  0   3.8G  0 disk
└─3600a098038314b7334245854726b6b52 253:2    0   3.8G  0 mpath
sdk                                   8:160  0   3.8G  0 disk
└─3600a098038314b7334245854726b6b52 253:2    0   3.8G  0 mpath
sdl                                   8:176  0   3.8G  0 disk
└─3600a098038314b7334245854726b6b52 253:2    0   3.8G  0 mpath
```

这么看，rport 的确很多，但是只有两个是 target ，一个 target 下可以有多个盘的。


## 分析一下代码

这些注释都是 AI 写的
```c
struct fc_function_template {
    /* Remote Port 管理回调 */
    void (*get_rport_dev_loss_tmo)(struct fc_rport *);
    void (*set_rport_dev_loss_tmo)(struct fc_rport *, u32);

    /* SCSI Target 属性获取 */
    void (*get_starget_node_name)(struct scsi_target *);
    void (*get_starget_port_name)(struct scsi_target *);
    void (*get_starget_port_id)(struct scsi_target *);

    /* Host 属性获取 */
    void (*get_host_port_id)(struct Scsi_Host *);
    void (*get_host_port_type)(struct Scsi_Host *);
    void (*get_host_port_state)(struct Scsi_Host *);
    void (*get_host_active_fc4s)(struct Scsi_Host *);
    void (*get_host_speed)(struct Scsi_Host *);
    void (*get_host_fabric_name)(struct Scsi_Host *);
    void (*get_host_symbolic_name)(struct Scsi_Host *);
    void (*set_host_system_hostname)(struct Scsi_Host *);

    /* 统计和管理 */
    struct fc_host_statistics * (*get_fc_host_stats)(struct Scsi_Host *);
    void (*reset_fc_host_stats)(struct Scsi_Host *);

    /* LIP (Loop Initialization Protocol) 操作 */
    int (*issue_fc_host_lip)(struct Scsi_Host *);

    /* Remote Port 生命周期回调 */
    void (*dev_loss_tmo_callbk)(struct fc_rport *);
    void (*terminate_rport_io)(struct fc_rport *);

    /* Virtual Port 管理 */
    void (*set_vport_symbolic_name)(struct fc_vport *);
    int (*vport_create)(struct fc_vport *, bool);
    int (*vport_disable)(struct fc_vport *, bool);
    int (*vport_delete)(struct fc_vport *);

    /* BSG (Block SCSI Generic) 支持 */
    u32 max_bsg_segments;
    int (*bsg_request)(struct bsg_job *);
    int (*bsg_timeout)(struct bsg_job *);

    /* 私有数据大小 */
    u32 dd_fcrport_size;    // remote port 私有数据大小
    u32 dd_fcvport_size;    // virtual port 私有数据大小
    u32 dd_bsg_size;        // BSG 私有数据大小

    /* Sysfs 显示控制标志 */
    unsigned long show_rport_maxframe_size:1;
    unsigned long show_rport_supported_classes:1;
    unsigned long show_starget_node_name:1;
    unsigned long show_starget_port_name:1;
    unsigned long show_starget_port_id:1;
    unsigned long show_host_node_name:1;
    unsigned long show_host_port_name:1;
    unsigned long show_host_port_id:1;
    unsigned long show_host_port_type:1;
    unsigned long show_host_port_state:1;
    unsigned long show_host_speed:1;
    unsigned long show_host_fabric_name:1;
    unsigned long show_host_symbolic_name:1;
    // ... 还有更多控制标志

    unsigned long disable_target_scan:1;
};
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
