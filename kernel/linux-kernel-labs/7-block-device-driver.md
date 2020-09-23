Although the register_blkdev() function obtains a major, it does not provide a device (disk) to the system. For creating and using block devices (disks), a specialized interface defined in linux/genhd.h is used.

Request queues implement an interface that allows the use of multiple I/O schedulers. A scheduler must sort the requests and present them to the driver in order to maximize performance. The scheduler also deals with the combination of adjacent requests (which refer to adjacent sectors of the disk).
> @todo 现在非常清楚将请求添加到 request queue 中间，但是
> 1. request queue 和 IO scheduler 的联系
> 2. IO scheduler 和 block device 驱动 ?
> 3. 从软件架构上 block device 提供了 request queue，那么岂不是其实 IO scheduler 其实只是一个可选项 ?

The function of type `request_fn_proc` is used to handle requests for working with the block device. This function is the equivalent of read and write functions encountered on character devices. 


blk_init_queue 和  被取消掉

A request for a block device is described by **struct request** structure.

To use the content of a struct bio structure, the structure’s support pages must be mapped to the kernel address space from where they can be accessed. For mapping /unmapping, use the `kmap_atomic` and the `kunmap_atomic` macros.

For simplify the processing of a struct bio, use the `bio_for_each_segment` macrodefinition.
In case request queues are used and you needed to process the requests at struct bio level, use the `rq_for_each_segment` macrodefinition instead of the `bio_for_each_segment` macrodefinition.

`bio`, a dynamic list of struct bio structures that is a set of buffers associated to the request; this field is accessed by macrodefinition rq_for_each_segment if there are multiple buffers, or by bio_data macrodefinition in case there is only one associated buffer;
> segment 的含义，其实可以猜测为 bio 中间对应的一个 buffer 为一个 segment，表示连续的物理地址区间。
> 所以 bio_for_each_segment : 处理 bio 中间的所有 buffer，rq_for_each_segment 处理 rq 中间每一个 bio 的每一个 segment


9. 试验 6 : USE_BIO_TRANSFER 和之前的区别体现在什么地方 ?
    1. 测试显示，只是巧合而已，由于 bio 机制的原因，当page 来自于同一个地方，那么并没有什么价值。
    2. 似乎使用 kmap_atomic 与否都是无所谓的 ? 并不是，还是相同的原因，bio_data 在只有一个 page 的时候可以用于获取地址。
    3. @todo 但是，kmap 在存在 highmem 下的作用还是一个谜!

10. 通过两个部分的试验 : bio request_queue 是上层，block driver 对于 bio 处理为下层任务
    1. 但是 IO scheduler 或者 block layer 对于 request_queue 的整合，现在不清楚的。
    2. 和具体物理设备如何打交道，目前不知道。

## TODO
1. IO scheduler 或者之类的东西到底是个什么情况 ?
2. partition 完全没有涉及到
3. 对于 blockdev 能不能采用类似 chardev 的 mknod ?
4. gendisk 和 blockdevice 两个结构体的作用是什么 ?

5. struct block_device 和 试验中间的 my_block_dev 是对称的存在的 ?
    1. 都存在 gendisk 和 request_queue
    2. 一个可用的 block device 总是需要持有: block_device
    3. 但是 data 和 size 找不到对应，很想知道最后走到 io 总线的位置是怎么样子的!
```c
static struct my_block_dev {
	spinlock_t lock;
	struct request_queue *queue;
	struct gendisk *gd;
	u8 *data;
	size_t size;
} g_dev;

struct block_device {
	dev_t			bd_dev;  /* not a kdev_t - it's a search key */
	int			bd_openers;
	struct inode *		bd_inode;	/* will die */
	struct super_block *	bd_super;
	struct mutex		bd_mutex;	/* open/close mutex */
	void *			bd_claiming;
	void *			bd_holder;
	int			bd_holders;
	bool			bd_write_holder;
#ifdef CONFIG_SYSFS
	struct list_head	bd_holder_disks;
#endif
	struct block_device *	bd_contains;
	unsigned		bd_block_size;
	u8			bd_partno;
	struct hd_struct *	bd_part;
	/* number of times partitions within this device have been opened. */
	unsigned		bd_part_count;
	int			bd_invalidated;
	struct gendisk *	bd_disk;
	struct request_queue *  bd_queue;
	struct backing_dev_info *bd_bdi;
	struct list_head	bd_list;
	/*
	 * Private data.  You must have bd_claim'ed the block_device
	 * to use this.  NOTE:  bd_claim allows an owner to claim
	 * the same device multiple times, the owner must take special
	 * care to not mess up bd_private for that case.
	 */
	unsigned long		bd_private;

	/* The counter of freeze processes */
	int			bd_fsfreeze_count;
	/* Mutex for freeze */
	struct mutex		bd_fsfreeze_mutex;
} __randomize_layout;
```

6. 对于 insmod 的理解有点问题，为什么 /dev/myblock 自动给安排上了 ?

7. /dev/vbd 是什么时候注册的，哪一个驱动在管理其 ?

8. kmap_atomic
    1. 为什么会产生 bug
    2. 产生的 bug 信息为什么看不懂 ?

## 
1. register_blkdev 和 add_disk 的区别:
    1. register_blkdev : 只要使用一个数值即可，和 register_chrdev_region 对称，cdev_add 之后将 cdev 和 设备号关联起来。

## 问题
需要找到硬件
