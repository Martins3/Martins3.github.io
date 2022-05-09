# genhd.c

1. 700 :  alloc_disk add_disk
2. 1300 : ref counter
3. end : events


## TODO
1. block/genhd.c 和 fs/block_dev.c 的关系是什么 ?
    1. genhd.c 处理都是 genhd 这个结构体 : 和具体的驱动处理
    2. block_dev 处理的是 block_device 这个内容 : 似乎是用来和 vfs
    3. 所以，两者如何是如何关联在一起的 ?

2. partion 和 minor number 的关系 ?
3. add_disk 调用的内容非常的多 !
4. ldd ch16 在 disk 初始化的过程中间，完成 make_request_fn 的注册，所以，整个 mq ，io scheduler 都是 lib 吗 ? driver 如何和他们协作的 ?

## add_disk 和 alloc_disk

```c
static inline void add_disk(struct gendisk *disk)
{
	device_add_disk(NULL, disk, NULL);
}

void device_add_disk(struct device *parent, struct gendisk *disk,
		     const struct attribute_group **groups)

{
	__device_add_disk(parent, disk, groups, true);
}
EXPORT_SYMBOL(device_add_disk);


#define alloc_disk(minors) alloc_disk_node(minors, NUMA_NO_NODE)

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
```


1. 到底如何管理设备 ? 至少让 fs mount 的时候可以找到它

```c
/**
 * register_blkdev - register a new block device
 *
 * @major: the requested major device number [1..BLKDEV_MAJOR_MAX-1]. If
 *         @major = 0, try to allocate any unused major number.
 * @name: the name of the new block device as a zero terminated string
 *
 * The @name must be unique within the system.
 *
 * The return value depends on the @major input parameter:
 *
 *  - if a major device number was requested in range [1..BLKDEV_MAJOR_MAX-1]
 *    then the function returns zero on success, or a negative error code
 *  - if any unused major number was requested with @major = 0 parameter
 *    then the return value is the allocated major number in range
 *    [1..BLKDEV_MAJOR_MAX-1] or a negative error code otherwise
 *
 * See Documentation/admin-guide/devices.txt for the list of allocated
 * major numbers.
 */
int register_blkdev(unsigned int major, const char *name) // 很简单，向major_names 中间注册即可发
```
