## ldd3 的驱动理解

- [ ] 似乎存在一个 block device 的结构体 ?

- [ ] `register_blkdev` , `add_disk`, `get_gendisk`
  - [x] `register_blkdev` 注册了 major number 在 `major_names` 中间，但是 `major_names` 除了使用在 `blkdev_show` (cat /proc/devices) 之外没有看到其他的用处
    - https://linux-kernel-labs.github.io/refs/heads/master/labs/block_device_drivers.html : 中说，`register_blkdev` 是会取消掉的
    - 从 virtio_blk.c 中间来看 : major 放到局部变量中间，所以实际功能是分配 major number

- [ ] `alloc_disk` 分配 `struct gendisk`，其中由于保存分区的

block_device_operations

Char devices make their operations available to the system by way of the `file_operations` structure. A similar structure is used with block devices; it is `struct
block_device_operations`, which is declared in `<linux/blkdev.h>`.

```c
struct block_device_operations {
	int (*open) (struct block_device *, fmode_t);
	void (*release) (struct gendisk *, fmode_t);
	int (*rw_page)(struct block_device *, sector_t, struct page *, int rw);
	int (*ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
	int (*compat_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
	long (*direct_access)(struct block_device *, sector_t, void __pmem **,
			unsigned long *pfn);
	unsigned int (*check_events) (struct gendisk *disk,
				      unsigned int clearing);
	/* ->media_changed() is DEPRECATED, use ->check_events() instead */
	int (*media_changed) (struct gendisk *);
	void (*unlock_native_capacity) (struct gendisk *);
	int (*revalidate_disk) (struct gendisk *);
	int (*getgeo)(struct block_device *, struct hd_geometry *);
	/* this callback is with swap_lock and sometimes page table lock held */
	void (*swap_slot_free_notify) (struct block_device *, unsigned long);
	struct module *owner;
	const struct pr_ops *pr_ops;
};
```
1. 这些函数的注册都是让驱动完成的
2. 这些函数的使用位置在哪里啊 ?
    1. open : blkdev_get : @todo 当设备出现在 /dev/nvme0n1p1 上的时候，此时 blkdev_get 被调用过没有，blkdev_get 会被调用吗 ? 如果不调用，那么似乎驱动就没有被初始化，那么连 aops 的实现基础 bio request 之类的实现似乎无从谈起了。


## 使用 drivers/scsi/sd.c 作为例子分析一下

分析一下这个文件: block/genhd.c

register_blkdev 的调用，只要内存存在这个模块，就会进行对应的注册。

## add_disk
```plain
#0  device_add_disk (parent=parent@entry=0xffff888100c22010, disk=0xffff8880055f3400, groups=groups@entry=0xffffffff82e1b8d0 <virtblk_attr_groups>) at block/genhd.c:393
#1  0xffffffff81aa2ba1 in virtblk_probe (vdev=0xffff888100c22000) at drivers/block/virtio_blk.c:1150
#2  0xffffffff81809e6b in virtio_dev_probe (_d=0xffff888100c22010) at drivers/virtio/virtio.c:305
#3  0xffffffff81a75481 in call_driver_probe (drv=0xffffffff82e1b760 <virtio_blk>, dev=0xffff888100c22010) at drivers/base/dd.c:560
#4  really_probe (dev=dev@entry=0xffff888100c22010, drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:639
#5  0xffffffff81a756bd in __driver_probe_device (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>, dev=dev@entry=0xffff888100c22010) at drivers/base/dd.c:778
#6  0xffffffff81a75749 in driver_probe_device (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>, dev=dev@entry=0xffff888100c22010) at drivers/base/dd.c:808
#7  0xffffffff81a759c5 in __driver_attach (data=0xffffffff82e1b760 <virtio_blk>, dev=0xffff888100c22010) at drivers/base/dd.c:1194
#8  __driver_attach (dev=0xffff888100c22010, data=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:1134
#9  0xffffffff81a72fc4 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff82e1b760 <virtio_blk>, fn=fn@entry=0xffffffff81a75940 <__driver_attach>) at drivers/base/bus.c:301
#10 0xffffffff81a74e79 in driver_attach (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:1211
#11 0xffffffff81a74810 in bus_add_driver (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/bus.c:618
#12 0xffffffff81a76c2e in driver_register (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/driver.c:246
#13 0xffffffff8180958b in register_virtio_driver (driver=driver@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/virtio/virtio.c:357
#14 0xffffffff835eb0fb in virtio_blk_init () at drivers/block/virtio_blk.c:1284
#15 0xffffffff81001940 in do_one_initcall (fn=0xffffffff835eb0aa <virtio_blk_init>) at init/main.c:1306
#16 0xffffffff8359b818 in do_initcall_level (command_line=0xffff888003c5cc00 "root", level=6) at init/main.c:1379
#17 do_initcalls () at init/main.c:1395
#18 do_basic_setup () at init/main.c:1414
#19 kernel_init_freeable () at init/main.c:1634
#20 0xffffffff82183405 in kernel_init (unused=<optimized out>) at init/main.c:1522
#21 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

`genhd.c` 中间包含如下了内容:

```c
struct gendisk {
	/* major, first_minor and minors are input parameters only,
	 * don't use directly.  Use disk_devt() and disk_max_parts().
	 */
	int major;			/* major number of driver */
	int first_minor;
	int minors;                     /* maximum number of minors, =1 for
                                         * disks that can't be partitioned. */

	char disk_name[DISK_NAME_LEN];	/* name of major driver */
	char *(*devnode)(struct gendisk *gd, umode_t *mode);

	unsigned int events;		/* supported events */
	unsigned int async_events;	/* async events, subset of all */

	/* Array of pointers to partitions indexed by partno.
	 * Protected with matching bdev lock but stat and other
	 * non-critical accesses use RCU.  Always access through
	 * helpers.
	 */
	struct disk_part_tbl __rcu *part_tbl;
	struct hd_struct part0;

	const struct block_device_operations *fops;
	struct request_queue *queue;
	void *private_data;

	int flags;
	struct device *driverfs_dev;  // FIXME: remove
	struct kobject *slave_dir;

	struct timer_rand_state *random;
	atomic_t sync_io;		/* RAID */
	struct disk_events *ev;
#ifdef  CONFIG_BLK_DEV_INTEGRITY
	struct kobject integrity_kobj;
#endif	/* CONFIG_BLK_DEV_INTEGRITY */
	int node_id;
};
```
1. `major` specifies the major number of the driver; `first_minor` and `minors` indicate the range
within which minor numbers may be located (we already know that each partition is allocated
its own minor number).
2. `disk_name` gives a name to the disk. It is used to represent the disk in sysfs and in
/proc/partitions.
3. `part` is an array consisting of pointers to `hd_struct`, whose definition is given below. There is
one entry for each disk partition.
4. If `part_uevent_suppress` is set to a positive value, no hotplug events are sent to userspace if
changes in the partition information of the device are detected. This is only used for the initial
partition scan that occurs before the disk is fully integrated into the system.
5. fops is a pointer to device-specific functions that perform various low-level tasks. I discuss these
below.(没有了)
6. `queue` is needed to manage request queues, which I discuss below.(没有了)
7. `private_data` is a pointer to private driver data not modified by the generic functions of the
block layer.
8. `capacity` specifies the disk capacity in sectors.(没有了!)
9. `driverfs_dev` identifies the hardware device to which the disk belongs. The destination is an
object of the driver model, which is discussed in Section 6.7.1.
10. `kobj` is an integrated kobject instance for the generic kernel object model


For each partition, there is an instance of `hd_struct` to describe the key data of the partition within the device.
```c
struct hd_struct {
	sector_t start_sect;
	/*
	 * nr_sects is protected by sequence counter. One might extend a
	 * partition while IO is happening to it and update of nr_sects
	 * can be non-atomic on 32bit machines with 64bit sector_t.
	 */
	sector_t nr_sects;
	seqcount_t nr_sects_seq;
	sector_t alignment_offset;
	unsigned int discard_alignment;
	struct device __dev;
	struct kobject *holder_dir;
	int policy, partno;
	struct partition_meta_info *info;
#ifdef CONFIG_FAIL_MAKE_REQUEST
	int make_it_fail;
#endif
	unsigned long stamp;
#ifdef	CONFIG_SMP
	struct disk_stats __percpu *dkstats;
#else
	struct disk_stats dkstats;
#endif
	struct percpu_ref ref;
	struct rcu_work rcu_work;
};
```
The `parts`(in `block_device`) array is filled by various routines that examine the partition structure of the hard disk when
it is registered. The kernel supports a large number of partitioning methods to support coexistence with
most other systems on many architectures.

It is also important to note that instances of struct gendisk may not be individually allocated by drivers.
Instead, the auxiliary function `alloc_disk` must be used:
```c
#define alloc_disk_node(minors, node_id)				\
({									\
	static struct lock_class_key __key;				\
	const char *__name;						\
	struct gendisk *__disk;						\
									\
	__name = "(gendisk_completion)"#minors"("#node_id")";		\
									\
	__disk = __alloc_disk_node(minors, node_id);			\
									\
	if (__disk)							\
		lockdep_init_map(&__disk->lockdep_map, __name, &__key, 0); \
									\
	__disk;								\
})

#define alloc_disk(minors) alloc_disk_node(minors, NUMA_NO_NODE)
```
Given the number of minors for the device, calling this function automatically allocates the genhd instance
equipped with sufficient space for pointers to `hd_structs` of the individual partitions.

Only memory for the pointers is added; the partition instances are only allocated when an actual partition
is detected on the device and added with `add_partition`.

Additionally, `alloc_disk` integrates the new disk into the device model data structures.
Consequently, gendisks must not be destroyed by simply freeing them. Use `del_gendisk` instead


**Connecting the Pieces**

For each **partition** of a block device that has already been opened, there is an instance of `struct block_device`.
The objects for partitions are connected with the object for the complete device via
`bd_contains`. All `block_device` instances contain a link to their generic disk data structure `gen_disk` via
`bd_disk`. Note that while there are multiple `block_device` instances for a partitioned disk, one gendisk
instance is sufficient.

The gendisk instance points to an array with pointers to `hd_structs`. *Each represents one partition. If
a block_device represents a partition, then it contains a pointer to the hd_struct in question — the
hd_struct instances are shared between struct gendisk and struct block_device.*

Additionally, generic hard disks are integrated into the kobject framework as shown in **Figure 6-11**. The
block subsystem is represented by the kset instance block_subsystem. The kset contains a linked list on
which the embedded kobjects of each gendisk instance are collected.

> struct block_device ----> gendisk ----> hd_structs



**Block Device Operations**
```c
struct block_device_operations {
	int (*open) (struct block_device *, fmode_t);
	void (*release) (struct gendisk *, fmode_t);
	int (*rw_page)(struct block_device *, sector_t, struct page *, int rw);
	int (*ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
	int (*compat_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
	long (*direct_access)(struct block_device *, sector_t, void __pmem **,
			unsigned long *pfn);
	unsigned int (*check_events) (struct gendisk *disk,
				      unsigned int clearing);
	/* ->media_changed() is DEPRECATED, use ->check_events() instead */
	int (*media_changed) (struct gendisk *);
	void (*unlock_native_capacity) (struct gendisk *);
	int (*revalidate_disk) (struct gendisk *);
	int (*getgeo)(struct block_device *, struct hd_geometry *);
	/* this callback is with swap_lock and sometimes page table lock held */
	void (*swap_slot_free_notify) (struct block_device *, unsigned long);
	struct module *owner;
	const struct pr_ops *pr_ops;
};
```
The functions are not invoked directly by the `VFS code` but indirectly by the operations contained in the standard file operations for block devices,
`def_blk_fops`.
> 所以为什么包含这一个结构体的不是 struct block_device 而是 `struct gendisk`.


> 神奇的操作，所以
> 1. 如果 fdisk，gendisk , block device , inode 等等现在所有接触到的概念，都是对于 disk 和 ssd 通用的，so when the path diverge to two
> 2. what is driver name of disk and ssd in my computer ?
