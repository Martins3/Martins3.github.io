# sysfs

## 基本使用
###  dev
分为 block 和 char ，最后指向 devices 目录，应该只是 legacy 目录
###  block
也是指向 devices

###  bus
正如其名，描述了主板的 typo 结构，但是很多 bus 无法理解。
```txt
 ac97          clocksource    gpio      machinecheck     node          pnp      soundwire   xen
 acpi          container      hdaudio   mei              nvmem         scsi     spi         xen-backend
 auxiliary     cpu            hid       memory           pci           serial   usb
 cec           dax            i2c       memory_tiering   pci_express   serio    wmi
 clockevents   event_source   isa       mipi-dsi         platform      soc      workqueue
```
###  class
对于 devices 的分类，都是指向 devices 的软链接
###  devices

完全看不懂
```txt
 breakpoint    intel_pt      pnp0           uncore_cbox_0   uncore_cbox_7    uncore_imc_1
 cpu_atom      isa           power          uncore_cbox_1   uncore_cbox_8    uncore_imc_free_running_0
 cpu_core      kprobe        software       uncore_cbox_2   uncore_cbox_9    uncore_imc_free_running_1
 cstate_core   LNXSYSTM:00   system         uncore_cbox_3   uncore_cbox_10   uprobe
 cstate_pkg    msr           tracepoint     uncore_cbox_4   uncore_cbox_11   virtual
 i915          pci0000:00    uncore_arb_0   uncore_cbox_5   uncore_clock
 intel_bts     platform      uncore_arb_1   uncore_cbox_6   uncore_imc_0
```
1. 不知道为什么 breakpointw 在启动
2. 还存在一些电源管理的东西
3. cpu 也是放到这里的

###  firmware

-  acpi   : acpi table 之类的东西
-  dmi    : dmidecode ...
-  efi    : efi var ...
-  memmap : https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-firmware-memmap

###  fs
文件系统的配置选项:
 bpf   cgroup   ext4   fuse   pstore   selinux

启动包含大名鼎鼎的 /sys/fs/cgroup/

###  hypervisor
不知道这个做啥的，host 中空的，guest 中没有这个文件夹

###  kernel

| 痛苦                 | 解释    |
|----------------------|---------|
|  address_bits       |         |
|  boot_params        |         |
|  btf                |         |
|  cgroup             |         |
|  config             | |
|  cpu_byteorder      |         |
|  debug              | debugfs |
|  fscaps             |         |
|  iommu_groups       |         |
|  irq                |         |
|  kexec_crash_loaded |         |
|  kexec_crash_size   |         |
|  kexec_loaded       |         |
|  mm                 |         |
|  notes              |         |
|  oops_count         |         |
|  profiling          |         |
|  rcu_expedited      |         |
|  rcu_normal         |         |
|  reboot             |         |
|  security           |         |
|  slab               | TODO    |
|  software_nodes     |         |
|  tracing            | ftrace  |
|  uevent_seqnum      |         |
|  vmcoreinfo         |         |
|  warn_count         |         |

###  module

###  power

## 具体问题
drivers/md/md.c::state_store 对应的用户态接口是，这个代码目录是如何生成的，感觉有点懵逼

/sys/devices/virtual/block/md10/md/dev-sda1/state


## 为什么这个 module 的参数是可以写的，但是
scsi 模块的下的 eh_deadline 是不可以写
```txt
/sys/module/scsi_debug/parameters🔒
🧀  ls
 add_host         inq_vendor           num_tgts          unmap_alignment
 ato              lbprz                opt_blks          unmap_granularity
 cdb_len          lbpu                 opt_xferlen_exp   unmap_max_blocks
 clustering       lbpws                opts              unmap_max_desc
 delay            lbpws10              per_host_store    uuid_ctl
 dev_size_mb      lowest_aligned       physblk_exp       virtual_gb
 dif              lun_format           poll_queues       vpd_use_hostno
 dix              max_luns             ptype             wp
 dsense           max_queue            random            write_same_length
 every_nth        medium_error_count   removable         zbc
 fake_rw          medium_error_start   scsi_level        zone_cap_mb
 guard            ndelay               sector_size       zone_max_open
 host_lock        no_lun_0             statistics        zone_nr_conv
 host_max_queue   no_rwlock            strict            zone_size_mb
 inq_product      no_uld               submit_queues
 inq_rev          num_parts            tur_ms_to_ready
```

可以找到:
```sh
static struct attribute *sdebug_drv_attrs[] = {
```

想不到唯一 reference 的位置是在这里:
```c
static struct bus_type pseudo_lld_bus = {
	.name = "pseudo",
	.probe = sdebug_driver_probe,
	.remove = sdebug_driver_remove,
	.drv_groups = sdebug_drv_groups,
};
```
从代码上看，这些操作应该是在这里的: /sys/bus//pseudo/drivers/scsi_debug/no_lun_0

测试显示，只是因为我没有搞清楚 modules 参数含义，这些 module 确实是可以写的，但是一个是经过
delay_store 的，一个不是的。

- [ ] 所以，kernel 的 modules 参数是通过什么方式写的，可以看看。

## 从 bus_type 分析阻止结构
看来 groups 的定义很有意思，在 pci 这里可以看到
```c
struct bus_type pci_bus_type = {
	.name		= "pci",
	.match		= pci_bus_match,
	.uevent		= pci_uevent,
	.probe		= pci_device_probe,
	.remove		= pci_device_remove,
	.shutdown	= pci_device_shutdown,
	.dev_groups	= pci_dev_groups, //  /sys/devices/pci0000:00/0000:00:02.0/rom
	.bus_groups	= pci_bus_groups, //  /sys/bus/pci/rescan
	.drv_groups	= pci_drv_groups, //  /sys/bus/pci/drivers/megaraid_sas/new_id
	.pm		= PCI_PM_OPS_PTR,
	.num_vf		= pci_bus_num_vf,
	.dma_configure	= pci_dma_configure,
	.dma_cleanup	= pci_dma_cleanup,
};
```

所有的 attribute groups 都是注册到这里的吗?

## 检查更多的 groups 看看

## 想不到网络设备也是如此
```txt
🧀  find . -name enp5s0
find: ‘./kernel/debug’: Permission denied
./class/net/enp5s0
./devices/pci0000:00/0000:00:1c.2/0000:05:00.0/net/enp5s0
find: ‘./fs/pstore’: Permission denied
find: ‘./fs/bpf’: Permission denied
```

## 如何才可以观测到某一个网卡的速率

## /sys/block/nvme0n1/stat

和 /proc/diskstats 中内容是等价的

## /proc/diskstats

## 分析下 sys 下的软链接

/sys/kernel/iommu_groups : 这里展现出来了 iommu 和 pci 设备的关系

## [ ] 类似这种 /proc/irq 看看

ls /proc/irq/65
affinity_hint  effective_affinity  effective_affinity_list  node  nvme1q1  smp_affinity  smp_affinity_list  spurious

为什么这里存在一个空的 nvme1q1 的文件夹?


- [x] 为什么 smp_affinity 虚拟机中完全无法修改了?
  - 例如 nvme1q1 锁对应的，是因为这是本来就是给那个 CPU 的吗?
  - 是的，换成网卡就一下子修改了

## /sys/kernel/irq 下内容看看


## 这文件下的内容看看
- linux/fs/proc/task_mmu.c


## /sys/class/scsi_* 一共存在三个文件夹，很是有趣的!

## /proc/sys/kernel 牛逼的目录

## 居然 /sys/block/sda 是存储创建时间的，这是怎么实现的

/sys/block 的目录是如何创建的:

device_add_disk 中:

```c
	ret = sysfs_create_link(block_depr, &ddev->kobj,
				kobject_name(&ddev->kobj));
```
目录是在初始化的时候创建的，但是

## 这种库应该很多吧
https://github.com/valpackett/systemstat
https://github.com/GuillaumeGomez/sysinfo

是一个方便对比各种 os 的机会

## 记录各种神奇的的文件，之后逐个分析

### 1. 这里的 pci 下面又有一个 scsi 总线的效果
```txt
🤒  find /sys -name max_retries
/sys/devices/pci0000:00/0000:00:17.0/ata8/host7/target7:0:0/7:0:0:0/scsi_disk/7:0:0:0/max_retries
/sys/devices/pci0000:00/0000:00:17.0/ata7/host6/target6:0:0/6:0:0:0/scsi_disk/6:0:0:0/max_retries
```


### 2. 看看这个有趣的 device
```txt
/sys/block/nvme0n1🔒 🦤
🧀  ls -la
.r--r--r-- 4.1k root  2 Nov 11:45  alignment_offset
lrwxrwxrwx    0 root  2 Nov 10:05  bdi -> ../../../../../../virtual/bdi/259:0
.r--r--r-- 4.1k root  2 Nov 11:45  capability
.r--r--r-- 4.1k root  2 Nov 11:45  dev
lrwxrwxrwx    0 root  2 Nov 10:05  device -> ../../nvme0
.r--r--r-- 4.1k root  2 Nov 11:45  discard_alignment
.r--r--r-- 4.1k root  2 Nov 11:45  diskseq
.r--r--r-- 4.1k root  2 Nov 11:45  eui
.r--r--r-- 4.1k root  2 Nov 11:45  events
```
第一次 device 指向 nvme

cd 到其中，可以看到 device 又是指向到 pcie
```txt
block/nvme0n1/device🔒 🦤
🧀  ls -la
.r--r--r-- 4.1k root  2 Nov 11:45  address
.r--r--r-- 4.1k root  2 Nov 11:45  cntlid
.r--r--r-- 4.1k root  2 Nov 11:45  cntrltype
.r--r--r-- 4.1k root  2 Nov 11:45  dctype
.r--r--r-- 4.1k root  2 Nov 11:45  dev
lrwxrwxrwx    0 root  1 Nov 10:11  device -> ../../../0000:02:00.0
```

### 3. /home/martins3/core/vn/docs/kernel/sysfs-scsi.md 可以重新整理一下

## 这也是一个模式吧
- kobj_to_hstate
  - 当内核中写 /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages 的时候，通过这个可以知道当前的 node 是什么


## 不错
https://mp.weixin.qq.com/s/dAoO67TlWDBS8YmDEsZXDA


## 这个 device->devt 有用吗?
```txt
int bus_for_each_dev(const struct bus_type *bus, struct device *start,
		     void *data, device_iter_t fn)
{
	struct subsys_private *sp = bus_to_subsys(bus);
	struct klist_iter i;
	struct device *dev;
	int error = 0;

	if (!sp)
		return -EINVAL;

	klist_iter_init_node(&sp->klist_devices, &i,
			     (start ? &start->p->knode_bus : NULL));
	while (!error && (dev = next_device(&i))){
		pr_info("[martins3:%s:%d] %s %d %d\n", __FUNCTION__, __LINE__, bus->name, dev->devt, dev->id);
		error = fn(dev, data);
	}
	klist_iter_exit(&i);
	subsys_put(sp);
	return error;
}
```

没有用，全部都是 0 ，很坑

## /sys/devices/virtual/ 这个下面都是有趣的东西

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
