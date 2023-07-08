# sysfs

[sysfs 和 proc 的区别](https://unix.stackexchange.com/questions/4884/what-is-the-difference-between-procfs-and-sysfs)，更加的结构化。

sysfs 应该都是各种 kobject 或者 device 之类的暴露 !

https://www.ibm.com/developerworks/cn/linux/l-cn-sysfs/

## Implementation


```txt
===============================================================================
 Language            Files        Lines         Code     Comments       Blanks
===============================================================================
 C                       5         1829         1130          436          263
 C Header                1           43           15           20            8
 Makefile                1            6            1            4            1
===============================================================================
 Total                   7         1878         1146          460          272
===============================================================================
```

一共几个小文件, sysfs/mount.c 一个很小的文件

### filesystem part

1. sysfs 也是和 ns 相关的，其中的 ns 如何使用 ?

2. 基于 kernfs，将 vfs 的操作 和 attribute 提供的参数关联起来。

> 从文件系统的角度分析，为什么可以创建出来这些内容

```c
struct attribute {
        char                    * name;
        umode_t                 mode;
};


int sysfs_create_file(struct kobject * kobj, const struct attribute * attr);
void sysfs_remove_file(struct kobject * kobj, const struct attribute * attr);

static inline int __must_check sysfs_create_file(struct kobject *kobj,
						 const struct attribute *attr)
{
	return sysfs_create_file_ns(kobj, attr, NULL);
}

/**
 * device_create_file - create sysfs attribute file for device.
 * @dev: device.
 * @attr: device attribute descriptor.
 */
int device_create_file(struct device *dev,
		       const struct device_attribute *attr) // 只是功能变的复杂了，但是内容并没有什么变化
{
	int error = 0;

	if (dev) {
		WARN(((attr->attr.mode & S_IWUGO) && !attr->store),
			"Attribute %s: write permission without 'store'\n",
			attr->attr.name);
		WARN(((attr->attr.mode & S_IRUGO) && !attr->show),
			"Attribute %s: read permission without 'show'\n",
			attr->attr.name);
		error = sysfs_create_file(&dev->kobj, &attr->attr);
	}

	return error;
}
```

Documentation/filesystems/sysfs.txt : 说对于每一个文件，其 show 和 store 函数存在一个 page 作为 buffer
的，关于这一点验证，以及说明很多规则，关于这一点的说明，应该在 kernfs 中间 :

```c
	ssize_t (*show)(struct device *dev, struct device_attribute *attr,
			char *buf);
	ssize_t (*store)(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count);
```

### device model part

> 到底提供了那些帮助

### function

1. 为什么需要 sfsfs ?
2. 如何将设备的信息通过 sfsfs export 出来 ?
      > 分析一下 /sys/ 的内容

More information can driver-model specific features can be found in
Documentation/driver-api/driver-model/.

The kernel provides a representation of its model in userspace through the sysfs virtual file system. It is usually mounted in the /sys directory and contains the following subdirectories:

block - all block devices available in the system (disks, partitions)
bus - types of bus to which physical devices are connected (pci, ide, usb).两个子目录 devices 和 drivers 表示关联到该 bus 的设备和驱动
class - drivers classes that are available in the system (net, sound, usb)
devices - the hierarchical structure of devices connected to the system.
firmware - information from system firmware (ACPI)
fs - information about mounted file systems: 提供文件系统以及设备的信息。
kernel - kernel status information (logged-in users, hotplug)
modules - the list of modules currently loaded
power - information related to the power management subsystem
dev - _It can be used to automatically create entries in the /dev directory_. 包含 char 和 block 两个文件夹，其中都是 symbol link
dev/ contains two directories char/ and block/. Inside these two
directories there are symlinks named <major>:<minor>. These symlinks
point to the sysfs directory for the given device. /sys/dev provides a
quick way to lookup the sysfs interface for a device from the result of
a stat(2) operation.

As you can see, there is a correlation between the kernel data structures within the described model and the subdirectories in the sysfs virtual file system. Although this likeness may lead to confusion between the two concepts, they are different. The kernel device model can work without the sysfs file system, but the reciprocal is not true.

sysfs 是可有可无的，
sysfs is always compiled in if CONFIG_SYSFS is defined. You can access
it by doing:

plain mount -t sysfs sysfs /sys

1. For every kobject that is registered with the system, a directory is created for it in sysfs.
      > 每一个 kobject 在 sysfs 中间对应一个文件夹
2. That directory is created as a subdirectory of the kobject's parent, expressing internal object hierarchies to userspace.
      > 一个 kobject 对应的文件夹在其 parent 对应的文件夹中间
3. Top-level directories in sysfs represent the common ancestors of object hierarchies; i.e. the subsystems the objects belong to.

4. Attributes can be exported for _kobjects_ in the form of _regular files_ in the filesystem.
   并且提供文件属性
      > 属性在 sfs 中间的存在形式是 文件

### kobject

kobject --- sysfs ---- kernfs

```c
fs_kobj = kobject_create_and_add("fs", NULL);
power_kobj = kobject_create_and_add("power", NULL);
kernel_kobj = kobject_create_and_add("kernel", NULL);
dev_kobj = kobject_create_and_add("dev", NULL);
firmware_kobj = kobject_create_and_add("firmware", NULL);
block_depr = kobject_create_and_add("block", NULL);

devices_kset = kset_create_and_add("devices", &device_uevent_ops, NULL);
class_kset = kset_create_and_add("class", NULL, NULL);
module_kset = kset_create_and_add("module", &module_uevent_ops, NULL);
bus_kset = kset_create_and_add("bus", &bus_uevent_ops, NULL);
```

```plain
kobject创建：
-----------
struct kobject *kobject_create_and_add(const char *name, struct kobject *parent)
	int kobject_add(struct kobject *kobj, struct kobject *parent, const char *fmt, ...)
		static int object_add_varg (vargs=...
			static int kobject_add_internal(struct kobject *kobj)
				static int create_dir(struct kobject *kobj)
					int sysfs_create_dir_ns(struct kobject *kobj, const void *ns)  //sysfs_root_kn
						struct kernfs_node *kernfs_create_dir_ns(struct kernfs_node *parent,
							struct kernfs_node *kernfs_new_node(struct kernfs_node *parent,

kset创建：
--------
struct kset *kset_create_and_add(const char *name,
	int kset_register(struct kset *k)
		static int kobject_add_internal(struct kobject *kobj)
```

### sysfs/group.c

其中的参数全部含有 kobject 值的分析的内容，

```c
/**
 * sysfs_create_group - given a directory kobject, create an attribute group
 * @kobj:	The kobject to create the group on
 * @grp:	The attribute group to create
 *
 * This function creates a group for the first time.  It will explicitly
 * warn and error if any of the attribute files being created already exist.
 *
 * Returns 0 on success or error code on failure.
 */
int sysfs_create_group(struct kobject *kobj,
		       const struct attribute_group *grp)
{
	return internal_create_group(kobj, 0, grp);
}
```

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
|  config             |         |
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
